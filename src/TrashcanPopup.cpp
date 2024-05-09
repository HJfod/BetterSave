#include "TrashcanPopup.hpp"
#include <Geode/ui/ScrollLayer.hpp>
#include <fmt/chrono.h>

static std::string toAgoString(save::TrashTime const& time) {
    auto const fmtPlural = [](auto count, auto unit) {
        if (count == 1) {
            return fmt::format("{} {} ago", count, unit);
        }
        return fmt::format("{} {}s ago", count, unit);
    };
    auto now = save::TrashClock::now();
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
    return fmt::format("{:%b %d %Y}", time);
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
    if (save::getLevelsInTrash().empty()) {
        this->onClose(nullptr);
    }
    for (auto level : save::getLevelsInTrash()) {
        auto node = CCNode::create();
        node->setContentSize({ m_scrollingLayer->getContentWidth(), 38 });

        auto separator = CCLayerColor::create({ 0, 0, 0, 90 }, node->getContentWidth(), 1);
        separator->ignoreAnchorPointForPosition(false);
        separator->setOpacity(90);
        node->addChildAtPosition(separator, Anchor::Bottom);

        auto title = CCLabelBMFont::create(level.getLevel()->m_levelName.c_str(), "bigFont.fnt");
        title->setScale(.5f);
        title->setAnchorPoint({ 0, .5f });
        node->addChildAtPosition(title, Anchor::Left, ccp(10, 8));

        auto objCount = CCLabelBMFont::create(
            fmt::format("Trashed {}", toAgoString(level.getTrashTime())
        ).c_str(), "goldFont.fnt");
        objCount->setScale(.4f);
        objCount->setAnchorPoint({ 0, .5f });
        node->addChildAtPosition(objCount, Anchor::Left, ccp(10, -8));

        auto actionsMenu = CCMenu::create();
        actionsMenu->setContentWidth(m_scrollingLayer->getContentWidth() / 2);

        auto restoreSpr = CCSprite::createWithSpriteFrameName("GJ_undoBtn_001.png");
        auto restoreBtn = CCMenuItemSpriteExtra::create(
            restoreSpr, this, menu_selector(TrashcanPopup::onRestore)
        );
        restoreBtn->setUserObject(level.getLevel());
        actionsMenu->addChild(restoreBtn);

        auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        auto deleteBtn = CCMenuItemSpriteExtra::create(
            deleteSpr, this, menu_selector(TrashcanPopup::onDelete)
        );
        deleteBtn->setLayoutOptions(AxisLayoutOptions::create()->setRelativeScale(.95f));
        deleteBtn->setUserObject(level.getLevel());
        actionsMenu->addChild(deleteBtn);

        auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoSpr, this, menu_selector(TrashcanPopup::onInfo)
        );
        infoBtn->setUserObject(level.getLevel());
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
    auto level = static_cast<GJGameLevel*>(static_cast<CCNode*>(sender)->getUserObject());
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
void TrashcanPopup::onDelete(CCObject* sender) {
    auto level = static_cast<GJGameLevel*>(static_cast<CCNode*>(sender)->getUserObject());
    createQuickPopup(
        "Permanently Delete",
        fmt::format(
            "Are you sure you want to <cr>permanently delete</c> <cy>{}</c>?\n"
            "<cr>This can NOT be undone!</c>",
            level->m_levelName
        ),
        "Cancel", "Delete",
        [level = Ref(level)](auto*, bool btn2) {
            if (btn2) {
                auto res = save::permanentlyDelete(level);
                if (!res) {
                    FLAlertLayer::create(
                        "Failed to Delete",
                        fmt::format("Failed to permanently delete <cy>{}</c>: {}", level->m_levelName, res.unwrapErr()),
                        "OK"
                    )->show();
                }
            }
        }
    );
}
void TrashcanPopup::onRestore(CCObject* sender) {
    auto level = static_cast<GJGameLevel*>(static_cast<CCNode*>(sender)->getUserObject());
    auto res = save::untrashLevel(level);
    if (!res) {
        FLAlertLayer::create(
            "Failed to Restore",
            fmt::format("Failed to restore <cy>{}</c>: {}", level->m_levelName, res.unwrapErr()),
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
                auto res = save::clearTrash();
                if (!res) {
                    FLAlertLayer::create(
                        "Failed to Clear",
                        fmt::format("Failed to clear trash: {}", res.unwrapErr()),
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
