#pragma once

#include "Gdr2.hpp"

#include <Geode/ui/Popup.hpp>

class StatsPopup : public geode::Popup {
protected:
    macro::Stats m_stats;

    bool init(macro::Replay const& replay, macro::Stats stats, double parseMs);

    void onCopy(cocos2d::CCObject*);
    void onOther(cocos2d::CCObject*);

public:
    static StatsPopup* create(macro::Replay const& replay, macro::Stats stats, double parseMs);
};
