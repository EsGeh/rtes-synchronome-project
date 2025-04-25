#include "lib/global.h"

#include "lib/camera.h"

/********************
 * Function Decls
********************/

ret_t synchronome_run(
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval,
		const char* output_dir
);

void synchronome_stop(void);
