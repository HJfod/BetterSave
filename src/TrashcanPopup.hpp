#pragma once

#include <Geode/ui/Popup.hpp>
#include "../include/LocalLevels.hpp"

using namespace geode::prelude;

class TrashcanPopup : public Popup<> {
protected:
    ScrollLayer* m_scrollingLayer;
    EventListener<EventFilter<save::TrashLevelEvent>> m_listener;

    bool setup() override;
    void updateList();

    void onInfo(CCObject* sender);
    void onDelete(CCObject* sender);
    void onRestore(CCObject* sender);
    void onDeleteAll(CCObject* sender);

public:
    static TrashcanPopup* create();
};
