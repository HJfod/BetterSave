#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include "../include/LocalLevels.hpp"

using namespace geode::prelude;

class $modify(TrashBrowserLayer, LevelBrowserLayer) {
    struct Fields {
        EventListener<EventFilter<save::TrashLevelEvent>> listener;
    };

    bool init(GJSearchObject* search) {
        if (!LevelBrowserLayer::init(search))
            return false;
        
        if (search->m_searchType == SearchType::MyLevels) {
            if (auto menu = this->getChildByID("my-levels-menu")) {
                auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
                auto trashBtn = CCMenuItemSpriteExtra::create(
                    trashSpr, this, menu_selector(TrashBrowserLayer::onTrashcan)
                );
                menu->addChild(trashBtn);
                menu->updateLayout();

                auto updateTrashSprite = [trashSpr] {
                    trashSpr->setOpacity(save::getLevelsInTrash().empty() ? 205 : 255);
                    trashSpr->setColor(save::getLevelsInTrash().empty() ? ccc3(90, 90, 90) : ccWHITE);
                };
                m_fields->listener.bind([=](auto*) {
                    updateTrashSprite();
                    // Reload levels list
                    this->loadPage(m_searchObject);
                    return ListenerResult::Propagate;
                });
                updateTrashSprite();
            }

        }

        return true;
    }
    void onTrashcan(CCObject*) {
        save::openTrashcanPopup();
    }
};

// Fix the popup messages claiming deletion is permanent

class $modify(EditLevelLayer) {
    void confirmDelete(CCObject*) {
        auto alert = FLAlertLayer::create(
            this,
            "Delete level", 
            "Are you sure you want to <cr>trash</c> this level?\n"
            "<cy>You can restore the level or permanently delete it through the Trashcan.</c>",
            "No", "Yes",
            340
        );
        alert->setTag(4);
        alert->show();
    }
};
