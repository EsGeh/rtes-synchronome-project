/************************
 * compress multiple frames
 * into zip files
 ************************/
#include "compressor.h"

#include "exe/synchronome/queues/rgb_queue.h"
#include "lib/camera.h"
#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/thread.h"

#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <zip.h>

// sockets:
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/********************
 * Types
********************/

typedef struct {
	char arch_filename[STR_BUFFER_SIZE];
	zip_t* zip_archive;
	FILE* mem_file;
} data_t;

/********************
 * Function Decls
********************/

#define SERVICE_NAME "compressor"

#define LOG_ERROR(fmt,...) log_error( "%-20s: " fmt, SERVICE_NAME , ## __VA_ARGS__ )
#define LOG_VERBOSE(fmt,...) log_verbose( "%-20s: " fmt, SERVICE_NAME, ## __VA_ARGS__ )
#define LOG_ERROR_STD_LIB(FUNC) LOG_ERROR("'" #FUNC "': %d - %s\n", errno, strerror(errno) )

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		LOG_ERROR( "error in '%s'\n", #FUNC_CALL ); \
		compressor_cleanup(); \
		return RET_FAILURE; \
	} \
}

ret_t compressor_cleanup();

static data_t data;

/********************
 * API Def
********************/

ret_t compressor_run(
		const compressor_args_t args,
		rgb_consumers_queue_t* input_queue,
		sem_t* done_processing
)
{
	thread_info( SERVICE_NAME );

	uint counter = 0;
	uint package_counter = 0;
	uint frame_acc_count = 0;
	char timestamp_str[STR_BUFFER_SIZE];
	while(true) {
		rgb_consumers_queue_read_start( input_queue );
		if( rgb_consumers_queue_get_should_stop( input_queue ) ) {
			LOG_VERBOSE( "stop received\n" );
			break;
		}
		frame_acc_count++;
		LOG_VERBOSE( "received %u\n", counter );
 		// open archive:
		if( data.zip_archive == NULL )
		{
			snprintf( data.arch_filename, STR_BUFFER_SIZE, "%s/package%04u.zip", args.shared_dir, package_counter);
			LOG_VERBOSE( "creating archive: %s\n", data.arch_filename );
			int err;
			data.zip_archive = zip_open( data.arch_filename, ZIP_CREATE | ZIP_TRUNCATE | ZIP_CHECKCONS, &err );
			if( data.zip_archive == NULL ) {
				zip_error_t error;
				zip_error_init_with_code(&error, err);
				LOG_ERROR("cannot open zip archive '%s': %s\n",
						data.arch_filename, zip_error_strerror(&error)
				);
				zip_error_fini(&error);
				compressor_cleanup();
				return RET_FAILURE;
			}
		}
		// add file to archive:
		{
			rgb_entry_t* frame = *rgb_consumers_queue_read_get( input_queue );
			// add all accumulated images to archive:
			char filename[STR_BUFFER_SIZE] = "";
			snprintf( filename, STR_BUFFER_SIZE, "package%04u/image%04u.ppm", package_counter, counter );
			LOG_VERBOSE( "adding file: %s\n", filename );

			char* file_content = NULL;
			size_t file_size = 0;
			data.mem_file = open_memstream(&file_content, &file_size);
			if( data.mem_file == NULL ) {
				LOG_ERROR_STD_LIB(open_memstream);
				compressor_cleanup();
				return RET_FAILURE;
			}
			snprintf(timestamp_str, STR_BUFFER_SIZE, "%lu.%lu",
					frame->time.tv_sec,
					frame->time.tv_nsec / 1000 / 1000
			);
			API_RUN( image_save_ppm_to_ram(
						filename,
						timestamp_str,
						frame->frame.data,
						frame->frame.size,
						args.image_size.width,
						args.image_size.height,
						data.mem_file
						) );
			if( fclose(data.mem_file) ) {
				LOG_ERROR_STD_LIB(fclose);
				compressor_cleanup();
				return RET_FAILURE;
			}
			data.mem_file = NULL;
			LOG_VERBOSE( "file size: %d\n", file_size );
			zip_source_t* zip_source = zip_source_buffer_create(file_content, file_size, 1, 0 ); // auto-frees file_conten, when no longer needed by zip
			if( zip_source == NULL ) {
				LOG_ERROR("cannot add file to zip archive %s\n", zip_strerror(data.zip_archive));
				zip_source_free( zip_source );
				compressor_cleanup();
				return RET_FAILURE;
			}
			if( -1 == zip_file_add( data.zip_archive, filename, zip_source, ZIP_FL_ENC_UTF_8 ) ) {
				LOG_ERROR("cannot add file to zip archive %s\n", zip_strerror(data.zip_archive) );
				zip_source_free( zip_source );
				compressor_cleanup();
				return RET_FAILURE;
			}
		}
		rgb_consumers_queue_read_stop_dump( input_queue );
		// close archive, when we have enough files:
		if( frame_acc_count >= args.package_size ) {
			LOG_VERBOSE( "write archive %s\n", data.arch_filename );
			API_RUN(compressor_cleanup());
			package_counter++;
			frame_acc_count = 0;
		}
		API_RUN( sem_post( done_processing ) );
		counter++;
	}
	LOG_VERBOSE( "stopping\n" );
	compressor_cleanup();
	return RET_SUCCESS;
}

ret_t compressor_cleanup()
{
	ret_t ret = RET_SUCCESS;
	if( data.mem_file != NULL ) {
		if( fclose(data.mem_file) ) {
			LOG_ERROR_STD_LIB(fclose);
			compressor_cleanup();
			ret = RET_FAILURE;
		}
		data.mem_file = NULL;
	}
	if( data.zip_archive != NULL ) {
		if( 0 != zip_close( data.zip_archive ) ) {
			LOG_ERROR("cannot close zip archive '%s': %s\n",
					data.arch_filename, zip_strerror(data.zip_archive)
			);
			compressor_cleanup();
			ret = RET_FAILURE;
		}
		data.zip_archive = NULL;
	}
	return ret;
}
