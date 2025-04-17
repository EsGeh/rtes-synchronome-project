#include "output.h"

/***********************
 * Function Declarations
 ***********************/

void log_init(
		const char* prefix
)
{
	openlog(
			prefix,
			LOG_CONS,
			LOG_USER
	);
}

void log_exit()
{
	closelog();
}
