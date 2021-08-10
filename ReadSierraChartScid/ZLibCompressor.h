// ZLibCompressor.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <filesystem>
#include <fstream>

#include "zlib/zlib.h"
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

struct ZlibCompressor {
    static const int CHUNK{ 16384 };
    static const int level{ 8 }; // level is from -1 to 9...larger is better compression, but slower
    z_stream strm;
    bool is_constructed{ false };
    unsigned char* outbuf{ nullptr };
    std::filesystem::path out_path;
    std::ofstream z_ostream;

    ZlibCompressor(const std::filesystem::path out_path);
    ~ZlibCompressor();

    int save_line_compressed(const std::string& in);
};
