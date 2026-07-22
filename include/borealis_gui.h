// Borealis Native Horizon UI for 3DS to ZCCI Converter (Nintendo Switch)
#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include "zcci_compressor.h"

namespace SwitchZCCI {

struct GameFile {
    std::string path;
    std::string filename;
    uint64_t size_bytes;
};

struct ProgressState {
    std::atomic<uint64_t> current_bytes{0};
    std::atomic<uint64_t> total_bytes{0};
    std::atomic<bool> is_running{false};
    std::atomic<bool> cancel_requested{false};
    std::atomic<CompressResult> result{CompressResult::Success};
};

/**
 * Initializes and starts the Borealis Native Switch GUI.
 */
void RunBorealisGUI();

/**
 * Formats byte counts into human-readable strings (KB, MB, GB).
 */
std::string FormatSize(uint64_t bytes);

} // namespace SwitchZCCI
