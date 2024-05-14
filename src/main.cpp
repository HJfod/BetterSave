#include <BetterSave.hpp>
#include <Geode/modify/GJAccountManager.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/LocalLevelManager.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include "Category.hpp"
#include "CreatedLevels.hpp"
#include "Trashcan.hpp"

using namespace geode::prelude;
using namespace save;

$on_mod(Loaded) {
	log::debug("Loading levels");

	// If some levels have already been loaded through LocalLevelManager, run migrations
	bool shouldMigrate = LocalLevelManager::get()->m_localLevels->count();

	// Load normal levels first, trashcan afterwards (in case of ID conflicts)
	CreatedLevels::get()->loadLevels();
	Trashcan::get()->loadLevels();

	// Run migrations only after to prevent duplications
	if (shouldMigrate) {
		CreatedLevels::get()->migrate();
	}

	log::debug("Loading done");
}

// Data loading

static bool IS_BACKING_UP = false;
struct $modify(LocalLevelManager) {
	void encodeDataTo(DS_Dictionary* dict) {
		// If the user is backing up, then run the function normally
		if (IS_BACKING_UP) {
			// Backup only does this in-memory so we don't need to do filesystem 
			// cleanup afterwards
			return LocalLevelManager::encodeDataTo(dict);
		}
		
		// If the mod is being disabled or uninstalled, save normally to CCLocalLevels
		auto action = Mod::get()->getRequestedAction();
		if (
			action == ModRequestedAction::Disable || 
			action == ModRequestedAction::Uninstall || 
			action == ModRequestedAction::UninstallWithSaveData
		) {
			LocalLevelManager::encodeDataTo(dict);
		}
		// Otherwise levels are saved individually in EditorPauseLayer::saveLevel
		else {
			// Save the level order here though, since filesystem is alphabetically sorted
			CreatedLevels::get()->saveMetadata();
		}
	}
	void dataLoaded(DS_Dictionary* dict) {
		LocalLevelManager::dataLoaded(dict);

		// Migrate old levels if there are any that haven't been migrated yet
		CreatedLevels::get()->migrate();
	}
};
struct $modify(GJAccountManager) {
	bool backupAccount(gd::string idk) {
		// For backup we need to temporarily allow saving CCLocalLevels normally
		IS_BACKING_UP = true;
		auto ret = GJAccountManager::backupAccount(idk);
		IS_BACKING_UP = false;
		return ret;
	}
};
struct $modify(EditorPauseLayer) {
	void saveLevel() {
		EditorPauseLayer::saveLevel();
        auto info = CategoryInfo::from(m_editorLayer->m_level, CreatedLevels::get());
        if (info) {
			auto res = info->saveLevel();
			if (!res) {
            	log::error("Unable to save level: {}", res.unwrapErr());
			}
        }
	}
};
struct $modify(GameLevelManager) {
	void deleteLevel(GJGameLevel* level) {
		if (level->m_levelType == GJLevelType::Editor) {
			auto res = save::trash(level);
			if (!res) {
				log::error("Unable to move level to trash: {} - refusing to delete", res.unwrapErr());
				FLAlertLayer::create(
					"Error Trashing Level",
					fmt::format("Unable to move level to trash: {}", res.unwrapErr()),
					"OK"
				)->show();
			}
			return;
		}
		GameLevelManager::deleteLevel(level);
	}
};

// UI

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
                    auto finnsTrashed = !save::trashcan::getTrashedLevels().empty();
                    trashSpr->setOpacity(finnsTrashed ? 255 : 205);
                    trashSpr->setColor(finnsTrashed ? ccWHITE : ccc3(90, 90, 90));
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
        save::trashcan::openTrashcanPopup();
    }
};

// Fix the popup messages claiming deletion is permanent

class $modify(EditLevelLayer) {
    void confirmDelete(CCObject*) {
        auto alert = FLAlertLayer::create(
            this,
            "Trash level", 
            "Are you sure you want to <cr>trash</c> this level?\n"
            "<cy>You can restore the level or permanently delete it through the Trashcan.</c>",
            "Cancel", "Trash",
            340
        );
        alert->setTag(4);
		alert->m_button2->updateBGImage("GJ_button_06.png");
        alert->show();
    }
};
class $modify(LevelBrowserLayer) {
    void onDeleteSelected(CCObject* sender) {
		if (m_searchObject->m_searchType == SearchType::MyLevels) {
			size_t count = 0;
			for (auto level : CCArrayExt<GJGameLevel*>(m_levels)) {
				if (level->m_selected) {
					count += 1;
				}
			}
			if (count > 0) {
				auto alert = FLAlertLayer::create(
					this,
					"Trash levels", 
					fmt::format(
						"Are you sure you want to <cr>trash</c> the <cp>{0}</c> selected level{1}?\n"
						"<cy>You can restore the level{1} or permanently delete {2} through the Trashcan.</c>",
						count, (count == 1 ? "" : "s"), (count == 1 ? "it" : "them")
					),
					"Cancel", "Trash",
					340
				);
				alert->setTag(5);
				alert->m_button2->updateBGImage("GJ_button_06.png");
				alert->show();
			}
			else {
				FLAlertLayer::create("Nothing here...", "No levels selected.", "OK")->show();
			}
		}
		else {
			return LevelBrowserLayer::onDeleteSelected(sender);
		}
    }
};
