#include "synchronome/main.h"
#include "lib/output.h"
#include "lib/global.h"

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h> // <- work with files
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>


/********************
 * Types
********************/

typedef struct {
	frame_size_t size;
	frame_interval_t acq_interval;
	char* output_dir;
} program_args_t;

/********************
 * Global Constants
********************/

static const program_args_t def_args = {
	.output_dir = "local/output/synchronome",
	.acq_interval = { 1, 1 },
	.size = {
			.width = 320,
			.height = 240,
	},
};

const char short_options[] = "ho:s:a:";
const  struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "size", required_argument, 0, 's' },
	{ "acquisition-intervall", required_argument, 0, 'a' },
	{ "output-dir", required_argument, 0, 'o' },
	{ 0,0,0,0 },
};

const char dev_name[] = "/dev/video0";
const char output_dir[] = "local/output/synchronome";

/********************
 * Function Decls
********************/

void print_cmd_line_info(
		// int argc,
		char* argv[]
);
int parse_cmd_line_args(
		int argc,
		char* argv[],
		program_args_t* args
);

void print_args(
		program_args_t* args
);

static void signal_handler(int signal);

ret_t parse_size(
		const char* str,
		frame_size_t* size
);

ret_t parse_2_toks(
		const char* str,
		const char sep,
		uint* x,
		uint* y
);

/********************
 * Main
********************/

int main(
		int argc,
		char* argv[]
) {
	program_args_t args = def_args;
	// parse cmd line args:
	{
		int ret = parse_cmd_line_args(
				argc, argv,
				&args
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
	print_args( &args );
	log_info("---------------------------\n");
	log_init(
			argv[0],
			true, false,
			true, true
	);
	{
		struct sigaction sa;
		memset( &sa, 0, sizeof(sa));
		sa.sa_handler = signal_handler;
		sigemptyset( &sa.sa_mask );
		// sa.sa_flags = SA_RESTART;
		if( -1 == sigaction( SIGINT, &sa, NULL ) ) {
			log_error( "'sigaction': %s", strerror(errno) );
			return EXIT_FAILURE;
		}
	}
	// run:
	{
		const pixel_format_t pixel_format = V4L2_PIX_FMT_YUYV;
		ret_t ret = synchronome_run(
				pixel_format,
				args.size,
				&args.acq_interval,
				args.output_dir
		);
		log_exit();

		if( RET_SUCCESS != ret ) {
			return EXIT_FAILURE;
		}
	}
	log_info( "exit success\n" );
	exit(EXIT_SUCCESS);
}

/********************
 * Function Defs
********************/

void print_cmd_line_info(
		// int argc,
		char* argv[]
)
{
	printf(
			"usage: %s [OPTIONS]\n"
			"\n"
			"OPTIONS:\n",
			argv[0]
	);
	printf(
			"--size|-s SIZE_DESCR: image size. Format WxH. default: 320x240\n"
	);
	printf(
			"--acquisition-interval|-a RATE: frame sample interval in seconds. Format: X/Y. default: %u/%u\n",
			def_args.acq_interval.numerator,
			def_args.acq_interval.denominator
	);
	printf(
			"--output-dir|-o DIR: output recorded images here. default: '%s'\n",
			def_args.output_dir
	);
}

int parse_cmd_line_args(
		int argc,
		char* argv[],
		program_args_t* args
)
{
	// parse options:
	// <getopt.h> globals:
	// - optarg: the option argument, as in --some-opt=OPTARG
	// - optind: next element to be processed by getopt_long
	// - opterr: if 1, getop_long automatically prints errors
	// - optopt: erroneous opt char (if error)
	int option_index = 0;
	while( true ) {
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
			case 'o':
				args->output_dir = optarg;
			break;
			case 's': {
				if( RET_SUCCESS != parse_2_toks(optarg, 'x', &args->size.width, &args->size.height) ) {
					log_error( "invalid format. expected: WxH\n" );
					return 1;
				}
			}
			break;
			case 'a':
				if( RET_SUCCESS != parse_2_toks(optarg, '/', &args->acq_interval.numerator, &args->acq_interval.denominator) ) {
					log_error( "invalid format. expected: X/Y\n" );
					return 1;
				}
			break;
			case '?':
				// log_error( "invalid arguments!\n" );
				return 1;
			break;
			default:
				log_error( "getopt returned %o\n", c );
				return 1;
		}
	}
	return 0;
}

void print_args(
		program_args_t* args
)
{
	log_info( "selected settings:\n" );
	log_info( "size: %ux%u\n", args->size.width, args->size.height );
	log_info( "acquisition interval: %u/%u\n",
			args->acq_interval.numerator,
			args->acq_interval.denominator
	);
	log_info( "output dir: %s\n", args->output_dir );
}

ret_t parse_2_toks(
		const char* str,
		const char sep,
		uint* x,
		uint* y
)
{
	const char* next_pos = str;
	{
		char* rest_str;
		errno = 0;
		(*x) = strtoul(next_pos, &rest_str, 10);
		if( errno != 0 || rest_str == next_pos ) {
			return RET_FAILURE;
		}
		if( rest_str[0] != sep ) {
			return RET_FAILURE;
		}
		next_pos = rest_str;
	}
	next_pos++;
	if( strlen(next_pos) == 0 ) {
		return RET_FAILURE;
	}
	{
		char* rest_str;
		errno = 0;
		(*y) = strtoul(next_pos, &rest_str, 10);
		if( errno != 0 || rest_str == next_pos || rest_str[0] != '\0' ) {
			return RET_FAILURE;
		}
	}
	return RET_SUCCESS;
}

static void signal_handler(int signal)
{
	switch( signal ) {
		case SIGINT:
			synchronome_stop();
		break;
	}
}
