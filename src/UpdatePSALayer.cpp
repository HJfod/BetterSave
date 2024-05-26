#include "UpdatePSALayer.hpp"
#include <Geode/ui/General.hpp>

bool UpdatePSALayer::init() {
    if (!CCLayer::init())
        return false;
    
    this->addChildAtPosition(createLayerBG(), Anchor::Center);

    m_pageContainer = CCNode::create();
    m_pageContainer->setContentSize({ 300, 250 });

    m_pages = CCNode::create();

    auto createPage = [&](bool last = false) {
        auto menu = CCMenu::create();
        menu->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoGrowAxis(50)
        );
        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Next", "bigFont.fnt", "GJ_button_01.png", .8f),
            this, last ? 
                menu_selector(UpdatePSALayer::onEnd) : 
                menu_selector(UpdatePSALayer::onNextPage)
        );
        btn->setZOrder(10);
        menu->addChild(btn);
        m_pages->addChild(menu);
        return menu;
    };

    {
        auto page = createPage();

        auto title = CCLabelBMFont::create("Important Announcement", "goldFont.fnt");
        page->addChild(title);

        auto logo = CCSprite::create("hjfod.bettersave.png");
        page->addChild(logo);

        auto text = TextArea::create(
            "This is an important announcement regarding the <cy>BetterSave</c> mod. "
            "Be sure to read it thoroughly - <cr>you might lose your levels if you don't</c>!",
            "bigFont.fnt",
            .7f, 250, { 1, 1 }, 25, false
        );
        page->addChild(text);

        page->updateLayout();
    }

    {
        auto page = createPage(true);

        auto title = CCLabelBMFont::create("Thank you for reading.", "goldFont.fnt");
        page->addChild(title);

        auto text = TextArea::create(
            "This announcement will stay visible in the main menu as a reminder about "
            "the issue.",
            "bigFont.fnt",
            .7f, 250, { 1, 1 }, 25, false
        );
        page->addChild(text);

        page->updateLayout();
    }

    m_pages->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoGrowAxis(50)
    );
    m_pages->setPositionY(m_pages->getContentHeight() - m_pageContainer->getContentHeight());
    m_pageContainer->addChild(m_pages);

    this->addChildAtPosition(m_pageContainer, Anchor::Center);

    return true;
}

void UpdatePSALayer::onNextPage(CCObject* sender) {
    static_cast<CCMenuItemSpriteExtra*>(sender)->setEnabled(false);
    m_pages->runAction(CCEaseElasticInOut::create(
        CCMoveBy::create(.5f, ccp(0, -m_pageContainer->getContentHeight())),
        1.f
    ));
}
void UpdatePSALayer::onEnd(CCObject*) {
    switchToScene(MenuLayer::create());
}

UpdatePSALayer* UpdatePSALayer::create() {
    auto ret = new UpdatePSALayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
