#include "lib/camera.h"
#include "lib/image.h"
#include "lib/output.h"
#include "lib/time.h"
#include "lib/global.h"

#include <bits/types/struct_timeval.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h> // <- work with files
#include <string.h>
#include <sys/syslog.h>


/********************
 * Types
********************/

typedef struct {
	camera_t camera;
	byte_t* rgb_buffer;
	uint rgb_buffer_size;
} runtime_data_t;

/********************
 * Global Constants
********************/

const char short_options[] = "h";
const  struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ 0,0,0,0 },
};
const char dev_name[] = "/dev/video0";
const uint buffer_count = 30;
const uint iterations_capture = 10;
const uint iterations_convert = 50;
const uint iterations_save_img = 10;

/********************
 * Function Decls
********************/

ret_t program_run( runtime_data_t* data );
void program_exit( runtime_data_t* data );

ret_t run_capture(
		runtime_data_t* data,
		const camera_mode_descr_t* dest_mode
);
ret_t print_all_camera_modes(
		runtime_data_t* data,
		camera_mode_descrs_t* modes
);
ret_t print_platform_info();


void print_cmd_line_info(
		// int argc,
		char* argv[]
);
int parse_cmd_line_args(
		int argc,
		char* argv[]
);

/********************
 * Main
********************/

#define CAMERA_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("ERROR: %s\n", camera_error() ); \
		return EXIT_FAILURE; \
	} \
}

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		return EXIT_FAILURE; \
	} \
}

int main(
		int argc,
		char* argv[]
) {
	// parse cmd line args:
	{
		int ret = parse_cmd_line_args(
				argc, argv
		);
		// --help:
		if( ret == -1 ) {
			print_cmd_line_info( /*argc,*/ argv );
			return EXIT_SUCCESS;
		}
		// error parsing cmd line args:
		else if( ret != 0 ) {
			print_cmd_line_info( /*argc,*/ argv );
			return EXIT_FAILURE;
		}
	}
	log_init(
			argv[0],
			true, false,
			true, true
	);
	time_init();
	// run:
	{
		runtime_data_t data = {
			.rgb_buffer = NULL,
			.rgb_buffer_size = 0,
		};
		camera_zero( &data.camera );
		ret_t ret = program_run( &data );
		program_exit( &data );
		log_exit();

		if( RET_SUCCESS != ret ) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/********************
 * Function Defs
********************/

#define TIME_TEST_INIT(label) \
		USEC CAT(label,_max) = 0; \
		USEC CAT(label,_acc) = 0;

#define TIME_TEST_MAX_US(label) (CAT(label,_max))
#define TIME_TEST_ACC_US(label) (CAT(label,_acc))

#define TIME_TEST_MAX_S(label) ((float)CAT(label,_max) / 1000.0 / 1000.0)
#define TIME_TEST_ACC_S(label) ((float )CAT(label,_acc) / 1000.0 / 1000.0)
#define TIME_TEST_AVG_S(label,iterations) (TIME_TEST_ACC_S(label) / iterations)

#define TIME_TEST_START(label) timeval_t (label ## _t0) = time_measure_current_time();
#define TIME_TEST_STOP(label) \
	timeval_t (label ## _t1) = time_measure_current_time(); \
	timeval_t (label ## _delta); \
	time_delta( &(label ## _t1), &(label ## _t0), &(label ## _delta) ); \
	USEC (label ## _delta_us) = time_us_from_timespec( &(label ## _delta) ); \
	CAT(label,_max) = MAX( CAT(label,_max), CAT(label,_delta_us) ); \
	CAT(label,_acc) += CAT(label,_delta_us);

#define TIME_TEST_PRINT(label,iterations) \
		log_info( "\t- " #label  ":" ); \
		log_info( " max: %f, avg: %fs (%fs / %u)\n", \
				TIME_TEST_MAX_S(label), \
				TIME_TEST_AVG_S(label,iterations), \
				TIME_TEST_ACC_S(label), \
				iterations_capture \
		);

ret_t program_run( runtime_data_t* data )
{
	log_info( "----------------------------------\n" );
	log_info("system info:\n" );
	API_RUN( print_platform_info() );
	// 
	camera_mode_descrs_t modes;
	log_info( "----------------------------------\n" );
	log_info( "camera supported formats:\n" );
	API_RUN( print_all_camera_modes(
				data,
				&modes
	));
	// filter modes:
	camera_mode_descrs_t selected_modes = (camera_mode_descrs_t){
		.count = 0,
	};
	for( uint i=0; i<modes.count; i++ ) {
		const camera_mode_descr_t* mode = &modes.mode_descrs[i];
		// remove doubles:
		/*
		{
			bool skip = false;
			for( uint selected_index=0; selected_index<selected_modes.count; selected_index++ ) {
				if( camera_mode_descr_equal( mode, &selected_modes.mode_descrs[selected_index] ) ) {
					skip = true;
				}
			}
			if( skip ) continue;
		}
		*/
		// only select YUYV formats:
		if( strstr( (char* )mode->pixel_format_descr.description, "YUYV" ) != NULL ) {
			selected_modes.mode_descrs[selected_modes.count] = *mode;
			selected_modes.count ++;
		}
	}
	log_info( "----------------------------------\n" );
	log_info( "Time measurements:\n" );
	for( uint i=0; i<selected_modes.count; i++ ) {
		const camera_mode_descr_t* dest_mode = &selected_modes.mode_descrs[i];
		API_RUN( run_capture(
					data,
					dest_mode
		));
	}
	return RET_SUCCESS;
}

ret_t run_capture(
		runtime_data_t* data,
		const camera_mode_descr_t* dest_mode
)
{
	CAMERA_RUN(camera_init(
			&data->camera,
			dev_name
	));
	CAMERA_RUN(camera_set_mode(
			&data->camera,
			dest_mode->pixel_format_descr.pixelformat, FORMAT_EXACT,
			(frame_size_t){
				.width = dest_mode->frame_size_descr.discrete.width,
				.height = dest_mode->frame_size_descr.discrete.height,
			}, FRAME_SIZE_EXACT,
			dest_mode->frame_interval_descr.discrete, FRAME_INTERVAL_EXACT
	));
	{
		camera_mode_t mode;
		CAMERA_RUN(camera_get_mode(
					&data->camera,
					&mode
		));
		log_info( "- %ux%u, %f\n",
				// (char* )(mode->pixel_format_descr.description),
				mode.frame_size.width,
				mode.frame_size.height,
				(float )mode.frame_interval.numerator
					/ (float )mode.frame_interval.denominator
		);
	}
	{
		CAMERA_RUN( camera_init_buffer(
				&data->camera,
				buffer_count
		));
		CAMERA_RUN( camera_stream_start( &data->camera ));
		frame_buffer_t frame;
		/* First frame may be used by camera
		 * for adjustments and can
		 * take much longer.
		 * Therefore: skip and don't measure first frame!
		 */
		CAMERA_RUN( camera_get_frame(&data->camera, &frame) );
		CAMERA_RUN( camera_return_frame( &data->camera, &frame));
		TIME_TEST_INIT(camera_get_frame);
		TIME_TEST_INIT(camera_return_frame);
		for( uint i=0; i<iterations_capture; ++i ) {
			TIME_TEST_START(camera_get_frame);
			CAMERA_RUN( camera_get_frame(&data->camera, &frame) );
			TIME_TEST_STOP(camera_get_frame);
			TIME_TEST_START(camera_return_frame);
			CAMERA_RUN( camera_return_frame( &data->camera, &frame));
			TIME_TEST_STOP(camera_return_frame);
		}
		TIME_TEST_PRINT(camera_get_frame,iterations_capture)
		TIME_TEST_PRINT(camera_return_frame,iterations_capture)
	}
	{
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
			TIME_TEST_INIT( image_convert_to_rgb )
			for( uint i=0; i<iterations_convert; ++i ) {
				TIME_TEST_START(image_convert_to_rgb)
				API_RUN( image_convert_to_rgb(
						data->camera.format,
						frame.data,
						data->rgb_buffer,
						data->rgb_buffer_size
				));
				TIME_TEST_STOP( image_convert_to_rgb )
			}
			CAMERA_RUN( camera_return_frame( &data->camera, &frame));
			CAMERA_RUN( camera_stream_stop( &data->camera ));
			TIME_TEST_PRINT(image_convert_to_rgb,iterations_convert)
		}
		// measure 'image_save_ppm'
		{
			TIME_TEST_INIT( image_save_ppm )
			char filename[STR_BUFFER_SIZE];
			for( uint i=0; i<iterations_save_img; ++i ) {
				snprintf( filename, STR_BUFFER_SIZE, "local/img/test%u.ppm", i );
				TIME_TEST_START(image_save_ppm)
				API_RUN( image_save_ppm(
						filename,
						"captured from camera",
						data->rgb_buffer,
						data->rgb_buffer_size,
						data->camera.format.width,
						data->camera.format.height
				));
				TIME_TEST_STOP(image_save_ppm)
			}
			TIME_TEST_PRINT(image_save_ppm,iterations_save_img)
		}
		FREE( data->rgb_buffer );
	}
	CAMERA_RUN(camera_exit(
			&data->camera
	));
	return RET_SUCCESS;
}

ret_t print_all_camera_modes(
		runtime_data_t* data,
		camera_mode_descrs_t* modes
)
{
	CAMERA_RUN(camera_init(
			&data->camera,
			dev_name
	));
	{
		size_descrs_t sizes;
		CAMERA_RUN(camera_list_sizes(
				&data->camera,
				V4L2_PIX_FMT_YUYV,
				&sizes
		));
	}
	CAMERA_RUN(camera_list_modes(
			&data->camera,
			modes
	));
	for( uint index=0; index< modes->count; index++ ) {
		const camera_mode_descr_t* mode = &modes->mode_descrs[index];
		log_info( "- %s, %ux%u, %f\n",
				(char* )(mode->pixel_format_descr.description),
				mode->frame_size_descr.discrete.width,
				mode->frame_size_descr.discrete.height,
				(float )mode->frame_interval_descr.discrete.numerator
					/ (float )mode->frame_interval_descr.discrete.denominator
		);
	}
	CAMERA_RUN(camera_exit(
			&data->camera
	));
	return RET_SUCCESS;
}

ret_t print_platform_info()
{
	char cmd[] = "uname -a";
	char str[STR_BUFFER_SIZE];
	FILE* file = popen( cmd, "r" );
	if( !file ) {
		perror( "popen" );
		return RET_FAILURE;
	}
	if( NULL == fgets(str, STR_BUFFER_SIZE, file) ) {
		perror( "pclose" );
		return RET_FAILURE;
	}
	log_info("\t%s", str);
	if( -1 == pclose( file ) ) {
		perror( "pclose" );
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

void program_exit( runtime_data_t* data )
{
	FREE( data->rgb_buffer );
	data->rgb_buffer_size = 0;
	camera_exit( &data->camera );
}

void print_cmd_line_info(
		// int argc,
		char* argv[]
)
{
	printf( "usage: %s\n", argv[0] );
}

int parse_cmd_line_args(
		int argc,
		char* argv[]
)
{
	// parse options:
	while( true ) {
		int option_index = 0;
		int c = getopt_long(
				argc, argv,
				short_options,
				long_options,
				&option_index
		);
		if( c == -1 ) { break; }
		switch( c ) {
			case 'h':
				return -1;
			break;
		}
	}
	return 0;
}
