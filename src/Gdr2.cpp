#include "Gdr2.hpp"

#include <algorithm>
#include <cstring>

namespace macro {

    namespace {
        struct Reader {
            std::span<const uint8_t> data;
            size_t pos = 0;
            bool ok = true;

            bool atEnd() const { return pos >= data.size(); }
            size_t left() const { return data.size() - pos; }

            uint64_t varint() {
                uint64_t result = 0;
                for (int i = 0; i < 10; i++) {
                    if (pos >= data.size()) { ok = false; return 0; }
                    uint8_t b = data[pos++];
                    result |= static_cast<uint64_t>(b & 0x7f) << (7 * i);
                    if ((b & 0x80) == 0) return result;
                }
                ok = false;
                return 0;
            }

            std::string str() {
                uint64_t len = varint();
                if (!ok || len > left()) { ok = false; return {}; }
                std::string s(reinterpret_cast<const char*>(data.data() + pos), static_cast<size_t>(len));
                pos += static_cast<size_t>(len);
                return s;
            }

            // Floats and doubles are stored big-endian.
            float f32() {
                if (left() < 4) { ok = false; return 0.f; }
                uint32_t v = 0;
                for (int i = 0; i < 4; i++) v = (v << 8) | data[pos++];
                float f;
                std::memcpy(&f, &v, sizeof f);
                return f;
            }

            double f64() {
                if (left() < 8) { ok = false; return 0.0; }
                uint64_t v = 0;
                for (int i = 0; i < 8; i++) v = (v << 8) | data[pos++];
                double d;
                std::memcpy(&d, &v, sizeof d);
                return d;
            }
        };
    }

    bool parse(std::span<const uint8_t> bytes, Replay& r, std::string& error) {
        r = Replay{};

        if (bytes.size() < 4 || bytes[0] != 'G' || bytes[1] != 'D' || bytes[2] != 'R') {
            error = "This is not a GDR replay (missing the GDR header).";
            return false;
        }

        Reader in{ bytes, 3 };

        r.version = in.varint();
        if (!in.ok) { error = "The file ended unexpectedly."; return false; }
        if (r.version != 2) {
            error = "This is GDR version " + std::to_string(r.version) + ". Only .gdr2 files are supported.";
            return false;
        }

        r.inputTag = in.str();
        r.author = in.str();
        r.description = in.str();
        r.duration = in.f32();
        r.gameVersion = in.varint();
        r.framerate = in.f64();
        r.seed = in.varint();
        r.coins = in.varint();
        r.ldm = in.varint() != 0;
        r.platformer = in.varint() != 0;
        r.botName = in.str();
        r.botVersion = in.varint();
        r.levelId = in.varint();
        r.levelName = in.str();

        uint64_t extensionSize = in.varint();
        if (!in.ok || extensionSize > in.left()) { error = "The replay header is damaged."; return false; }
        in.pos += static_cast<size_t>(extensionSize);

        uint64_t deathCount = in.varint();
        if (!in.ok || deathCount > in.left()) { error = "The death list is damaged."; return false; }
        r.deaths.reserve(static_cast<size_t>(deathCount));
        uint64_t acc = 0;
        for (uint64_t i = 0; i < deathCount; i++) {
            acc += in.varint();
            r.deaths.push_back(acc);
        }

        uint64_t total = in.varint();
        uint64_t p1Left = in.varint();
        if (!in.ok) { error = "The input list is damaged."; return false; }

        // Every input takes at least one byte, so this also guards against absurd sizes.
        size_t reserve = static_cast<size_t>(std::min<uint64_t>(total, in.left()));
        r.frame.reserve(reserve);
        r.down.reserve(reserve);
        r.button.reserve(reserve);
        r.player2.reserve(reserve);

        bool hasInputExtension = !r.inputTag.empty();
        bool onPlayer2 = p1Left == 0;
        acc = 0;

        while (!in.atEnd() && r.frame.size() < total) {
            uint64_t packed = in.varint();
            if (!in.ok) { error = "The input list ended unexpectedly."; return false; }

            uint64_t delta;
            uint8_t btn;
            if (r.platformer) {
                delta = packed >> 3;
                btn = static_cast<uint8_t>((packed >> 1) & 3);
            } else {
                delta = packed >> 1;
                btn = 1;
            }
            acc += delta;

            r.frame.push_back(acc);
            r.down.push_back(static_cast<uint8_t>(packed & 1));
            r.button.push_back(btn);
            r.player2.push_back(onPlayer2 ? 1 : 0);

            if (hasInputExtension) {
                uint64_t size = in.varint();
                if (!in.ok || size > in.left()) { error = "An input extension is damaged."; return false; }
                in.pos += static_cast<size_t>(size);
            }

            // Player 1's inputs come first, then player 2's, each starting from frame 0.
            if (p1Left > 0 && --p1Left == 0) {
                acc = 0;
                onPlayer2 = true;
            }
        }

        return true;
    }

    Stats analyze(Replay const& r) {
        Stats s;
        s.tps = r.framerate > 0.0 ? r.framerate : 240.0;

        int64_t lastDown[2][4] = { { -1, -1, -1, -1 }, { -1, -1, -1, -1 } };
        int64_t lastClick[2] = { -1, -1 };

        for (size_t i = 0; i < r.frame.size(); i++) {
            int pl = r.player2[i] ? 1 : 0;
            uint8_t b = r.button[i];
            uint64_t f = r.frame[i];
            PlayerStats& p = s.players[pl];

            s.lastFrame = std::max(s.lastFrame, f);

            if (r.down[i]) {
                if (b == 1) {
                    p.clicks++;
                    p.clickFrames.push_back(f);
                    if (lastClick[pl] >= 0) {
                        uint64_t gap = f - static_cast<uint64_t>(lastClick[pl]);
                        if (!p.hasGap || gap < p.minGap) { p.minGap = gap; p.hasGap = true; }
                    }
                    lastClick[pl] = static_cast<int64_t>(f);
                }
                else if (b == 2) p.left++;
                else if (b == 3) p.right++;
                lastDown[pl][b] = static_cast<int64_t>(f);
            }
            else {
                p.releases++;
                if (b == 1 && lastDown[pl][b] >= 0) {
                    uint64_t hold = f - static_cast<uint64_t>(lastDown[pl][b]);
                    p.framesHeld += hold;
                    p.longestHold = std::max(p.longestHold, hold);
                }
                lastDown[pl][b] = -1;
            }
        }

        // A jump still held when the macro ends counts up to the last frame.
        for (int pl = 0; pl < 2; pl++) {
            if (lastDown[pl][1] >= 0) {
                uint64_t hold = s.lastFrame - static_cast<uint64_t>(lastDown[pl][1]);
                s.players[pl].framesHeld += hold;
                s.players[pl].longestHold = std::max(s.players[pl].longestHold, hold);
            }
        }

        s.clicks = s.players[0].clicks + s.players[1].clicks;
        s.framesHeld = s.players[0].framesHeld + s.players[1].framesHeld;
        s.seconds = static_cast<double>(s.lastFrame) / s.tps;
        s.avgPerSecond = s.seconds > 0.0 ? static_cast<double>(s.clicks) / s.seconds : 0.0;

        s.allClicks.resize(s.clicks);
        std::merge(
            s.players[0].clickFrames.begin(), s.players[0].clickFrames.end(),
            s.players[1].clickFrames.begin(), s.players[1].clickFrames.end(),
            s.allClicks.begin()
        );

        // Most clicks inside any one-second window.
        size_t lo = 0;
        uint64_t peak = 0;
        double window = s.tps;
        for (size_t hi = 0; hi < s.allClicks.size(); hi++) {
            while (static_cast<double>(s.allClicks[hi] - s.allClicks[lo]) >= window) lo++;
            peak = std::max<uint64_t>(peak, hi - lo + 1);
        }
        s.peakPerSecond = peak;

        return s;
    }

    std::string joinFrames(std::vector<uint64_t> const& frames) {
        std::string out;
        out.reserve(frames.size() * 7);
        for (size_t i = 0; i < frames.size(); i++) {
            if (i) out += ", ";
            out += std::to_string(frames[i]);
        }
        return out;
    }

    std::string withCommas(uint64_t value) {
        std::string digits = std::to_string(value);
        std::string out;
        int count = 0;
        for (size_t i = digits.size(); i-- > 0;) {
            out.insert(out.begin(), digits[i]);
            if (++count % 3 == 0 && i > 0) out.insert(out.begin(), ',');
        }
        return out;
    }
}
