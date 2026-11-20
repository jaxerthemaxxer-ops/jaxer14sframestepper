// Native test for the macro reader. Build: g++ -O2 -std=c++20 -Isrc test/test_gdr2.cpp src/Gdr2.cpp -o test_gdr2
#include "Gdr2.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); failures++; } } while (0)

int main(int argc, char** argv) {
    if (argc > 1) {
        std::ifstream f(argv[1], std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        macro::Replay r; std::string err;
        auto t0 = std::chrono::steady_clock::now();
        bool ok = macro::parse(bytes, r, err);
        auto s = ok ? macro::analyze(r) : macro::Stats{};
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        if (!ok) { std::printf("parse error: %s\n", err.c_str()); return 1; }
        std::printf("bot=%s v%llu tps=%.0f inputs=%zu\n", r.botName.c_str(), (unsigned long long)r.botVersion, r.framerate, r.frame.size());
        std::printf("clicks=%llu held=%llu lastFrame=%llu seconds=%.2f avg=%.2f peak=%llu minGap=%llu longest=%llu releases=%llu p2=%llu\n",
            (unsigned long long)s.clicks, (unsigned long long)s.framesHeld, (unsigned long long)s.lastFrame, s.seconds, s.avgPerSecond,
            (unsigned long long)s.peakPerSecond, (unsigned long long)s.players[0].minGap, (unsigned long long)s.players[0].longestHold,
            (unsigned long long)(s.players[0].releases + s.players[1].releases), (unsigned long long)s.players[1].clicks);
        std::printf("parsed+analyzed in %.3f ms\n", ms);
    }

    // Error handling
    { macro::Replay r; std::string e; std::vector<uint8_t> b{ 1, 2, 3, 4 }; CHECK(!macro::parse(b, r, e)); CHECK(!e.empty()); }
    { macro::Replay r; std::string e; std::vector<uint8_t> b{ 'G', 'D', 'R', 1, 0 }; CHECK(!macro::parse(b, r, e)); CHECK(e.find("version 1") != std::string::npos); }
    { macro::Replay r; std::string e; std::vector<uint8_t> b{ 'G', 'D', 'R', 2, 0, 0 }; CHECK(!macro::parse(b, r, e)); }

    // Formatting
    CHECK(macro::withCommas(0) == "0");
    CHECK(macro::withCommas(999) == "999");
    CHECK(macro::withCommas(1000) == "1,000");
    CHECK(macro::withCommas(19325) == "19,325");
    CHECK(macro::withCommas(1234567) == "1,234,567");
    CHECK(macro::joinFrames({ 190, 2100 }) == "190, 2100");

    // Speed: 3 million inputs
    {
        std::vector<uint8_t> out{ 'G', 'D', 'R' };
        auto w = [&](uint64_t v) { while (v >= 128) { out.push_back((v & 127) | 128); v >>= 7; } out.push_back((uint8_t)v); };
        w(2); w(0); w(0); w(0);
        for (uint8_t b : { 0x41, 0xf0, 0x00, 0x00 }) out.push_back(b);
        w(22074);
        for (uint8_t b : { 0x40, 0x6e, 0, 0, 0, 0, 0, 0 }) out.push_back(b);
        for (int i = 0; i < 10; i++) w(0);
        const uint64_t N = 3000000; w(N); w(N);
        for (uint64_t i = 0; i < N; i++) w(((i % 7) + 1) * 2 + (i % 2 ? 0 : 1));
        macro::Replay r; std::string e;
        auto t0 = std::chrono::steady_clock::now();
        bool ok = macro::parse(out, r, e);
        auto s = macro::analyze(r);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        CHECK(ok); CHECK(r.frame.size() == N); CHECK(s.clicks == N / 2);
        std::printf("3M inputs parsed+analyzed in %.1f ms\n", ms);
    }

    std::printf(failures ? "%d test(s) FAILED\n" : "All tests passed\n", failures);
    return failures ? 1 : 0;
}
