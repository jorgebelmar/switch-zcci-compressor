// ZCCI (Z3DS) ROM Compressor Module for Nintendo Switch
#include "zcci_compressor.h"
#include "zstd_seekable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>
#include <chrono>

namespace SwitchZCCI {

static const uint8_t g_default_zcci_metadata[128] = {
    0x01, 0x01, 0x0a, 0x11, 0x00, 0x63, 0x6f, 0x6d, 0x70, 0x72, 0x65, 0x73, 0x73, 0x6f, 0x72, 0x41,
    0x7a, 0x61, 0x68, 0x61, 0x72, 0x20, 0x32, 0x31, 0x32, 0x36, 0x2e, 0x30, 0x2d, 0x72, 0x63, 0x33,
    0x01, 0x09, 0x18, 0x00, 0x74, 0x69, 0x74, 0x6c, 0x65, 0x69, 0x6e, 0x66, 0x6f, 0x00, 0x87, 0x1b,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x50, 0x1d, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x14, 0x00, 0x64, 0x61, 0x74, 0x65, 0x32, 0x30, 0x32,
    0x36, 0x2d, 0x30, 0x37, 0x2d, 0x32, 0x32, 0x54, 0x30, 0x36, 0x3a, 0x31, 0x37, 0x3a, 0x33, 0x32,
    0x5a, 0x01, 0x0c, 0x06, 0x00, 0x6d, 0x61, 0x78, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x73, 0x69, 0x7a,
    0x65, 0x32, 0x36, 0x32, 0x31, 0x34, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

CompressResult Compress3DSToZCCI(
    const std::string& input_path,
    const std::string& output_path,
    ProgressCallback update_callback)
{
    FILE* in_file = fopen(input_path.c_str(), "rb");
    if (!in_file) {
        return CompressResult::CannotOpenInput;
    }

    struct stat st_in;
    if (stat(input_path.c_str(), &st_in) != 0) {
        fclose(in_file);
        return CompressResult::CannotOpenInput;
    }
    uint64_t total_size = static_cast<uint64_t>(st_in.st_size);

    uint8_t header_buf[0x200];
    size_t bytes_hdr = fread(header_buf, 1, sizeof(header_buf), in_file);
    fseeko(in_file, 0, SEEK_SET);

    uint8_t underlying_magic[4] = {'N', 'C', 'S', 'D'};
    if (bytes_hdr >= 0x104 && memcmp(header_buf + 0x100, "NCSD", 4) == 0) {
        memcpy(underlying_magic, header_buf + 0x100, 4);
    } else if (bytes_hdr >= 4 && memcmp(header_buf, "NCCH", 4) == 0) {
        memcpy(underlying_magic, "NCCH", 4);
    }

    FILE* out_file = fopen(output_path.c_str(), "wb");
    if (!out_file) {
        fclose(in_file);
        return CompressResult::CannotOpenOutput;
    }

    // Write 32-byte Z3DS File Header + 128-byte Metadata payload (Total 160 bytes)
    Z3DSFileHeader header{};
    memcpy(header.magic, Z3DSFileHeader::EXPECTED_MAGIC, 4);
    memcpy(header.underlying_magic, underlying_magic, 4);
    header.version = Z3DSFileHeader::EXPECTED_VERSION;
    header.reserved = 0;
    header.header_size = sizeof(Z3DSFileHeader); // 32
    header.metadata_size = 128;
    header.compressed_size = 0;
    header.uncompressed_size = total_size;

    fwrite(&header, 1, sizeof(header), out_file);
    fwrite(g_default_zcci_metadata, 1, sizeof(g_default_zcci_metadata), out_file);

    ZSTD_seekable_CStream* cstream = ZSTD_seekable_createCStream();
    if (!cstream) {
        fclose(in_file);
        fclose(out_file);
        return CompressResult::CompressionError;
    }

    // Initialize ZSTD Seekable Stream with checksumFlag = 1 (Exact Azahar PC match)
    if (ZSTD_isError(ZSTD_seekable_initCStream(cstream, 3, 1, 256 * 1024))) {
        ZSTD_seekable_freeCStream(cstream);
        fclose(in_file);
        fclose(out_file);
        return CompressResult::CompressionError;
    }

    constexpr size_t IN_BUF_SIZE = 256 * 1024;
    constexpr size_t OUT_BUF_SIZE = 256 * 1024 + 1024;
    std::vector<uint8_t> in_buf(IN_BUF_SIZE);
    std::vector<uint8_t> out_buf(OUT_BUF_SIZE);

    uint64_t processed_bytes = 0;
    uint64_t total_written_compressed = 0;

    while (processed_bytes < total_size) {
        size_t to_read = (total_size - processed_bytes > IN_BUF_SIZE) ?
                          IN_BUF_SIZE : static_cast<size_t>(total_size - processed_bytes);

        size_t read_bytes = fread(in_buf.data(), 1, to_read, in_file);
        if (read_bytes == 0) break;

        ZSTD_inBuffer input = { in_buf.data(), read_bytes, 0 };

        while (input.pos < input.size) {
            ZSTD_outBuffer output = { out_buf.data(), out_buf.size(), 0 };
            size_t ret = ZSTD_seekable_compressStream(cstream, &output, &input);
            if (ZSTD_isError(ret)) {
                ZSTD_seekable_freeCStream(cstream);
                fclose(in_file);
                fclose(out_file);
                return CompressResult::CompressionError;
            }

            if (output.pos > 0) {
                if (fwrite(out_buf.data(), 1, output.pos, out_file) != output.pos) {
                    ZSTD_seekable_freeCStream(cstream);
                    fclose(in_file);
                    fclose(out_file);
                    return CompressResult::WriteError;
                }
                total_written_compressed += output.pos;
            }
        }

        processed_bytes += read_bytes;

        if (update_callback) {
            update_callback(processed_bytes, total_size);
        }
    }

    // Flush Seek Table
    size_t remaining = 0;
    do {
        ZSTD_outBuffer output = { out_buf.data(), out_buf.size(), 0 };
        remaining = ZSTD_seekable_endStream(cstream, &output);
        if (ZSTD_isError(remaining)) {
            ZSTD_seekable_freeCStream(cstream);
            fclose(in_file);
            fclose(out_file);
            return CompressResult::CompressionError;
        }

        if (output.pos > 0) {
            if (fwrite(out_buf.data(), 1, output.pos, out_file) != output.pos) {
                ZSTD_seekable_freeCStream(cstream);
                fclose(in_file);
                fclose(out_file);
                return CompressResult::WriteError;
            }
            total_written_compressed += output.pos;
        }
    } while (remaining > 0);

    ZSTD_seekable_freeCStream(cstream);

    // Update Header compressed_size and uncompressed_size
    header.compressed_size = total_written_compressed;
    header.uncompressed_size = total_size;

    fseeko(out_file, 0, SEEK_SET);
    fwrite(&header, 1, sizeof(header), out_file);

    fflush(out_file);
    fclose(in_file);
    fclose(out_file);

    return CompressResult::Success;
}

const char* GetResultString(CompressResult result) {
    switch (result) {
        case CompressResult::Success: return "Success";
        case CompressResult::CannotOpenInput: return "Cannot open input file";
        case CompressResult::CannotOpenOutput: return "Cannot open output file";
        case CompressResult::AlreadyCompressed: return "File is already compressed";
        case CompressResult::InvalidFormat: return "Invalid 3DS format";
        case CompressResult::ReadError: return "Error reading file";
        case CompressResult::WriteError: return "Error writing file";
        case CompressResult::CompressionError: return "ZSTD compression error";
        default: return "Unknown error";
    }
}

} // namespace SwitchZCCI
