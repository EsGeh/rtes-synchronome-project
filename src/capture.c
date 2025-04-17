#include "camera.h"
#include "image.h"
#include "global.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h> // <- work with files
#include <string.h>


const char short_options[] = "h";
const  struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ 0,0,0,0 },
};


void print_cmd_line_info(
		// int argc,
		char* argv[]
);
int parse_cmd_line_args(
		int argc,
		char* argv[]
);

#define ERR_HANDLING( EXIT_HANDLER, FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		fprintf(stderr, "ERROR: %s\n", camera_error() ); \
		EXIT_HANDLER; \
		return EXIT_SUCCESS; \
	} \
}

typedef struct {
	camera_t camera;
	byte_t* rgb_buffer;
	uint rgb_buffer_size;

} runtime_data_t;

int set_camera_format( runtime_data_t* data );

void do_exit( runtime_data_t* data );

int main(
		int argc,
		char* argv[]
) {
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
	runtime_data_t data = {
		.rgb_buffer = NULL,
		.rgb_buffer_size = 0,
	};
	camera_zero( &data.camera );

	const char dev_name[] = "/dev/video0";
	ERR_HANDLING(,camera_init(
			&data.camera,
			dev_name
	));
	ERR_HANDLING(do_exit(&data),set_camera_format(
				&data
	));
	// allocate image buffer:
	data.rgb_buffer_size = image_rgb_size(
			data.camera.format.width,
			data.camera.format.height
	);
	CALLOC(data.rgb_buffer, data.rgb_buffer_size, 1);
	{
		ERR_HANDLING(do_exit(&data), camera_init_buffer(
				&data.camera,
				1
		));
		ERR_HANDLING(do_exit(&data), camera_stream_start( &data.camera ));
		frame_buffer_t frame;
		ERR_HANDLING(do_exit(&data), camera_get_frame( &data.camera, &frame ));

		ERR_HANDLING(do_exit(&data), image_convert_to_rgb(
				data.camera.format,
				frame.data,
				data.rgb_buffer,
				data.rgb_buffer_size
		));
		ERR_HANDLING(do_exit(&data), image_save_ppm(
				"local/img/test.ppm",
				"captured from camera",
				data.rgb_buffer,
				data.rgb_buffer_size,
				data.camera.format.width,
				data.camera.format.height
		));
		ERR_HANDLING(do_exit(&data), camera_return_frame( &data.camera, &frame));
		ERR_HANDLING(do_exit(&data), camera_stream_stop( &data.camera ));
	}
	do_exit( &data );

	return EXIT_SUCCESS;
}

int set_camera_format( runtime_data_t* data )
{
	format_descriptions_t format_descriptions;
	if( RET_SUCCESS != camera_list_formats(
				&data->camera,
				&format_descriptions
	) )
		return RET_FAILURE;
	// printf( "camera video formats:\n" );
	__u32 required_format;
	{
		// unsigned char expected_format = "YUYV";
		bool format_found = false;
		for( unsigned int i=0; i< format_descriptions.count; i++ ) {
			// printf( "format: %s\n", format_descriptions.format_descrs[i].description );
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
