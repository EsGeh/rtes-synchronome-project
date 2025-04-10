#include "camera.h"
#include "global.h"
#include "output.h"

#include <linux/videodev2.h>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>


#define DEV_ERROR( FMT_STR, ...) { \
	int ret = snprintf( error_str, STR_BUFFER_SIZE, \
			FMT_STR, ## __VA_ARGS__ \
	); \
	if( ret >= STR_BUFFER_SIZE ) { \
		strncpy( &error_str[STR_BUFFER_SIZE-1-3], "...", 4); \
	} \
	return RET_FAILURE; \
}

static char error_str[STR_BUFFER_SIZE] = "";

int ioctl_helper(int fh, unsigned int request, void *arg);

ret_t negotiate_caps(
		camera_t* camera
);

ret_t negotiate_caps(
		camera_t* camera
);

ret_t camera_init(
		camera_t* camera,
		const char* dev_name
)
{
	if( camera == NULL ) {
		DEV_ERROR( "'camera_init' camera == NULL\n" );
		return RET_FAILURE;
	}
	struct stat st;
	if (-1 == stat(dev_name, &st)) {
		DEV_ERROR(
				"'%s': %s\n",
				dev_name, strerror(errno)
		);
		return RET_FAILURE;
	}
	if (!S_ISCHR(st.st_mode)) {
		DEV_ERROR(
				"'%s' is no device\n",
				dev_name
		);
		return RET_FAILURE;
	}
	camera->dev_file = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
	if( camera->dev_file == -1 ) {
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
			return ret;
		}
	}
	return RET_SUCCESS;
}

ret_t camera_exit(
		camera_t* camera
)
{
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
			return RET_FAILURE;
		}
	}
	return RET_SUCCESS;
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
			)
		}
		else
		{
			DEV_ERROR( "'VIDIOC_QUERYCAP' error %d, %s\n",
					errno,
					strerror(errno)
			)
		}
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		DEV_ERROR( "%s: is no video capture device\n",
				camera->dev_name
		)
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		DEV_ERROR( "%s: device does not support streaming\n",
				camera->dev_name
		);
	}
	return RET_SUCCESS;
}

char* camera_error() {
	return error_str;
}

void camera_reset_error() {
	strncpy( error_str, "", STR_BUFFER_SIZE );
}

int ioctl_helper(int fh, unsigned int request, void *arg)
{
	int r;
	do 
	{
		r = ioctl(fh, request, arg);

	} while (-1 == r && EINTR == errno);
	return r;
}
