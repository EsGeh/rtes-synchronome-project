#include "frame_acq.h"

#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"

#include <stdio.h>
#include <string.h>


int set_camera_format(
		camera_t* camera,
		const uint width,
		const uint height
);

#define CAMERA_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		log_error("%s\n", camera_error() ); \
		return RET_FAILURE; \
	} \
}

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		return RET_FAILURE; \
	} \
}

ret_t frame_acq_init(
		camera_t* camera,
		const char* dev_name,
		const uint buffer_size,
		const uint width,
		const uint height
)
{
	CAMERA_RUN(camera_init(
			camera,
			dev_name
	));
	CAMERA_RUN(set_camera_format(
				camera,
				width,
				height
	));
	CAMERA_RUN( camera_init_buffer(
			camera,
			buffer_size
	));
	CAMERA_RUN( camera_stream_start( camera ));
	return RET_SUCCESS;
}

ret_t frame_acq_exit(
	camera_t* camera
)
{
	CAMERA_RUN( camera_stream_stop( camera ));
	return RET_SUCCESS;
}

ret_t frame_acq_run(
	camera_t* camera,
	sem_t* sem,
	bool* stop,
	byte_t* rgb_buffer,
	uint rgb_buffer_size,
	const char* output_dir
)
{
	frame_buffer_t frame;
	char output_path[STR_BUFFER_SIZE];
	int counter = 0;
	while( true ) {
		if( sem_wait( sem ) ) {
			perror("sem_wait");
			return RET_FAILURE;
		}
		if( (*stop) ) {
			break;
		}
		CAMERA_RUN( camera_get_frame( camera, &frame ));

		API_RUN( image_convert_to_rgb(
				camera->format,
				frame.data,
				rgb_buffer,
				rgb_buffer_size
		));
		snprintf(output_path, STR_BUFFER_SIZE, "%s/image%04u.ppm",
				output_dir,
				counter
		);
		log_info( "writing frame: '%s'\n", output_path );
		API_RUN( image_save_ppm(
				output_path,
				"captured from camera",
				rgb_buffer,
				rgb_buffer_size,
				camera->format.width,
				camera->format.height
		));
		CAMERA_RUN( camera_return_frame( camera, &frame));
		++counter;
	}
	return RET_SUCCESS;
}


int set_camera_format(
		camera_t* camera,
		const uint width,
		const uint height
)
{
	format_descriptions_t format_descriptions;
	if( RET_SUCCESS != camera_list_formats(
				camera,
				&format_descriptions
	) )
		return RET_FAILURE;
	__u32 required_format;
	{
		bool format_found = false;
		for( unsigned int i=0; i< format_descriptions.count; i++ ) {
			if( strstr( (char* )format_descriptions.format_descrs[i].description, "YUYV" ) != NULL ) {
				format_found = true;
				required_format = format_descriptions.format_descrs[i].pixelformat;
				break;
			}
		}
		if( !format_found ) {
			log_error( "camera does not support expected format!\n");
			return RET_FAILURE;
		}
	}
	frame_size_t required_frame_size = { width, height };
	if( RET_SUCCESS != camera_set_mode(
				camera,
				required_format, FORMAT_EXACT,
				required_frame_size, FRAME_SIZE_EXACT,
				(frame_interval_t){0, 0}, FRAME_INTERVAL_ANY
	)) {
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}
