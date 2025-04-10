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


const unsigned int buffer_count = 10;

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

ret_t init_frame_buffer(
		camera_t* camera
);

/************************
 * API implementation
*************************/

ret_t camera_init(
		camera_t* camera,
		const char* dev_name
)
{
	if( camera == NULL ) {
		DEV_ERROR( "'camera_init' camera == NULL\n" );
		return RET_FAILURE;
	}
	// init data:
	(*camera) = (camera_t){
		.dev_file = -1,
		.buffer_container = {
			.buffers = NULL,
			.count = 0,
		},
	};
	strncpy( camera->dev_name, dev_name, STR_BUFFER_SIZE );
	struct stat st;
	if (-1 == stat(dev_name, &st)) {
		private_camera_exit( camera );
		DEV_ERROR(
				"'%s': %s\n",
				dev_name, strerror(errno)
		);
		return RET_FAILURE;
	}
	if (!S_ISCHR(st.st_mode)) {
		private_camera_exit( camera );
		DEV_ERROR(
				"'%s' is no device\n",
				dev_name
		);
		return RET_FAILURE;
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
		ret_t ret = init_frame_buffer(
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
	ret_t ret = RET_SUCCESS;
	if( camera == NULL ) {
		DEV_ERROR(
			"'camera_exit' camera == NULL\n"
		);
		return RET_FAILURE;
	}
	{
		int ret = close( camera->dev_file );
		if( ret != 0 ) {
			DEV_ERROR(
					"'close' error %d, %s\n",
					errno,
					strerror(errno)
			);
			ret = RET_FAILURE;
		}
		camera->dev_file = -1;
	}
	if( camera->buffer_container.buffers != NULL ) {
		for(unsigned int i = 0; i < camera->buffer_container.count; i++ ) {
			if(  camera->buffer_container.buffers[i].data != NULL ) {
				int ret = munmap(
						camera->buffer_container.buffers[i].data,
						camera->buffer_container.buffers[i].size
				);
				if( ret == -1 ) {
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
			ret = RET_FAILURE;
		}
	}
	camera->dev_file = -1;
	if( camera->buffer_container.buffers != NULL ) {
		for(unsigned int i = 0; i < camera->buffer_container.count; i++ ) {
			if(  camera->buffer_container.buffers[i].data != NULL ) {
				munmap(
						camera->buffer_container.buffers[i].data,
						camera->buffer_container.buffers[i].size
				);
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

ret_t init_frame_buffer(
		camera_t* camera
)
{
	// printf( "init_frame_buffer\n" );
	// request buffers:
	struct v4l2_requestbuffers reqbuf;
	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = buffer_count;

	if (-1 == ioctl (camera->dev_file, VIDIOC_REQBUFS, &reqbuf)) {
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

		if (-1 == ioctl (camera->dev_file, VIDIOC_QUERYBUF, &buffer)) {
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

int ioctl_helper(int fh, unsigned int request, void *arg) {
	int r;
	do 
	{
		r = ioctl(fh, request, arg);

	} while (-1 == r && EINTR == errno);
	return r;
}
