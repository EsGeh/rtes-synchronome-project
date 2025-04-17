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

ret_t run_statistics( runtime_data_t* data );

ret_t print_platform_info();

ret_t run_capture( runtime_data_t* data );
int set_camera_format( runtime_data_t* data );

void do_exit( runtime_data_t* data );

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
		ret_t ret = run_statistics( &data );
		do_exit( &data );
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

ret_t run_statistics( runtime_data_t* data )
{
	if( RET_SUCCESS != print_platform_info() ) {
		return RET_FAILURE;
	}
	if( RET_SUCCESS != run_capture( data ) ) {
		return RET_FAILURE;
	}
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
	log_info("system info:\n" ); log_info("\t%s", str);
	if( -1 == pclose( file ) ) {
		perror( "pclose" );
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

#define TIME_START(label) timeval_t (label ## _t0) = time_measure_current_time();
#define TIME_STOP(label) \
	timeval_t (label ## _t1) = time_measure_current_time(); \
	timeval_t (label ## _delta); \
	time_delta( &(label ## _t1), &(label ## _t0), &(label ## _delta) ); \
	USEC (label ## _delta_us) = time_us_from_timespec( &(label ## _delta) );
#define TIME_US(label) (label ## _delta_us )

ret_t run_capture( runtime_data_t* data )
{
	CAMERA_RUN(camera_init(
			&data->camera,
			dev_name
	));
	CAMERA_RUN(set_camera_format(
				data
	));
	{
		CAMERA_RUN( camera_init_buffer(
				&data->camera,
				buffer_count
		));
		CAMERA_RUN( camera_stream_start( &data->camera ));
		frame_buffer_t frame;
		TIME_START(camera_get_frame);
		for( uint i=0; i<iterations_capture; ++i ) {
			CAMERA_RUN( camera_get_frame( &data->camera, &frame ));
			CAMERA_RUN( camera_return_frame( &data->camera, &frame));
		}
		TIME_STOP(camera_get_frame);
		log_info( "camera_get_frame + camera_return_frame:\n" );
		log_info( "\t%uus = %uus / %u\n",
				TIME_US(camera_get_frame) / iterations_capture,
				TIME_US(camera_get_frame),
				iterations_capture
		);

		/*
		API_RUN( image_save_ppm(
				"local/img/test.ppm",
				"captured from camera",
				data->rgb_buffer,
				data->rgb_buffer_size,
				data->camera.format.width,
				data->camera.format.height
		));
		*/
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
			TIME_START( image_convert_to_rgb )
			for( uint i=0; i<iterations_convert; ++i ) {
				API_RUN( image_convert_to_rgb(
						data->camera.format,
						frame.data,
						data->rgb_buffer,
						data->rgb_buffer_size
				));
			}
			TIME_STOP( image_convert_to_rgb )
			CAMERA_RUN( camera_return_frame( &data->camera, &frame));
			CAMERA_RUN( camera_stream_stop( &data->camera ));
			log_info( "image_convert_to_rgb:\n" );
			log_info( "\t%uus = %uus / %u\n",
					TIME_US(image_convert_to_rgb) / iterations_convert,
					TIME_US(image_convert_to_rgb),
					iterations_convert
			);
		}
		// measure 'image_save_ppm'
		{
			TIME_START( image_save_ppm )
			char filename[STR_BUFFER_SIZE];
			for( uint i=0; i<iterations_save_img; ++i ) {
				snprintf( filename, STR_BUFFER_SIZE, "local/img/test%u.ppm", i );
				API_RUN( image_save_ppm(
						filename,
						"captured from camera",
						data->rgb_buffer,
						data->rgb_buffer_size,
						data->camera.format.width,
						data->camera.format.height
				));
			}
			TIME_STOP( image_save_ppm )
			log_info( "image_save_ppm:\n" );
			log_info( "\t%uus = %uus / %u\n",
					TIME_US(image_save_ppm) / iterations_save_img,
					TIME_US(image_save_ppm),
					iterations_save_img
			);
		}
	}
	return RET_SUCCESS;
}

int set_camera_format( runtime_data_t* data )
{
	format_descriptions_t format_descriptions;
	if( RET_SUCCESS != camera_list_formats(
				&data->camera,
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
			fprintf(stderr, "camera does not support expected format!\n");
			return EXIT_FAILURE;
		}
	}
	if( RET_SUCCESS != camera_set_format(
				&data->camera,
				100,
				100,
				&required_format,
				true // force format
	))
		return RET_FAILURE;
	return RET_SUCCESS;
}

void do_exit( runtime_data_t* data )
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
