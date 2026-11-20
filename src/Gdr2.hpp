#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

// Reader and analyzer for GDR2 replay files (the .gdr2 macro format).
// This file has no Geode or Geometry Dash dependency, so it can be tested on its own.
namespace macro {

    struct Replay {
        uint64_t version = 0;
        std::string inputTag;
        std::string author;
        std::string description;
        float duration = 0.f;
        uint64_t gameVersion = 0;
        double framerate = 240.0;
        uint64_t seed = 0;
        uint64_t coins = 0;
        bool ldm = false;
        bool platformer = false;
        std::string botName;
        uint64_t botVersion = 0;
        uint64_t levelId = 0;
        std::string levelName;
        std::vector<uint64_t> deaths;

        // Inputs are stored as parallel arrays, which keeps huge macros fast.
        std::vector<uint64_t> frame;
        std::vector<uint8_t> down;
        std::vector<uint8_t> button;   // 1 = jump, 2 = left, 3 = right
        std::vector<uint8_t> player2;
    };

    struct PlayerStats {
        uint64_t clicks = 0;
        uint64_t releases = 0;
        uint64_t framesHeld = 0;
        uint64_t longestHold = 0;
        uint64_t minGap = 0;
        bool hasGap = false;
        uint64_t left = 0;
        uint64_t right = 0;
        std::vector<uint64_t> clickFrames;
    };

    struct Stats {
        PlayerStats players[2];
        uint64_t clicks = 0;
        uint64_t framesHeld = 0;
        uint64_t lastFrame = 0;
        uint64_t peakPerSecond = 0;
        double tps = 240.0;
        double seconds = 0.0;
        double avgPerSecond = 0.0;
        std::vector<uint64_t> allClicks; // both players, sorted
    };

    /// Reads a .gdr2 file. Returns false and fills `error` if the data is not a valid replay.
    bool parse(std::span<const uint8_t> bytes, Replay& out, std::string& error);

    /// Counts jumps, held frames, click rate and more.
    Stats analyze(Replay const& replay);

    /// "190, 2100, 3163" style list of every click frame.
    std::string joinFrames(std::vector<uint64_t> const& frames);

    /// 19325 -> "19,325"
    std::string withCommas(uint64_t value);
}
