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
	char* output_dir;
} program_args_t;

/********************
 * Global Constants
********************/

const char short_options[] = "ho:";
const  struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "output-dir", required_argument, 0, 'o' },
	{ 0,0,0,0 },
};
const char dev_name[] = "/dev/video0";
const char output_dir[] = "local/output/synchronome";

static const program_args_t def_args = {
	.output_dir = "local/output/synchronome",
};

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

void scheduler(void);

static void signal_handler(int signal);

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
		const uint width = 320;
		const uint height = 240;
		ret_t ret = synchronome_run(
				width,
				height,
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
			case 'o':
				args->output_dir = optarg;
			break;
			case '?':
				log_error( "invalid arguments!\n" );
				return 1;
			break;
			default:
				log_error( "getopt returned %o", c );
				return -1;
		}
	}
	return 0;
}

static void signal_handler(int signal)
{
	switch( signal ) {
		case SIGINT:
			synchronome_stop();
		break;
	}
}
