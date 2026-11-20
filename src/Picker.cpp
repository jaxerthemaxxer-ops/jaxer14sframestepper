#include "Picker.hpp"

#include "Gdr2.hpp"
#include "StatsPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>
#include <chrono>

using namespace geode::prelude;

namespace {
    void showError(std::string const& message) {
        FLAlertLayer::create("Macro Jump Counter", message, "OK")->show();
    }
}

void pickAndShowMacro() {
    async::spawn(file::pick(file::PickMode::OpenFile, file::FilePickOptions {
        .filters = { file::FilePickOptions::Filter {
            .description = "GD macros (.gdr2)",
            .files = { "*.gdr2" },
        }}
    }), [](file::PickResult result) {
        if (result.isErr()) {
            showError("Could not open the file picker: " + result.unwrapErr());
            return;
        }

        auto picked = std::move(result).unwrap();
        if (!picked.has_value()) return; // cancelled

        auto data = file::readBinary(*picked);
        if (data.isErr()) {
            showError("Could not read that file: " + data.unwrapErr());
            return;
        }

        macro::Replay replay;
        std::string error;
        auto start = std::chrono::steady_clock::now();

        if (!macro::parse(data.unwrap(), replay, error)) {
            showError(error);
            return;
        }
        auto stats = macro::analyze(replay);

        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        StatsPopup::create(replay, std::move(stats), ms)->show();
    });
}
