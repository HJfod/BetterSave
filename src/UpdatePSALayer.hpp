#pragma once

#include <Geode/DefaultInclude.hpp>

using namespace geode::prelude;

class UpdatePSALayer : public CCLayer {
protected:
    CCNode* m_pageContainer;
    CCNode* m_pages;

    bool init();

    void onNextPage(CCObject*);
    void onEnd(CCObject*);

public:
    static UpdatePSALayer* create();
};
