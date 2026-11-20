#include "Picker.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

// Adds a "JC" button to the main menu. Clicking it opens the macro picker.
class $modify(MacroMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto* menu = this->getChildByID("bottom-menu");
        if (!menu) return true;

        auto* label = CCLabelBMFont::create("JC", "bigFont.fnt");
        auto* sprite = CircleButtonSprite::create(label, CircleBaseColor::Blue, CircleBaseSize::MediumAlt);
        auto* button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(MacroMenuLayer::onMacroCounter));
        button->setID("macro-counter-button"_spr);

        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onMacroCounter(CCObject*) {
        pickAndShowMacro();
    }
};
