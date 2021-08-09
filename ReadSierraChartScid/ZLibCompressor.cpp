// ZLibCompressor.cpp : Include file for standard system include files,
// or project specific include files.

#include <cassert>
#include "ZLibCompressor.h"

using namespace std;

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */

ZlibCompressor::ZlibCompressor(const std::filesystem::path out_path_)
    : out_path(out_path_)
{
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    int ret = deflateInit(&strm, level);
    if (ret == Z_OK) is_constructed = true;
    outbuf = new unsigned char[CHUNK];
    for (int i = 0; i < CHUNK; i++)
        outbuf[i] = 0;
    strm.avail_out = CHUNK; // avail_out is # of available output bytes
    strm.next_out = outbuf;

    csv_ostream.open(out_path);
}

ZlibCompressor::~ZlibCompressor() {
    // write out last part
    int ret = deflate(&strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    auto size = CHUNK - strm.avail_out;
    csv_ostream.write((char*)outbuf, size);
    deflateEnd(&strm);

    csv_ostream.close();
}

int ZlibCompressor::save_line_compressed(const string& in) {
    assert(is_constructed);
    int ret{ Z_OK };

    strm.avail_in = (uInt)in.size();
    strm.next_in = (Bytef*)in.c_str();

    do {
        // compress the supplied input string
        strm.avail_out = CHUNK;
        strm.next_out = outbuf;
        ret = deflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

        // compute how much output was created
        //auto deflated_size = starting_out - strm.avail_out;
        // write size chars to stream
        auto size = CHUNK - strm.avail_out;
        csv_ostream.write((char*)outbuf, size);
    } while (strm.avail_out == 0);

    return ret;
}
