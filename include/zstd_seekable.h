// ZSTD Seekable Stream API for Nintendo Switch ZCCI Compressor
#pragma once

#include "zstd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ZSTD_seekable_CStream_s ZSTD_seekable_CStream;

ZSTD_seekable_CStream* ZSTD_seekable_createCStream(void);
size_t ZSTD_seekable_initCStream(ZSTD_seekable_CStream* zcs, int compressionLevel, int checksumFlag, unsigned maxFrameSize);
size_t ZSTD_seekable_compressStream(ZSTD_seekable_CStream* zcs, ZSTD_outBuffer* output, ZSTD_inBuffer* input);
size_t ZSTD_seekable_endStream(ZSTD_seekable_CStream* zcs, ZSTD_outBuffer* output);
void ZSTD_seekable_freeCStream(ZSTD_seekable_CStream* zcs);

#ifdef __cplusplus
}
#endif
