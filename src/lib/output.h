#include <stdbool.h>


typedef struct {

		bool time_enable_print;
		bool time_enable_log;

		bool verbose_enable_print;
		bool verbose_enable_log;

		bool info_enable_print;
		bool info_enable_log;

		bool warning_enable_print;
		bool warning_enable_log;

		bool error_enable_print;
		bool error_enable_log;

} log_config_t;

/***********************
 * Function Declarations
 ***********************/

void log_init(
		const char* prefix,
		const log_config_t config
);
void log_exit(void);

void log_time(
		const char* fmt,
		...
);

void log_verbose(
		const char* fmt,
		...
);

void log_info(
		const char* fmt,
		...
);

void log_warning(
		const char* fmt,
		...
);

void log_error(
		const char* fmt,
		...
);

void log_run(void);

void log_stop(void);
