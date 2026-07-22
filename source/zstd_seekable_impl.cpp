// ZSTD Seekable Stream Implementation for Nintendo Switch (devkitPro / libnx)
// Official ZSTD Seekable Stream Protocol Specification

#include "zstd_seekable.h"
#include <zstd.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <algorithm>

struct seekEntry_t {
    uint32_t cSize;
    uint32_t dSize;
};

struct ZSTD_seekable_CStream_s {
    ZSTD_CCtx* cctx = nullptr;
    unsigned maxFrameSize = 256 * 1024;
    size_t frameBuffered = 0;
    uint32_t currentFrameCSize = 0;
    int checksumFlag = 0;
    int clevel = 3;

    std::vector<seekEntry_t> entries;

    ZSTD_seekable_CStream_s() {
        cctx = ZSTD_createCCtx();
    }

    ~ZSTD_seekable_CStream_s() {
        if (cctx) {
            ZSTD_freeCCtx(cctx);
        }
    }
};

extern "C" {

ZSTD_seekable_CStream* ZSTD_seekable_createCStream(void) {
    return new ZSTD_seekable_CStream();
}

size_t ZSTD_seekable_initCStream(ZSTD_seekable_CStream* zcs, int compressionLevel, int checksumFlag, unsigned maxFrameSize) {
    if (!zcs) return (size_t)-1;
    zcs->maxFrameSize = (maxFrameSize > 0) ? maxFrameSize : (256 * 1024);
    zcs->checksumFlag = checksumFlag;
    zcs->clevel = compressionLevel;
    zcs->frameBuffered = 0;
    zcs->currentFrameCSize = 0;
    zcs->entries.clear();

    ZSTD_CCtx_reset(zcs->cctx, ZSTD_reset_session_only);
    ZSTD_CCtx_setParameter(zcs->cctx, ZSTD_c_compressionLevel, zcs->clevel);
    if (zcs->checksumFlag) {
        ZSTD_CCtx_setParameter(zcs->cctx, ZSTD_c_checksumFlag, 1);
    }
    return 0;
}

size_t ZSTD_seekable_compressStream(ZSTD_seekable_CStream* zcs, ZSTD_outBuffer* output, ZSTD_inBuffer* input) {
    if (!zcs || !output || !input) return (size_t)-1;

    while (input->pos < input->size) {
        size_t roomInFrame = zcs->maxFrameSize - zcs->frameBuffered;
        size_t available = input->size - input->pos;
        size_t toProcess = std::min(roomInFrame, available);

        ZSTD_inBuffer in2 = { input->src, input->pos + toProcess, input->pos };

        bool endFrame = (zcs->frameBuffered + toProcess == zcs->maxFrameSize);
        ZSTD_EndDirective flush = endFrame ? ZSTD_e_end : ZSTD_e_continue;

        size_t startPos = output->pos;
        size_t ret = ZSTD_compressStream2(zcs->cctx, output, &in2, flush);

        size_t bytesWritten = output->pos - startPos;
        zcs->currentFrameCSize += static_cast<uint32_t>(bytesWritten);
        size_t consumed = in2.pos - input->pos;
        zcs->frameBuffered += consumed;
        input->pos = in2.pos;

        if (ZSTD_isError(ret)) {
            return ret;
        }

        if (endFrame && ret == 0) {
            zcs->entries.push_back({ zcs->currentFrameCSize, static_cast<uint32_t>(zcs->frameBuffered) });
            zcs->currentFrameCSize = 0;
            zcs->frameBuffered = 0;

            ZSTD_CCtx_reset(zcs->cctx, ZSTD_reset_session_only);
            ZSTD_CCtx_setParameter(zcs->cctx, ZSTD_c_compressionLevel, zcs->clevel);
            if (zcs->checksumFlag) {
                ZSTD_CCtx_setParameter(zcs->cctx, ZSTD_c_checksumFlag, 1);
            }
        }

        if (output->pos >= output->size) {
            break;
        }
    }

    return zcs->maxFrameSize - zcs->frameBuffered;
}

size_t ZSTD_seekable_endStream(ZSTD_seekable_CStream* zcs, ZSTD_outBuffer* output) {
    if (!zcs || !output) return (size_t)-1;

    // 1. Flush remaining partial frame if present
    if (zcs->frameBuffered > 0) {
        ZSTD_inBuffer dummyIn = { nullptr, 0, 0 };
        size_t startPos = output->pos;
        size_t ret = ZSTD_compressStream2(zcs->cctx, output, &dummyIn, ZSTD_e_end);
        
        size_t bytesWritten = output->pos - startPos;
        zcs->currentFrameCSize += static_cast<uint32_t>(bytesWritten);

        if (ZSTD_isError(ret)) {
            return ret;
        }

        if (ret == 0) {
            zcs->entries.push_back({ zcs->currentFrameCSize, static_cast<uint32_t>(zcs->frameBuffered) });
            zcs->currentFrameCSize = 0;
            zcs->frameBuffered = 0;
        } else {
            return ret; // Output buffer full, need to call endStream again
        }
    }

    // 2. Append Seek Table Skippable Frame Header + Entries + Footer
    uint32_t numFrames = static_cast<uint32_t>(zcs->entries.size());
    uint32_t seekTablePayloadSize = numFrames * 8 + 9;
    uint32_t skippableHeaderSize = 8;
    uint32_t totalSeekTableSize = skippableHeaderSize + seekTablePayloadSize;

    if (output->size - output->pos < totalSeekTableSize) {
        return totalSeekTableSize; // Need more buffer space
    }

    uint8_t* dst = static_cast<uint8_t*>(output->dst) + output->pos;
    size_t pos = 0;

    // A. Skippable Frame Header (0x184D2A5E)
    uint32_t skippableMagic = 0x184D2A5E;
    memcpy(dst + pos, &skippableMagic, 4); pos += 4;
    memcpy(dst + pos, &seekTablePayloadSize, 4); pos += 4;

    // B. Seek Table Entries
    for (const auto& entry : zcs->entries) {
        uint32_t cSz = entry.cSize;
        uint32_t dSz = entry.dSize;
        memcpy(dst + pos, &cSz, 4); pos += 4;
        memcpy(dst + pos, &dSz, 4); pos += 4;
    }

    // C. Seek Table Footer (0x8F92EAB1 when checksumFlag=1, 0x8F925F1B when checksumFlag=0)
    uint8_t flags = zcs->checksumFlag ? 1 : 0;
    uint32_t seekMagic = zcs->checksumFlag ? 0x8F92EAB1 : 0x8F925F1B;
    memcpy(dst + pos, &numFrames, 4); pos += 4;
    dst[pos] = flags; pos += 1;
    memcpy(dst + pos, &seekMagic, 4); pos += 4;

    output->pos += pos;
    return 0;
}

void ZSTD_seekable_freeCStream(ZSTD_seekable_CStream* zcs) {
    delete zcs;
}

} // extern "C"
