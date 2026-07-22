// ZCCI Compressor Module for Nintendo Switch (devkitPro / libnx)
// Ported from Azahar / Citra 3DS Emulator codebase

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <functional>

namespace SwitchZCCI {

#pragma pack(push, 1)
struct Z3DSFileHeader {
    static constexpr uint8_t EXPECTED_MAGIC[4] = {'Z', '3', 'D', 'S'};
    static constexpr uint8_t EXPECTED_VERSION = 1;

    uint8_t magic[4] = {'Z', '3', 'D', 'S'};
    uint8_t underlying_magic[4] = {'N', 'C', 'S', 'D'};
    uint8_t version = 1;
    uint8_t reserved = 0;
    uint16_t header_size = 0x20;
    uint32_t metadata_size = 0;
    uint64_t compressed_size = 0;
    uint64_t uncompressed_size = 0;
};
#pragma pack(pop)
static_assert(sizeof(Z3DSFileHeader) == 0x20, "Invalid Z3DSFileHeader size: must be 32 bytes");

enum class CompressResult {
    Success,
    CannotOpenInput,
    CannotOpenOutput,
    AlreadyCompressed,
    InvalidFormat,
    ReadError,
    WriteError,
    CompressionError
};

using ProgressCallback = std::function<void(uint64_t bytes_processed, uint64_t total_bytes)>;

/**
 * Compresses an unencrypted .3ds / .cci (NCSD) file into .zcci format.
 *
 * @param input_path  Path to the source .3ds / .cci file.
 * @param output_path Path to the destination .zcci file.
 * @param update_callback Optional callback to monitor compression progress.
 * @return CompressResult status of the operation.
 */
CompressResult Compress3DSToZCCI(const std::string& input_path,
                                 const std::string& output_path,
                                 ProgressCallback update_callback = nullptr);

/**
 * Returns a human-readable string for a CompressResult error code.
 */
const char* GetResultString(CompressResult result);

} // namespace SwitchZCCI
