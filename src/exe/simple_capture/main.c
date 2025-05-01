#include "main.h"

#include "lib/image.h"
#include "lib/output.h"

#include <unistd.h>


#define CAMERA_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("%s\n", camera_error() ); \
		return EXIT_FAILURE; \
	} \
}

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		return EXIT_FAILURE; \
	} \
}

typedef struct {
	camera_t camera;
	byte_t* rgb_buffer;
	uint rgb_buffer_size;
} capture_data_t;

ret_t capture_exit(
		capture_data_t* data
);

ret_t capture_run(
		capture_args_t* args,
		capture_data_t* data
);

ret_t capture_single(capture_args_t* args)
{
	capture_data_t data;
	API_RUN( capture_run(args, &data) );
	API_RUN( capture_exit(&data) );
	return RET_SUCCESS;
}

ret_t capture_exit(capture_data_t* data)
{
	FREE( data->rgb_buffer );
	return RET_SUCCESS;
}

ret_t capture_run(
		capture_args_t* args,
		capture_data_t* data
)
{
	CAMERA_RUN(camera_init(
			&data->camera,
			args->dev_name
	));
	CAMERA_RUN(camera_set_mode(
			&data->camera,
			V4L2_PIX_FMT_YUYV, FORMAT_EXACT,
			args->size, FRAME_SIZE_EXACT,
			args->acq_interval, FRAME_INTERVAL_ANY
	));
	{
		camera_mode_t mode;
		CAMERA_RUN(camera_get_mode(
					&data->camera,
					&mode
		));
		log_info( "- %ux%u, %f\n",
				mode.frame_size.width,
				mode.frame_size.height,
				(float )mode.frame_interval.numerator
					/ (float )mode.frame_interval.denominator
		);
	}
	CAMERA_RUN( camera_init_buffer(
			&data->camera,
			1
	));
	CAMERA_RUN( camera_stream_start( &data->camera ));
	sleep( 1 );
	{
		data->rgb_buffer = NULL;
		// allocate image buffer:
		data->rgb_buffer_size = image_rgb_size(
				data->camera.format.width,
				data->camera.format.height
		);
		CALLOC(data->rgb_buffer, data->rgb_buffer_size, 1);
		frame_buffer_t frame;
		CAMERA_RUN( camera_get_frame( &data->camera, &frame ));

		// measure 'image_convert_to_rgb''
		{
			API_RUN( image_convert_to_rgb(
					data->camera.format,
					frame.data,
					data->rgb_buffer,
					data->rgb_buffer_size
			));
		}
		CAMERA_RUN( camera_return_frame( &data->camera, &frame));
		CAMERA_RUN( camera_stream_stop( &data->camera ));
		// run 'image_save_ppm'
		{
			char filename[STR_BUFFER_SIZE];
			snprintf( filename, STR_BUFFER_SIZE, "%s/capture.ppm",
					args->output_dir
			);
			API_RUN( image_save_ppm(
					filename,
					"captured from data->camera",
					data->rgb_buffer,
					data->rgb_buffer_size,
					data->camera.format.width,
					data->camera.format.height
			));
		}
	}
	CAMERA_RUN(camera_exit(
			&data->camera
	));
	return RET_SUCCESS;
}
