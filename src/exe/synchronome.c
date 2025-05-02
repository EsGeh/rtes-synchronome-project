#include "synchronome/main.h"
#include "simple_capture/main.h"
#include "lib/output.h"
#include "lib/global.h"

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>


/********************
 * Types
********************/

typedef enum {
	RUN_SYNCHRONOME,
	RUN_CAPTURE_SINGLE
} command_t;

/********************
 * Global Constants
********************/

#define LOG_PREFIX "[Course #4] [Final Project]"

const char short_options[] = "h";
const  struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ 0,0,0,0 },
};

// synchronome:

static const synchronome_args_t synchronome_def_args = {
	.pixel_format = V4L2_PIX_FMT_YUYV,
	.output_dir = "local/output/synchronome",
	.acq_interval = { 1, 3 },
	.clock_tick_interval = { 1, 1 },
	.tick_threshold = 0.2,
	.save_all = false,
	.select_delay = 10,
	.size = {
			.width = 320,
			.height = 240,
	},
};

const char synchronome_short_options[] = "ho:s:a:c:x:d:t";
const  struct option synchronome_long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "output-dir", required_argument, 0, 'o' },
	{ "size", required_argument, 0, 's' },
	{ "acq-interval", required_argument, 0, 'a' },
	{ "clock-tick", required_argument, 0, 'c' },
	{ "tick-thresh", required_argument, 0, 't' },
	{ "select-delay", required_argument, 0, 'd' },
	{ "save-all", no_argument, 0, 'x' },
	{ 0,0,0,0 },
};

// capture:

static const capture_args_t capture_def_args = {
	.output_dir = "local/output/synchronome",
	.dev_name = "/dev/video0",
	.acq_interval = { 1, 3 },
	.size = {
			.width = 320,
			.height = 240,
	},
};

const char capture_short_options[] = "ho:s:a:";
const  struct option capture_long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "output-dir", required_argument, 0, 'o' },
	{ "size", required_argument, 0, 's' },
	{ "acq-interval", required_argument, 0, 'a' },
	{ 0,0,0,0 },
};

/********************
 * Function Decls
********************/

int parse_cmd_line_args(
		int argc,
		char* argv[],
		int* rest_args,
		command_t* command
);

void print_cmd_line_info(
		char* argv[]
);

// synchronome:

int synchronome_parse_cmd_line_args(
		int argc,
		char* argv[],
		synchronome_args_t* args
);

void synchronome_print_cmd_line_info(
		char* argv[]
);

void synchronome_print_args(
		synchronome_args_t* args
);

// capture:

int capture_parse_cmd_line_args(
		int argc,
		char* argv[],
		capture_args_t* args
);

void capture_print_cmd_line_info(
		char* argv[]
);

ret_t print_platform_info();

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
	command_t command;
	int rest_args = 0;
	{
		int ret = parse_cmd_line_args(
				argc,
				argv,
				&rest_args,
				&command
		);
		// --help:
		if( ret == -1 ) {
			print_cmd_line_info( argv );
			return EXIT_SUCCESS;
		}
		// error parsing cmd line args:
		else if( ret != 0 ) {
			print_cmd_line_info( argv );
			return EXIT_FAILURE;
		}
	}
	log_init(
			LOG_PREFIX,
			false, false,
			true, true,
			true, true
	);
	switch( command ) {
		case RUN_SYNCHRONOME: {
			synchronome_args_t args = synchronome_def_args;
			{
				int ret = synchronome_parse_cmd_line_args(
						argc-rest_args,
						&argv[rest_args],
						&args
				);
				// --help:
				if( ret == -1 ) {
					synchronome_print_cmd_line_info( argv );
					return EXIT_SUCCESS;
				}
				// error parsing cmd line args:
				else if( ret != 0 ) {
					synchronome_print_cmd_line_info( argv );
					return EXIT_FAILURE;
				}
			}
			{
				struct sigaction sa;
				memset( &sa, 0, sizeof(sa));
				sa.sa_handler = signal_handler;
				sigemptyset( &sa.sa_mask );
				// sa.sa_flags = SA_RESTART;
				if( -1 == sigaction( SIGINT, &sa, NULL ) ) {
					log_error( "'sigaction': %s\n", strerror(errno) );
					return EXIT_FAILURE;
				}
			}
			synchronome_print_args( &args );
			if( RET_SUCCESS != print_platform_info() ) {
				return EXIT_FAILURE;
			}
			ret_t ret = synchronome_run(
					args
			);
			log_exit();

			if( RET_SUCCESS != ret ) {
				log_error( "program failed!\n" );
				return EXIT_FAILURE;
			}
		}
		break;
		case RUN_CAPTURE_SINGLE: {
			capture_args_t args = capture_def_args;
			{
				int ret = capture_parse_cmd_line_args(
						argc-rest_args,
						&argv[rest_args],
						&args
				);
				// --help:
				if( ret == -1 ) {
					capture_print_cmd_line_info( argv );
					return EXIT_SUCCESS;
				}
				// error parsing cmd line args:
				else if( ret != 0 ) {
					capture_print_cmd_line_info( argv );
					return EXIT_FAILURE;
				}
			}
			log_info("---------------------------\n");
			if( RET_SUCCESS != print_platform_info() ) {
				return EXIT_FAILURE;
			}
			log_info("---------------------------\n");
			ret_t ret = capture_single(
					&args
			);
			log_exit();
			if( RET_SUCCESS != ret ) {
				log_error( "program failed!\n" );
				return EXIT_FAILURE;
			}
		}
		break;
	};
	return RET_SUCCESS;
}

/********************
 * Function Defs
********************/

int parse_cmd_line_args(
		int argc,
		char* argv[],
		int* rest_args,
		command_t* command
)
{
	(*command) = RUN_SYNCHRONOME;
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
			case '?':
				// log_error( "invalid arguments!\n" );
				return 1;
			break;
			default:
				log_error( "getopt returned %o\n", c );
				return 1;
		}
	}
	(*rest_args) = optind-1;
	if( optind < argc ) {
		(*rest_args) = optind;
		char* cmd_str = argv[optind];
		if( !strcmp( "synchronome", cmd_str ) ) {
			(*command) = RUN_SYNCHRONOME;
		}
		else if( !strcmp( "capture", cmd_str ) ) {
			(*command) = RUN_CAPTURE_SINGLE;
		}
		else {
			log_error( "invalid COMMAND %s\n", cmd_str );
			return 1;
		}
	}
	return RET_SUCCESS;
}

void print_cmd_line_info(
		char* argv[]
)
{
	printf(
			"usage: %s [OPTIONS...] -- [COMMAND] [ARGUMENTS...]\n",
			argv[0]
	);
	printf(
			"\n"
			"try `$ %s -- COMMAND -h` to get help for specific commands\n",
			argv[0]
	);
	printf(
			"\n"
			"OPTIONS:\n"
	);
	printf(
			"--help|-h print help\n"
	);
	printf( "\nCOMMAND: (default: synchronome)\n"
			"  synchronome: run synchronomy\n"
			"  capture: capture and save single frame\n"
	);
}

// synchronome:

int synchronome_parse_cmd_line_args(
		int argc,
		char* argv[],
		synchronome_args_t* args
)
{
	optind = 0; // reset getopt
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
				synchronome_short_options,
				synchronome_long_options,
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
			case 'c': {
				if( RET_SUCCESS != parse_2_toks(optarg, '/', &args->clock_tick_interval.numerator, &args->clock_tick_interval.denominator) ) {
					log_error( "invalid format. expected: X/Y\n" );
					return 1;
				}
			}
			break;
			case 'd': {
				char* next_tok;
				args->select_delay = strtoul( optarg, &next_tok, 10);
				if( next_tok == optarg ) {
					log_error( "failed parsing select-delay. expected: unsigned int\n" );
					return 1;
				}
			}
			break;
			case 't': {
				char* next_tok;
				args->tick_threshold = strtof( optarg, &next_tok );
				if( next_tok == optarg ) {
					log_error( "failed parsing tick-threshold. expected: floating point value\n" );
					return 1;
				}
			}
			break;
			case 'x': {
					args->save_all = true;
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
	if( optind < argc ) {
		log_error( "unexpected argument %s\n", argv[optind] );
		return 1;
	}
	return 0;
}

void synchronome_print_cmd_line_info(
		char* argv[]
)
{
	printf(
			"usage: %s synchronome [OPTIONS...]\n",
			argv[0]
	);
	printf(
			"\n"
			"repeatedly capture frames from camera and save them to disk. \n"
			"The camera observes an external ticking clock.\n"
			"The process synchronizes itself to the external clock and save one image per tick.\n"
	);
	printf(
			"\n"
			"OPTIONS:\n"
	);
	printf(
			"--size|-s SIZE_DESCR: image size. Format WxH. default: 320x240\n"
	);
	printf(
			"--acq-interval|-a RATE: frame sample interval in seconds. Format: X/Y. default: %u/%u\n",
			synchronome_def_args.acq_interval.numerator,
			synchronome_def_args.acq_interval.denominator
	);
	printf(
			"--clock-tick|-c FREQ: tick interval of the external clock in seconds. Format: X/Y. default: %u/%u\n",
			synchronome_def_args.clock_tick_interval.numerator,
			synchronome_def_args.clock_tick_interval.denominator
	);
	printf(
			"--tick-thresh|-t FLOAT: threshold for tick detection (fraction of (max-avg)). default: %f\n",
			synchronome_def_args.tick_threshold
	);
	printf(
			"--select-delay|-d DELAY: wait DELAY seconds before starting selection/saving of images. default: %u\n",
			synchronome_def_args.select_delay
	);
	printf(
			"--save-all|-x: save all acquired frames to disk. default: false\n"
	);
	printf(
			"--output-dir|-o DIR: output recorded images here. default: '%s'\n",
			synchronome_def_args.output_dir
	);
}

void synchronome_print_args(
		synchronome_args_t* args
)
{
	log_verbose( "selected settings:\n" );
	log_verbose( "size: %ux%u\n", args->size.width, args->size.height );
	log_verbose( "acquisition interval (in s): %u/%u\n",
			args->acq_interval.numerator,
			args->acq_interval.denominator
	);
	log_verbose( "clock tick interval (in s): %u/%u\n",
			args->clock_tick_interval.numerator,
			args->clock_tick_interval.denominator
	);
	log_verbose( "output dir: %s\n", args->output_dir );
}

// capture:

int capture_parse_cmd_line_args(
		int argc,
		char* argv[],
		capture_args_t* args
)
{
	optind = 0; // reset getopt
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
				synchronome_short_options,
				synchronome_long_options,
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
	if( optind < argc ) {
		log_error( "unexpected argument %s\n", argv[optind] );
		return 1;
	}
	return 0;
}

void capture_print_cmd_line_info(
		char* argv[]
)
{
	printf(
			"usage: %s capture [OPTIONS...]\n",
			argv[0]
	);
	printf(
			"\n"
			"capture and save single frame\n"
	);
	printf(
			"\n"
			"OPTIONS:\n"
	);
	printf(
			"--size|-s SIZE_DESCR: image size. Format WxH. default: 320x240\n"
	);
	printf(
			"--acq-interval|-a RATE: frame sample interval in seconds. Format: X/Y. default: %u/%u\n",
			synchronome_def_args.acq_interval.numerator,
			synchronome_def_args.acq_interval.denominator
	);
	printf(
			"--output-dir|-o DIR: output recorded images here. default: '%s'\n",
			synchronome_def_args.output_dir
	);
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
	log_info("%s", str);
	if( -1 == pclose( file ) ) {
		perror( "pclose" );
		return RET_FAILURE;
	}
	return RET_SUCCESS;
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
