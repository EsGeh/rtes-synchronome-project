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

ret_t get_format(
		camera_t* camera
);

ret_t negotiate_format(
		camera_t* camera,
		const pixel_format_t format,
		const format_constraint_t format_constraint,
		const frame_size_t frame_size,
		frame_size_constraint_t frame_size_constraint,
		const frame_interval_t frame_interval,
		const frame_interval_constraint_t frame_interval_constraint
);

/************************
 * API implementation
*************************/

void camera_zero( camera_t* camera )
{
	(*camera) = (camera_t){
		.dev_name = "",
		.dev_file = -1,
		.buffer_container = {
			.buffers = NULL,
			.count = 0
		},
		.format = {
			.width = 0,
			.height = 0,
		}
	};
}

ret_t camera_init(
		camera_t* camera,
		const char* dev_name
)
{
	assert( camera != NULL );
	// init data:
	camera_zero( camera );
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
	{
		// set camera.format
		// without changing camera settings:
		frame_size_t frame_size = { 0, 0 };
		ret_t ret = negotiate_format(
				camera,
				0, FORMAT_ANY,
				frame_size, FRAME_SIZE_ANY,
				(frame_interval_t){ 0, 0}, FRAME_INTERVAL_ANY
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
			"'camera_list_formats': camera is uninitialized\n"
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

// precondition: camera must be in state "initialized" or better
ret_t camera_list_sizes(
		camera_t* camera,
		const pixel_format_t pixel_format,
		size_descrs_t* frame_size_descrs
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_list_sizes': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	unsigned int i=0;
	for( ; i<BUFFER_SIZE; i++ ) {
		struct v4l2_frmsizeenum frame_size_descr;
		{
			memset( &frame_size_descr, 0, sizeof(frame_size_descr) );
			frame_size_descr.index = i;
			frame_size_descr.pixel_format = pixel_format;
		}
		if (-1 == ioctl_helper(camera->dev_file, VIDIOC_ENUM_FRAMESIZES, &frame_size_descr)) {
			if( errno == EINVAL ) {
				break;
			}
			DEV_ERROR(
					"VIDIOC_ENUM_FRAMESIZES error: %d, %s\n",
					errno,
					strerror( errno )
			);
			return RET_FAILURE;
		}
		frame_size_descrs->frame_size_descrs[i] = frame_size_descr;;
	}
	frame_size_descrs->count = i;
	return RET_SUCCESS;
}

// precondition: camera must be in state "initialized" or better
ret_t camera_list_frame_intervals(
		camera_t* camera,
		const pixel_format_t pixel_format,
		const frame_size_t frame_size,
		frame_interval_descrs_t* frame_interval_descrs
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_list_frame_intervals': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	unsigned int i=0;
	for( ; i<BUFFER_SIZE; i++ ) {
		struct v4l2_frmivalenum descr;
		memset( &descr, 0, sizeof(descr) );
		{
			descr.index = i;
			descr.pixel_format = pixel_format;
			descr.width = frame_size.width;
			descr.height = frame_size.height;
		}
		if (-1 == ioctl_helper(camera->dev_file, VIDIOC_ENUM_FRAMEINTERVALS, &descr)) {
			if( errno == EINVAL ) {
				break;
			}
			DEV_ERROR(
					"VIDIOC_ENUM_FRAMEINTERVALS error: %d, %s\n",
					errno,
					strerror( errno )
			);
			return RET_FAILURE;
		}
		frame_interval_descrs->descrs[i] = descr;;
	}
	frame_interval_descrs->count = i;
	return RET_SUCCESS;
}

ret_t camera_list_modes(
		camera_t* camera,
		camera_mode_descrs_t* modes
)
{
	modes->count = 0;
	camera_mode_descr_t mode;
	format_descriptions_t format_descriptions;
	if( RET_SUCCESS != camera_list_formats(
				camera,
				&format_descriptions
	)) {
		return RET_FAILURE;
	}
	for( uint format_index=0; format_index< format_descriptions.count; format_index++ ) {
		const pixel_format_descr_t* format_descr = &format_descriptions.format_descrs[format_index];
		mode.pixel_format_descr = *format_descr;
		size_descrs_t size_descriptions;
		if( RET_SUCCESS != camera_list_sizes(
					camera,
					format_descr->pixelformat,
					&size_descriptions
		)) {
			return RET_FAILURE;
		}
		for( uint size_index=0; size_index< size_descriptions.count; size_index++ ) {
			const frame_size_descr_t* size_descr = &size_descriptions.frame_size_descrs[size_index];
			if( size_descr->type == V4L2_FRMSIZE_TYPE_DISCRETE ) {
				mode.frame_size_descr = *size_descr;
			}
			else {
				// non-discrete modes not implemented
				continue;
			}
			frame_interval_descrs_t frame_interval_descrs;
			if( RET_SUCCESS != camera_list_frame_intervals(
						camera,
						mode.pixel_format_descr.pixelformat,
						(frame_size_t){
							.width = mode.frame_size_descr.discrete.width,
							.height = mode.frame_size_descr.discrete.height,
						},
						&frame_interval_descrs
			)) {
				return RET_FAILURE;
			}
			for( uint frame_interval_index=0; frame_interval_index< frame_interval_descrs.count; frame_interval_index++ ) {
				const frame_interval_descr_t* frame_interval_descr = &frame_interval_descrs.descrs[frame_interval_index];
				if( frame_interval_descr->type == V4L2_FRMIVAL_TYPE_DISCRETE ) {
					mode.frame_interval_descr = *frame_interval_descr;
				}
				else {
					// non-discrete frame_intervals not implemented
					continue;
				}
				modes->mode_descrs[modes->count] = mode;
				modes->count ++;
			}
		}
	}
	return RET_SUCCESS;
}

ret_t camera_set_format(
		camera_t* camera,
		const pixel_format_t requested_format,
		const format_constraint_t format_constraint,
		const frame_size_t frame_size,
		const frame_size_constraint_t frame_size_constraint,
		const frame_interval_t frame_interval,
		const frame_interval_constraint_t frame_interval_constraint
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_set_format': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	{
		ret_t ret = negotiate_format(
				camera,
				requested_format, format_constraint,
				frame_size, frame_size_constraint,
				frame_interval, frame_interval_constraint
		);
		if( ret != RET_SUCCESS ) {
			return ret;
		}
	}
	return RET_SUCCESS;
}

// precondition: camera must be in state "initialized"
ret_t camera_get_mode(
		camera_t* camera,
		camera_mode_t* mode
)
{
	assert( camera != NULL );
	if( camera->dev_file == -1 ) {
		DEV_ERROR(
			"'camera_get_mode': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	mode->pixel_format = camera->format.pixelformat;
	mode->frame_size = (frame_size_t){
		.width = camera->format.width,
		.height = camera->format.height,
	};
	mode->frame_interval = camera->frame_interval;
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
			"'camera_init_buffer': camera is uninitialized\n"
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
			"'camera_stream_start': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	if( camera->buffer_container.buffers == NULL ) {
		DEV_ERROR(
			"'camera_stream_start': camera is not ready\n"
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
			"'camera_stream_stop': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	if( camera->buffer_container.buffers == NULL ) {
		DEV_ERROR(
			"'camera_stream_stop': camera is not ready\n"
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
			"'camera_get_frame': camera is uninitialized\n"
		);
		return RET_FAILURE;
	}
	if( camera->buffer_container.buffers == NULL ) {
		DEV_ERROR(
			"'camera_get_frame': camera is not ready\n"
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

bool camera_mode_descr_equal(
		const camera_mode_descr_t* mode1,
		const camera_mode_descr_t* mode2
)
{
	if( mode1->pixel_format_descr.pixelformat != mode2->pixel_format_descr.pixelformat ) {
		return false;
	}
	if(
			mode1->frame_size_descr.discrete.width != mode2->frame_size_descr.discrete.width
			|| mode1->frame_size_descr.discrete.height != mode2->frame_size_descr.discrete.height
	) {
		return false;
	}
	if(
			mode1->frame_interval_descr.discrete.numerator != mode2->frame_interval_descr.discrete.numerator
			|| mode1->frame_interval_descr.discrete.denominator != mode2->frame_interval_descr.discrete.denominator
	) {
		return false;
	}
	return true;
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

ret_t get_format(
		camera_t* camera
)
{
	struct v4l2_format format;
	if( -1 == ioctl_helper( camera->dev_file, VIDIOC_G_FMT, &format ) ) {
		DEV_ERROR( "'VIDIOC_G_FMT' error %d, %s\n",
				errno,
				strerror(errno)
		)
		return RET_FAILURE;
	}
	camera->format = format.fmt.pix;
	return RET_SUCCESS;
}

ret_t negotiate_format(
		camera_t* camera,
		const pixel_format_t requested_format,
		const format_constraint_t format_constraint,
		const frame_size_t frame_size,
		const frame_size_constraint_t frame_size_constraint,
		const frame_interval_t frame_interval,
		const frame_interval_constraint_t frame_interval_constraint
)
{
	struct v4l2_format format;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	// Query current format:
	{
		if( -1 == ioctl_helper( camera->dev_file, VIDIOC_G_FMT, &format ) ) {
			DEV_ERROR( "'VIDIOC_G_FMT' error %d, %s\n",
					errno,
					strerror(errno)
			)
			return RET_FAILURE;
		}
	}
	// Query frame_interval:
	struct v4l2_streamparm streamparm;
	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	{
		if( -1 == ioctl_helper(camera->dev_file, VIDIOC_G_PARM, &streamparm) )
		{
			DEV_ERROR( "'VIDIOC_G_PARM' error %d, %s\n",
					errno,
					strerror(errno)
			)
			return RET_FAILURE;
		}
	}
	// Check format
	if(
			(format_constraint == FORMAT_ANY || format.fmt.pix.pixelformat == requested_format)
			&& (frame_size_constraint == FRAME_SIZE_ANY
				|| (format.fmt.pix.width == frame_size.width && format.fmt.pix.height == frame_size.height)
			)
			&& (
				frame_interval_constraint == FRAME_INTERVAL_ANY
				|| (
					frame_interval.numerator == streamparm.parm.capture.timeperframe.numerator
					&& frame_interval.denominator == streamparm.parm.capture.timeperframe.denominator
				)
			)
	) {
		// format fulfills requirements:
		camera->format = format.fmt.pix;
		camera->frame_interval = streamparm.parm.capture.timeperframe;
		return RET_SUCCESS;
	}
	// try to force format:
	{
		if(
				format_constraint != FORMAT_ANY
		) {
			format.fmt.pix.pixelformat = requested_format;
		}
		if( frame_size_constraint != FRAME_SIZE_ANY ) {
			format.fmt.pix.width = frame_size.width;
			format.fmt.pix.height = frame_size.height;
		}
		if( -1 == ioctl_helper( camera->dev_file, VIDIOC_S_FMT, &format ) ) {
			DEV_ERROR( "'VIDIOC_S_FMT' error %d, %s\n",
					errno,
					strerror(errno)
			)
			return RET_FAILURE;
		}
		// set framerate:
		if(
				frame_interval_constraint != FRAME_INTERVAL_ANY
		) {
			streamparm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
			streamparm.parm.capture.timeperframe = frame_interval;
			if( -1 == ioctl_helper(camera->dev_file, VIDIOC_S_PARM, &streamparm) )
			{
				DEV_ERROR( "'VIDIOC_S_PARM' error %d, %s\n",
						errno,
						strerror(errno)
				)
				return RET_FAILURE;
			}
		}
	}
	camera->format = format.fmt.pix;
	camera->frame_interval = streamparm.parm.capture.timeperframe;
	// fail, if we didn't succeed negotiating
	// the requirements
	if(
			format_constraint == FORMAT_EXACT
			&& format.fmt.pix.pixelformat != requested_format
	) {
		DEV_ERROR(
				"device does not support required format: required: %d, supported: %d",
				format.fmt.pix.pixelformat,
				requested_format
		);
		return RET_FAILURE;
	}
	if(
			frame_size_constraint == FRAME_SIZE_EXACT
			&& (
				format.fmt.pix.width != frame_size.width
				|| format.fmt.pix.height != frame_size.height
			)
	) {
		DEV_ERROR(
				"device does not support required size: required: %dx%d, supported: %dx%d",
				frame_size.width, frame_size.height,
				format.fmt.pix.width, format.fmt.pix.height
		);
		return RET_FAILURE;
	}
	if(
			frame_interval_constraint == FRAME_INTERVAL_EXACT
			&& (
				streamparm.parm.capture.timeperframe.numerator != frame_interval.numerator
				|| streamparm.parm.capture.timeperframe.denominator != frame_interval.denominator
			)
	) {
		DEV_ERROR(
				"device does not support frame interval: required: %f, supported: %f",
				(float )frame_interval.numerator / (float )frame_interval.denominator,
				(float )streamparm.parm.capture.timeperframe.numerator / (float )streamparm.parm.capture.timeperframe.denominator 
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
