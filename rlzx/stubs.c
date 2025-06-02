#include <stddef.h>
#include <string.h>

#include "wimlib/compressor_ops.h"
#include "wimlib.h"

void
get_random_bytes(void *p, size_t n)
{
	memset(p, 0, n);
}

int
wimlib_global_init(int init_flags)
{
	return 0;
}

static u64 null_get_needed_memory(size_t max_block_size,
		unsigned int compression_level,
		bool destructive)
{
	return 0;
}

static int null_create_compressor(size_t max_block_size,
		unsigned int compression_level,
		bool destructive,
		void **private_ret)
{
	*private_ret = NULL;
	return WIMLIB_ERR_INVALID_COMPRESSION_TYPE;
}

static size_t null_compress(const void *uncompressed_data,
		size_t uncompressed_size,
		void *compressed_data,
		size_t compressed_size_avail,
		void *private)
{
	return 0;
}

static void null_free_compressor(void *private)
{
}

static int null_set_uint_property(
	enum wimlib_compressor_uint_property property,
	size_t value, void *private)
{
	return WIMLIB_ERR_INVALID_PARAM;
}

const struct compressor_ops lzms_compressor_ops = {

	.get_needed_memory  = null_get_needed_memory,
	.create_compressor  = null_create_compressor,
	.compress			= null_compress,
	.free_compressor    = null_free_compressor,
	.set_uint_property  = null_set_uint_property,
};

const struct compressor_ops xpress_compressor_ops = {

	.get_needed_memory	= null_get_needed_memory,
	.create_compressor	= null_create_compressor,
	.compress			= null_compress,
	.free_compressor	= null_free_compressor,
	.set_uint_property	= null_set_uint_property,
};
