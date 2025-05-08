#include <stdbool.h>


/***********************
 * Function Declarations
 ***********************/

void log_init(
		const char* prefix,

		bool time_enable_print,
		bool time_enable_log,

		bool verbose_enable_print,
		bool verbose_enable_log,

		bool info_enable_print,
		bool info_enable_log,

		bool error_enable_print,
		bool error_enable_log
);
void log_exit(void);

void log_time(
		char* fmt,
		...
);

void log_verbose(
		char* fmt,
		...
);

void log_info(
		char* fmt,
		...
);

void log_error(
		char* fmt,
		...
);
