#include "TrashcanPopup.hpp"
#include <Geode/ui/ScrollLayer.hpp>
#include <fmt/chrono.h>

static std::string toAgoString(Trashed::TimePoint const& time) {
    auto const fmtPlural = [](auto count, auto unit) {
        if (count == 1) {
            return fmt::format("{} {} ago", count, unit);
        }
        return fmt::format("{} {}s ago", count, unit);
    };
    auto now = Trashed::Clock::now();
    auto len = std::chrono::duration_cast<std::chrono::minutes>(now - time).count();
    if (len < 60) {
        return fmtPlural(len, "minute");
    }
    len = std::chrono::duration_cast<std::chrono::hours>(now - time).count();
    if (len < 24) {
        return fmtPlural(len, "hour");
    }
    len = std::chrono::duration_cast<std::chrono::days>(now - time).count();
    if (len < 31) {
        return fmtPlural(len, "day");
    }
    return fmt::format("{:%b %d %Y}", std::chrono::clock_cast<std::chrono::system_clock>(time));
}

bool TrashcanPopup::setup() {
    this->setTitle("Trashcan");

    auto trashcanSpr = CCSprite::createWithSpriteFrameName("edit_delBtn_001.png");
    trashcanSpr->setScale(.7f);
    m_mainLayer->addChildAtPosition(trashcanSpr, Anchor::Top, ccp(-55, -20));

    m_scrollingLayer = ScrollLayer::create({ 300, 200 });
    m_scrollingLayer->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoGrowAxis(m_scrollingLayer->getContentHeight())
            ->setAxisAlignment(AxisAlignment::End)
            ->setGap(0)
    );
    m_mainLayer->addChildAtPosition(m_scrollingLayer, Anchor::Center, -m_scrollingLayer->getContentSize() / 2);

    auto border = ListBorders::create();
    border->setContentSize(m_scrollingLayer->getContentSize());
    m_mainLayer->addChildAtPosition(border, Anchor::Center);

    auto deleteAllSpr = CCSprite::createWithSpriteFrameName("GJ_resetBtn_001.png");
    auto deleteAllBtn = CCMenuItemSpriteExtra::create(
        deleteAllSpr, this, menu_selector(TrashcanPopup::onDeleteAll)
    );
    m_buttonMenu->addChildAtPosition(deleteAllBtn, Anchor::BottomLeft, ccp(20, 20));

    m_listener.bind([this](auto*) {
        this->updateList();
        return ListenerResult::Propagate;
    });
    this->updateList();
    
    return true;
}

void TrashcanPopup::updateList() {
    m_scrollingLayer->m_contentLayer->removeAllChildren();
    auto items = Trashed::load();
    if (items.empty()) {
        return this->onClose(nullptr);
    }
    for (auto gmd : items) {
        auto node = CCNode::create();
        node->setContentSize({ m_scrollingLayer->getContentWidth(), 38 });

        auto separator = CCLayerColor::create({ 0, 0, 0, 90 }, node->getContentWidth(), 1);
        separator->ignoreAnchorPointForPosition(false);
        separator->setOpacity(90);
        node->addChildAtPosition(separator, Anchor::Bottom);

        auto title = CCLabelBMFont::create(gmd->getName().c_str(), "bigFont.fnt");
        title->setScale(.5f);
        title->setAnchorPoint({ 0, .5f });
        if (gmd->asList()) {
            title->setColor({ 0, 255, 0 });
        }
        node->addChildAtPosition(title, Anchor::Left, ccp(10, 8));

        auto objCount = CCLabelBMFont::create(fmt::format("Trashed {}", toAgoString(gmd->getTrashTime())).c_str(), "goldFont.fnt");
        objCount->setScale(.4f);
        objCount->setAnchorPoint({ 0, .5f });
        node->addChildAtPosition(objCount, Anchor::Left, ccp(10, -8));

        auto actionsMenu = CCMenu::create();
        actionsMenu->setContentWidth(m_scrollingLayer->getContentWidth() / 2);

        auto restoreSpr = CCSprite::createWithSpriteFrameName("GJ_undoBtn_001.png");
        auto restoreBtn = CCMenuItemSpriteExtra::create(
            restoreSpr, this, menu_selector(TrashcanPopup::onRestore)
        );
        restoreBtn->setUserObject(gmd);
        actionsMenu->addChild(restoreBtn);

        auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        auto deleteBtn = CCMenuItemSpriteExtra::create(
            deleteSpr, this, menu_selector(TrashcanPopup::onDelete)
        );
        deleteBtn->setLayoutOptions(AxisLayoutOptions::create()->setRelativeScale(.95f));
        deleteBtn->setUserObject(gmd);
        actionsMenu->addChild(deleteBtn);

        auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoSpr, this, menu_selector(TrashcanPopup::onInfo)
        );
        infoBtn->setUserObject(gmd);
        actionsMenu->addChild(infoBtn);

        actionsMenu->setLayout(
            RowLayout::create()
                ->setAxisAlignment(AxisAlignment::End)
                ->setAxisReverse(true)
                ->setDefaultScaleLimits(.1f, .65f)
                ->setGap(10)
        );
        actionsMenu->setAnchorPoint({ 1, .5f });
        node->addChildAtPosition(actionsMenu, Anchor::Right, ccp(-10, 0));
        
        m_scrollingLayer->m_contentLayer->addChild(node);
    }
    m_scrollingLayer->m_contentLayer->updateLayout();

    // This is because updating the LevelBrowserLayer underneath causes it to take touch priority
    handleTouchPriority(this);
}

void TrashcanPopup::onInfo(CCObject* sender) {
    auto obj = static_cast<Trashed*>(static_cast<CCNode*>(sender)->getUserObject());
    if (auto level = obj->asLevel()) {
        FLAlertLayer::create(
            "Level Info",
            fmt::format(
                "<cb>Objects</c>: {}\n"
                "<co>Length</c>: {}\n"
                "<cp>Time in Editor</c>: {}\n",
                level->m_objectCount.value(),
                level->lengthKeyToString(level->m_levelLength),
                level->m_workingTime
            ),
            "OK"
        )->show();
    }
    else if (auto list = obj->asList()) {
        FLAlertLayer::create(
            "List Info",
            fmt::format(
                "<cb>Levels</c>: {}\n",
                list->m_levels.size()
            ),
            "OK"
        )->show();
    }
}
void TrashcanPopup::onDelete(CCObject* sender) {
    auto obj = static_cast<Trashed*>(static_cast<CCNode*>(sender)->getUserObject());
    createQuickPopup(
        "Permanently Delete",
        fmt::format(
            "Are you sure you want to <cr>permanently delete</c> <cy>{}</c>?\n"
            "<cr>This can NOT be undone!</c>",
            obj->getName()
        ),
        "Cancel", "Delete",
        [obj = Ref(obj)](auto*, bool btn2) {
            if (btn2) {
                auto res = obj->KABOOM();
                if (!res) {
                    FLAlertLayer::create(
                        "Failed to Delete",
                        fmt::format("Failed to permanently delete <cy>{}</c>: {}", obj->getName(), res.unwrapErr()),
                        "OK"
                    )->show();
                }
            }
        }
    );
}
void TrashcanPopup::onRestore(CCObject* sender) {
    auto obj = static_cast<Trashed*>(static_cast<CCNode*>(sender)->getUserObject());
    auto res = obj->untrash();
    if (!res) {
        FLAlertLayer::create(
            "Failed to Restore",
            fmt::format("Failed to restore <cy>{}</c>: {}", obj->getName(), res.unwrapErr()),
            "OK"
        )->show();
    }
}
void TrashcanPopup::onDeleteAll(CCObject*) {
    createQuickPopup(
        "Clear Trashcan",
        "Are you sure you want to <co>clear the Trashcan</c>?\n"
        "<cr>This will PERMANENTLY delete ALL levels in the trash!</c>",
        "Cancel", "Delete All",
        [](auto*, bool btn2) {
            if (btn2) {
                std::error_code ec;
                std::filesystem::remove_all(getTrashDir(), ec);
                if (ec) {
                    FLAlertLayer::create(
                        "Failed to Clear",
                        fmt::format("Failed to clear trash: {} (code {})", ec.message(), ec.value()),
                        "OK"
                    )->show();
                }
            }
        }
    );
}

TrashcanPopup* TrashcanPopup::create() {
    auto ret = new TrashcanPopup();
    if (ret && ret->initAnchored(350, 270)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
