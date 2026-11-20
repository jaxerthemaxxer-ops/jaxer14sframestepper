#include "StatsPopup.hpp"

#include "Picker.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>
#include <algorithm>

using namespace geode::prelude;

bool StatsPopup::init(macro::Replay const& replay, macro::Stats stats, double parseMs) {
    if (!Popup::init(380.f, 250.f)) return false;

    m_stats = std::move(stats);
    auto const& s = m_stats;

    this->setTitle(replay.levelName.empty() ? std::string("Macro Jump Counter") : replay.levelName);

    // The two headline numbers.
    auto addHero = [this](std::string const& number, char const* caption, float x) {
        auto num = CCLabelBMFont::create(number.c_str(), "goldFont.fnt");
        num->limitLabelWidth(150.f, 1.f, .3f);
        m_mainLayer->addChildAtPosition(num, Anchor::Center, ccp(x, 62));

        auto label = CCLabelBMFont::create(caption, "chatFont.fnt");
        label->setColor({ 180, 180, 180 });
        m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(x, 36));
    };
    addHero(macro::withCommas(s.clicks), "Jump inputs (clicks)", -90);
    addHero(macro::withCommas(s.framesHeld), "Frames holding jump", 90);

    auto addLine = [this](std::string const& text, float x, float y) {
        auto label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        label->setScale(.8f);
        m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(x, y), ccp(0.f, .5f));
    };

    uint64_t gap = 0;
    bool hasGap = false;
    for (auto const& p : s.players) {
        if (p.hasGap && (!hasGap || p.minGap < gap)) {
            gap = p.minGap;
            hasGap = true;
        }
    }
    uint64_t longest = std::max(s.players[0].longestHold, s.players[1].longestHold);
    uint64_t releases = s.players[0].releases + s.players[1].releases;

    addLine(fmt::format("Length: {} frames ({:.2f} s)", macro::withCommas(s.lastFrame), s.seconds), -170, 8);
    addLine(fmt::format("Average clicks per second: {:.2f}", s.avgPerSecond), -170, -10);
    addLine(fmt::format("Peak clicks in one second: {}", s.peakPerSecond), -170, -28);
    addLine(hasGap ? fmt::format("Shortest gap between clicks: {} frames", gap) : std::string("Shortest gap between clicks: n/a"), -170, -46);

    addLine(fmt::format("Longest single hold: {} frames", macro::withCommas(longest)), 25, 8);
    addLine(fmt::format("Releases: {}", macro::withCommas(releases)), 25, -10);
    addLine(fmt::format("Player 1 / Player 2 clicks: {} / {}", macro::withCommas(s.players[0].clicks), macro::withCommas(s.players[1].clicks)), 25, -28);
    addLine(fmt::format("Ticks per second: {:.0f}", s.tps), 25, -46);

    auto footer = CCLabelBMFont::create(
        fmt::format("{} v{}  |  {} inputs read in {:.2f} ms",
            replay.botName.empty() ? std::string("Unknown bot") : replay.botName,
            replay.botVersion, macro::withCommas(replay.frame.size()), parseMs).c_str(),
        "chatFont.fnt"
    );
    footer->setScale(.6f);
    footer->setColor({ 150, 150, 150 });
    m_mainLayer->addChildAtPosition(footer, Anchor::Bottom, ccp(0, 14));

    auto copySpr = ButtonSprite::create("Copy frames", "goldFont.fnt", "GJ_button_01.png", .8f);
    copySpr->setScale(.75f);
    auto copyBtn = CCMenuItemSpriteExtra::create(copySpr, this, menu_selector(StatsPopup::onCopy));
    m_buttonMenu->addChildAtPosition(copyBtn, Anchor::Bottom, ccp(-70, 42));

    auto otherSpr = ButtonSprite::create("Open another", "goldFont.fnt", "GJ_button_01.png", .8f);
    otherSpr->setScale(.75f);
    auto otherBtn = CCMenuItemSpriteExtra::create(otherSpr, this, menu_selector(StatsPopup::onOther));
    m_buttonMenu->addChildAtPosition(otherBtn, Anchor::Bottom, ccp(70, 42));

    return true;
}

void StatsPopup::onCopy(CCObject*) {
    if (geode::utils::clipboard::write(macro::joinFrames(m_stats.allClicks))) {
        Notification::create("Click frames copied", NotificationIcon::Success)->show();
    }
    else {
        Notification::create("Could not copy to the clipboard", NotificationIcon::Error)->show();
    }
}

void StatsPopup::onOther(CCObject*) {
    this->onClose(nullptr);
    pickAndShowMacro();
}

StatsPopup* StatsPopup::create(macro::Replay const& replay, macro::Stats stats, double parseMs) {
    auto ret = new StatsPopup();
    if (ret->init(replay, std::move(stats), parseMs)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
