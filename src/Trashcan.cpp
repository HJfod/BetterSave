#include "Mod.hpp"
#include <Geode/DefaultInclude.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/loader/Dirs.hpp>
#include <hjfod.gmd-api/include/GMD.hpp>
#include "TrashcanPopup.hpp"

using namespace geode::prelude;

std::filesystem::path getTrashDir() {
    return dirs::getSaveDir() / "bettersave.trash";
}

Trashed::Trashed(std::filesystem::path const& path, GJGameLevel* level) : m_path(path), m_value(level) {}
Trashed::Trashed(std::filesystem::path const& path, GJLevelList* list)  : m_path(path), m_value(list) {}

GJGameLevel* Trashed::asLevel() const {
    if (auto level = std::get_if<Ref<GJGameLevel>>(&m_value)) {
        return *level;
    }
    return nullptr;
}
GJLevelList* Trashed::asList() const {
    if (auto level = std::get_if<Ref<GJLevelList>>(&m_value)) {
        return *level;
    }
    return nullptr;
}

std::string Trashed::getName() const {
    if (asLevel()) {
        return asLevel()->m_levelName;
    }
    else {
        return asList()->m_listName;
    }
}

Trashed::TimePoint Trashed::getTrashTime() const {
    std::error_code ec;
    return std::filesystem::last_write_time(m_path, ec);
}

std::vector<Ref<Trashed>> Trashed::load() {
    std::vector<Ref<Trashed>> trashed;
    for (auto file : file::readDirectory(getTrashDir()).unwrapOrDefault()) {
        if (auto level = gmd::importGmdAsLevel(file)) {
            trashed.push_back(new Trashed(file, *level));
        }
        else if (auto list = gmd::importGmdAsList(file)) {
            trashed.push_back(new Trashed(file, *list));
        }
    }
    return trashed;
}

Result<> Trashed::trash(GJGameLevel* level) {
    (void)file::createDirectoryAll(getTrashDir());
    auto id = getFreeIDInDir(level->m_levelName, getTrashDir(), "gmd");
    auto save = gmd::exportLevelAsGmd(level, getTrashDir() / id);
    if (!save) {
        return Err(save.unwrapErr());
    }
    LocalLevelManager::get()->m_localLevels->removeObject(level);
    UpdateTrashEvent().post();
    return Ok();
}
Result<> Trashed::trash(GJLevelList* list) {
    (void)file::createDirectoryAll(getTrashDir());
    auto id = getFreeIDInDir(list->m_listName, getTrashDir(), "gmdl");
    auto save = gmd::exportListAsGmd(list, getTrashDir() / id);
    if (!save) {
        return Err(save.unwrapErr());
    }
    LocalLevelManager::get()->m_localLists->removeObject(list);
    UpdateTrashEvent().post();
    return Ok();
}
Result<> Trashed::untrash() {
    if (auto level = asLevel()) {
        LocalLevelManager::get()->m_localLevels->insertObject(asLevel(), 0);
    }
    else if (auto list = asList()) {
        LocalLevelManager::get()->m_localLists->insertObject(list, 0);
    }
    std::error_code ec;
    std::filesystem::remove(m_path, ec);
    if (ec) {
        return Err("Unable to delete trashed file: {} (code {})", ec.message(), ec.value());
    }
    UpdateTrashEvent().post();
    return Ok();
}
Result<> Trashed::KABOOM() {
    std::error_code ec;
    std::filesystem::remove(m_path, ec);
    if (ec) {
        return Err("Unable to delete trashed file: {} (code {})", ec.message(), ec.value());
    }
    UpdateTrashEvent().post();
    return Ok();
}

struct $modify(GameLevelManager) {
	$override
	void deleteLevel(GJGameLevel* level) {
		if (level->m_levelType == GJLevelType::Editor) {
			auto res = Trashed::trash(level);
			if (!res) {
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
	$override
	void deleteLevelList(GJLevelList* list) {
		if (list->m_listType == 2) {
			auto res = Trashed::trash(list);
			if (!res) {
				FLAlertLayer::create(
					"Error Trashing List",
					fmt::format("Unable to move list to trash: {}", res.unwrapErr()),
					"OK"
				)->show();
			}
			return;
		}
		GameLevelManager::deleteLevelList(list);
	}
};

class $modify(TrashBrowserLayer, LevelBrowserLayer) {
    struct Fields {
        EventListener<EventFilter<UpdateTrashEvent>> listener;
    };

	$override
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
                    std::error_code ec;
                    auto finnsTrashed = !std::filesystem::is_empty(getTrashDir(), ec) && !ec;
                    trashSpr->setOpacity(finnsTrashed ? 255 : 205);
                    trashSpr->setColor(finnsTrashed ? ccWHITE : ccc3(90, 90, 90));
                };
                m_fields->listener.bind([=, this](auto*) {
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
        std::error_code ec;
        auto finnsTrashed = !std::filesystem::is_empty(getTrashDir(), ec) && !ec;
        if (finnsTrashed) {
            TrashcanPopup::create()->show();
        }
        else {
            FLAlertLayer::create(
                "Trash is Empty",
                "You have not <co>trashed</c> any levels!",
                "OK"
		    )->show();
        }
    }
};

class $modify(EditLevelLayer) {
	$override
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
	$override
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
