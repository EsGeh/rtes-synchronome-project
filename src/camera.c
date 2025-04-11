#include "camera.h"
#include "global.h"
#include "output.h"

#include <linux/videodev2.h>

#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>


/************************
 * private utils decl
*************************/

#define DEV_ERROR( FMT_STR, ...) { \
	int ret = snprintf( error_str, STR_BUFFER_SIZE, \
			FMT_STR, ## __VA_ARGS__ \
	); \
	if( ret >= STR_BUFFER_SIZE ) { \
		strncpy( &error_str[STR_BUFFER_SIZE-1-3], "...", 4); \
	} \
}

static char error_str[STR_BUFFER_SIZE] = "";

int ioctl_helper(int fh, unsigned int request, void *arg);

// clean up on error:
ret_t private_camera_exit( camera_t* camera );

ret_t negotiate_caps(
		camera_t* camera
);

ret_t reset_cropping(
		camera_t* camera
);

ret_t negotiate_format(
		camera_t* camera,
		const unsigned int min_width,
		const unsigned int min_height,
		const __u32* format,
		const bool format_required
);

/************************
 * API implementation
*************************/

ret_t camera_init(
		camera_t* camera,
		const char* dev_name
)
{
	assert( camera != NULL );
	// init data:
	(*camera) = (camera_t){
		.dev_file = -1,
		.buffer_container = {
			.buffers = NULL,
			.count = 0,
		},
	};
	strncpy( camera->dev_name, dev_name, STR_BUFFER_SIZE );
	// sanity check dev file info:
	{
		struct stat st;
		if (-1 == stat(dev_name, &st)) {
			private_camera_exit( camera );
			DEV_ERROR(
					"'%s': %s\n",
					dev_name, strerror(errno)
			);
			return RET_FAILURE;
		}
		// check if char device:
		if (
				!S_ISCHR(st.st_mode)
		) {
			private_camera_exit( camera );
			DEV_ERROR(
					"'%s' is no device\n",
					dev_name
			);
			return RET_FAILURE;
		}
	}
	camera->dev_file = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
	if( camera->dev_file == -1 ) {
		private_camera_exit( camera );
		DEV_ERROR(
				"error opening '%s': %d, %s\n",
				dev_name,
				errno,
				strerror(errno)
		);
		return RET_FAILURE;
	}
	{
		ret_t ret = negotiate_caps(
				camera
		);
		if( ret != RET_SUCCESS ) {
			private_camera_exit( camera );
			return ret;
		}
	}
	{
		ret_t ret = reset_cropping(
				camera
		);
		if( ret != RET_SUCCESS ) {
			private_camera_exit( camera );
			return ret;
		}
	}
	return RET_SUCCESS;
}

ret_t camera_exit(
		camera_t* camera
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_exit': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	return private_camera_exit(camera);
}

ret_t camera_list_formats(
		camera_t* camera,
		format_descriptions_t* formats
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_exit': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	unsigned int i=0;
	for( ; i<BUFFER_SIZE; i++ ) {
		struct v4l2_fmtdesc format_descr;
		memset( &format_descr, 0, sizeof(format_descr) );
		format_descr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format_descr.index = i;
		if (-1 == ioctl_helper(camera->dev_file, VIDIOC_ENUM_FMT, &format_descr)) {
			if( errno == EINVAL ) {
				break;
			}
			DEV_ERROR(
					"VIDIOC_ENUM_FMT error: %d, %s\n",
					errno,
					strerror( errno )
			);
			return RET_FAILURE;
		}
		formats->format_descrs[i] = format_descr;
	}
	formats->count = i;
	return RET_SUCCESS;
}

ret_t camera_set_format(
		camera_t* camera,
		// const unsigned int buffer_count,
		const unsigned int min_width,
		const unsigned int min_height,
		const __u32* requested_format,
		const bool format_required
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_exit': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	{
		ret_t ret = negotiate_format(
				camera,
				min_width, min_height,
				requested_format, format_required
		);
		if( ret != RET_SUCCESS ) {
			return ret;
		}
	}
	return RET_SUCCESS;
}

ret_t camera_init_buffer(
		camera_t* camera,
		const unsigned int buffer_count
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_exit': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	// request buffers:
	struct v4l2_requestbuffers reqbuf;
	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = buffer_count;

	if (-1 == ioctl_helper(camera->dev_file, VIDIOC_REQBUFS, &reqbuf)) {
		if (errno == EINVAL) {
			DEV_ERROR( "Video capturing or mmap-streaming is not supported\\n" );
			return RET_FAILURE;
		}
		else {
			DEV_ERROR( "VIDIOC_REQBUFS: error %d, %s\n",
					errno,
					strerror( errno )
			);
			return RET_FAILURE;
		}
	}
	if( reqbuf.count < buffer_count ) {
		DEV_ERROR( "failed to request %d buffers (only got %d buffers)\n",
				buffer_count,
				reqbuf.count
		);
		return RET_FAILURE;
	}
	camera->buffer_container.buffers = calloc(reqbuf.count, sizeof(*camera->buffer_container.buffers));
	assert( camera->buffer_container.buffers != NULL );

	// init camera struct
	camera->buffer_container.count = reqbuf.count;
	for(unsigned int i = 0; i < reqbuf.count; i++ ) {
		camera->buffer_container.buffers[i].data = NULL;
	}

	// acquire buffers:
	for(unsigned int i = 0; i < reqbuf.count; i++ ) {
		struct v4l2_buffer buffer;
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = reqbuf.type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (-1 == ioctl_helper(camera->dev_file, VIDIOC_QUERYBUF, &buffer)) {
			DEV_ERROR( "VIDIOC_QUERYBUF: error %d, %s\n",
					errno,
					strerror( errno )
			);
			return RET_FAILURE;
		}
		camera->buffer_container.buffers[i].size = buffer.length;
		camera->buffer_container.buffers[i].data = mmap(
				NULL,
				buffer.length,
				PROT_READ | PROT_WRITE, /* recommended */
				MAP_SHARED,             /* recommended */
				camera->dev_file,
				buffer.m.offset
		);

		if (MAP_FAILED == camera->buffer_container.buffers[i].data) {
			camera->buffer_container.buffers[i].data = NULL;
			camera->buffer_container.buffers[i].size = 0;
			DEV_ERROR( "mmap: error %d, %s\n",
					errno,
					strerror( errno )
			);
			return RET_FAILURE;
		}
	}	
	return RET_SUCCESS;
}

ret_t camera_stream_start(
		camera_t* camera
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_exit': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	if( camera->buffer_container.buffers == NULL ) {
		DEV_ERROR(
			"'camera_exit': camera is not ready\n"
		);
		return RET_FAILURE;
	}
	// "enqueue" all buffers, so
	// they can be filled by the camera
	for (unsigned int i = 0; i < camera->buffer_container.count; ++i)
	{
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof( buffer ));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (-1 == ioctl_helper(camera->dev_file, VIDIOC_QBUF, &buffer)) {
			DEV_ERROR(
					"VIDIOC_QBUF error: %d, %s\n",
					errno,
					strerror( errno )
			);
			return RET_FAILURE;
		}
	}
	// start streaming:
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl_helper(camera->dev_file, VIDIOC_STREAMON, &type)) {
		DEV_ERROR(
				"VIDIOC_STREAMON error: %d, %s\n",
				errno,
				strerror( errno )
		);
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

ret_t camera_stream_stop(
		camera_t* camera
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_exit': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	if( camera->buffer_container.buffers == NULL ) {
		DEV_ERROR(
			"'camera_exit': camera is not ready\n"
		);
		return RET_FAILURE;
	}
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl_helper(camera->dev_file, VIDIOC_STREAMOFF, &type)) {
		DEV_ERROR(
				"VIDIOC_STREAMOFF error: %d, %s\n",
				errno,
				strerror( errno )
		);
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

ret_t camera_get_frame(
		camera_t* camera,
		frame_buffer_t* buffer
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_exit': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	if( camera->buffer_container.buffers == NULL ) {
		DEV_ERROR(
			"'camera_exit': camera is not ready\n"
		);
		return RET_FAILURE;
	}
	// wait for the device to get ready:
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(camera->dev_file, &fds);
		int r = select(camera->dev_file + 1, &fds, NULL, NULL, NULL);
		if (-1 == r)
		{
			DEV_ERROR(
					"select error: %d, %s\n",
					errno,
					strerror(errno)
			);
		}
	}
	// request 1 frame:
	struct v4l2_buffer buffer_descr;
	memset(&buffer_descr, 0, sizeof(buffer_descr) );
	buffer_descr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer_descr.memory = V4L2_MEMORY_MMAP;
	if (-1 == ioctl_helper(camera->dev_file, VIDIOC_DQBUF, &buffer_descr)) {
		DEV_ERROR(
				"VIDIOC_DQBUF error: %d, %s\n",
				errno,
				strerror( errno )
		);
		return RET_FAILURE;
	}
	assert(buffer_descr.index < camera->buffer_container.count);
	buffer->data = camera->buffer_container.buffers[buffer_descr.index].data;
	buffer->size = buffer_descr.bytesused;
	buffer->index = buffer_descr.index;
	return RET_SUCCESS;
}

ret_t camera_return_frame(
		camera_t* camera,
		frame_buffer_t* buffer
)
{
	struct v4l2_buffer buffer_descr;
	memset(&buffer_descr, 0, sizeof(buffer_descr) );
	buffer_descr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer_descr.memory = V4L2_MEMORY_MMAP;
	buffer_descr.index = buffer->index;
	if (-1 == ioctl_helper(camera->dev_file, VIDIOC_QBUF, &buffer_descr)) {
		DEV_ERROR(
				"VIDIOC_DQBUF error: %d, %s\n",
				errno,
				strerror( errno )
		);
		return RET_FAILURE;
	}
	buffer->data = NULL;
	buffer->size = 0;
	return RET_SUCCESS;
}

char* camera_error() {
	return error_str;
}

void camera_reset_error() {
	strncpy( error_str, "", STR_BUFFER_SIZE );
}

/************************
 * private utils impl
*************************/

ret_t private_camera_exit( camera_t* camera ) {
	int ret = RET_SUCCESS;
	if( camera->dev_file != -1 ) {
		int temp = close( camera->dev_file );
		if( temp != 0 ) {
			DEV_ERROR(
					"'close' error %d, %s\n",
					errno,
					strerror(errno)
			);
			ret = RET_FAILURE;
		}
	}
	camera->dev_file = -1;
	if( camera->buffer_container.buffers != NULL ) {
		for(unsigned int i = 0; i < camera->buffer_container.count; i++ ) {
			if(  camera->buffer_container.buffers[i].data != NULL ) {
				int temp = munmap(
						camera->buffer_container.buffers[i].data,
						camera->buffer_container.buffers[i].size
				);
				if( temp == -1 ) {
					DEV_ERROR(
							"'munmap' error %d, %s\n",
							errno,
							strerror(errno)
					);
					ret = RET_FAILURE;
				}
				camera->buffer_container.buffers[i].data = NULL;
			}
		}
		FREE( camera->buffer_container.buffers );
		camera->buffer_container.count = 0;
	}
	return ret;
}

ret_t negotiate_caps(
		camera_t* camera
)
{
	struct v4l2_capability cap;
	if (-1 == ioctl_helper(camera->dev_file, VIDIOC_QUERYCAP, &cap))
	{
		if( EINVAL == errno ) {
			DEV_ERROR( "%s is no V4L2 device\n",
					camera->dev_name
			);
			return RET_FAILURE;
		}
		else
		{
			DEV_ERROR( "'VIDIOC_QUERYCAP' error %d, %s\n",
					errno,
					strerror(errno)
			)
			return RET_FAILURE;
		}
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		DEV_ERROR( "%s: is no video capture device\n",
				camera->dev_name
		);
		return RET_FAILURE;
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		DEV_ERROR( "%s: device does not support streaming\n",
				camera->dev_name
		);
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

ret_t reset_cropping(
		camera_t* camera
)
{
	struct v4l2_cropcap cropcap;
	memset( &cropcap, 0, sizeof(cropcap) );
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	{
		int ret = ioctl_helper( camera->dev_file, VIDIOC_CROPCAP, &cropcap );
		// cropping is not supported:
		if( errno == EINVAL ) {
			return RET_SUCCESS;
		}
		if( ret == -1 ) {
			DEV_ERROR( "'VIDIOC_CROPCAP' error %d, %s\n",
					errno,
					strerror(errno)
			);
			return RET_FAILURE;
		}
	}
	struct v4l2_crop crop;
	memset( &crop, 0, sizeof(crop) );
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c = cropcap.defrect;

	// try to reset cropping to default:
	if( -1 == ioctl_helper(camera->dev_file, VIDIOC_S_CROP, &crop) && errno != EINVAL ) {
		// ignore errors...
	}
	return RET_SUCCESS;
}

ret_t negotiate_format(
		camera_t* camera,
		const unsigned int min_width,
		const unsigned int min_height,
		const __u32* requested_format,
		const bool format_required
)
{
	struct v4l2_pix_format current_format;
	// Query Format:
	{
		struct v4l2_format format;
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if( -1 == ioctl_helper( camera->dev_file, VIDIOC_G_FMT, &format ) ) {
			DEV_ERROR( "'VIDIOC_G_FMT' error %d, %s\n",
					errno,
					strerror(errno)
			)
			return RET_FAILURE;
		}
		current_format = format.fmt.pix;
	}
	// Check format
	if(
			current_format.width >= min_width
			&& current_format.height >= min_height
			&& (
				requested_format == NULL
			)
	) {
		// format fulfills requirements:
		return RET_SUCCESS;
	}
	// try to force format:
	{
		struct v4l2_format format;
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(
				current_format.width < min_width
				|| current_format.height < min_height
		) {
			format.fmt.pix.width = min_width;
			format.fmt.pix.height = min_height;
		}
		if( requested_format != NULL ) {
			format.fmt.pix.pixelformat = (*requested_format);
		}
		if( -1 == ioctl_helper( camera->dev_file, VIDIOC_S_FMT, &format ) ) {
			DEV_ERROR( "'VIDIOC_S_FMT' error %d, %s\n",
					errno,
					strerror(errno)
			)
			return RET_FAILURE;
		}
		current_format = format.fmt.pix;
	}
	// fail, if we didn't succeed negotiating
	// the requirements
	if(
			current_format.width < min_width
			|| current_format.height < min_height
	) {
		DEV_ERROR(
				"device does not support required size: required: %dx%d, supported: %dx%d",
				min_height, min_height,
				current_format.width, current_format.height
		);
		return RET_FAILURE;
	}
	if(
			format_required
			&& current_format.pixelformat != (*requested_format)
	) {
		DEV_ERROR(
				"device does not support required format: required: %d, supported: %d",
				current_format.pixelformat,
				(*requested_format)
		);
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

int ioctl_helper(int fh, unsigned int request, void *arg) {
	int r;
	do 
	{
		r = ioctl(fh, request, arg);

	} while (-1 == r && EINTR == errno);
	return r;
}
