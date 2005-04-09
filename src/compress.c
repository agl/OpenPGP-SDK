#include "compress.h"
#include <zlib.h>

#define DECOMPRESS_BUFFER	1024

typedef struct
    {
    ops_packet_reader_t *reader;
    void *reader_arg;
    ops_parse_options_t *opt;
    ops_region_t *region;
    unsigned char in[DECOMPRESS_BUFFER];
    unsigned char out[DECOMPRESS_BUFFER];
    z_stream stream;
    size_t offset;
    int inflate_ret;
    } decompress_arg_t;

#define ERR(err)	do { content.content.error.error=err; content.tag=OPS_PARSER_ERROR; arg->opt->cb(&content,arg->opt->cb_arg); return OPS_PR_EARLY_EOF; } while(0)

static ops_packet_reader_ret_t
compressed_data_reader(unsigned char *dest,unsigned length,void *arg_)
    {
    decompress_arg_t *arg=arg_;
    ops_parser_content_t content;

    if(arg->region->length_read == arg->region->length)
	{
	if(arg->inflate_ret != Z_STREAM_END)
	    ERR("Compressed data didn't end when region ended.");
	else
	    return OPS_PR_EOF;
	}

    while(length > 0)
	{
	int len;

	if(arg->out+arg->offset == arg->stream.next_out)
	    {
	    int ret;

	    arg->stream.next_out=arg->out;
	    arg->stream.avail_out=sizeof arg->out;
	    if(arg->stream.avail_in == 0)
		{
		int n=arg->region->length_read-arg->region->length;

		if(n > sizeof arg->in)
		    n=sizeof arg->in;

		arg->opt->reader=arg->reader;
		arg->opt->reader_arg=arg->reader_arg;

		if(!ops_limited_read(arg->in,n,arg->region,arg->opt))
		    return OPS_PR_EARLY_EOF;

		arg->opt->reader=compressed_data_reader;
		arg->opt->reader_arg=arg;

		arg->stream.next_in=arg->in;
		arg->stream.avail_in=n;
		}

	    ret=inflate(&arg->stream,Z_SYNC_FLUSH);
	    if(ret == Z_STREAM_END
	       && arg->region->length_read != arg->region->length)
		ERR("Compressed stream ended before packet end.");
	    else if(ret != Z_OK)
		ERR("Decompression error.");
	    arg->inflate_ret=ret;
	    }
	len=arg->offset-(arg->stream.next_out-arg->out);
	if(len > length)
	    len=length;
	memcpy(dest,arg->out,len);
	arg->offset+=len;
	}

    return OPS_PR_OK;
    }

int ops_decompress(ops_region_t *region,ops_parse_options_t *opt)
    {
    decompress_arg_t arg;

    memset(&arg,'\0',sizeof arg);

    arg.reader_arg=opt->reader_arg;
    arg.reader=opt->reader;
    arg.region=region;
    arg.opt=opt;

    arg.stream.next_in=Z_NULL;
    arg.stream.avail_in=0;
    arg.stream.next_out=arg.out;
    arg.offset=0;
    arg.stream.zalloc=Z_NULL;
    arg.stream.zfree=Z_NULL;

    inflateInit(&arg.stream);

    opt->reader=compressed_data_reader;
    opt->reader_arg=&arg;

    return ops_parse(opt);
    }