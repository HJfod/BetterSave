#include "Mod.hpp"
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/GManager.hpp>
#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <hjfod.gmd-api/include/GMD.hpp>

using namespace geode::prelude;

static std::filesystem::path getTempDir() {
    return dirs::getSaveDir() / "bettersave.temp";
}

static std::vector<std::string> recoverCrashedLevels() {
	std::vector<std::string> recovered = {};
	for (auto file : file::readDirectory(getTempDir()).unwrapOrDefault()) {
		auto levelRes = gmd::importGmdAsLevel(file);
		if (!levelRes) {
			log::error("Unable to recover level '{}': {}", file.filename(), levelRes.unwrapErr());
			continue;
		}
		auto imported = *levelRes;
		bool existing = false;
		// Check if this is an existing level
		for (auto level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
			if (level->m_levelName == imported->m_levelName && level->m_levelRev == imported->m_levelRev) {
				level->m_levelString = imported->m_levelString;
				level->m_levelDesc = imported->m_levelDesc;
				existing = true;
				break;
			}
		}
		if (!existing) {
			LocalLevelManager::get()->m_localLevels->insertObject(imported, 0);
		}
		recovered.push_back(imported->m_levelName);
	}

	// Save LLM
	if (recovered.size()) {
		LocalLevelManager::get()->save();
		std::error_code ec;
		std::filesystem::remove_all(getTempDir(), ec);
	}

	return recovered;
}

static bool SKIP_SAVING_LLM = false;
class $modify(GManager) {
	$override
	void save() {
		if (static_cast<LocalLevelManager*>(static_cast<GManager*>(this)) == LocalLevelManager::get()) {
			if (SKIP_SAVING_LLM) return;
			GManager::save();

			// If saving local levels succeeded, delete temp dir
			std::error_code ec;
			std::filesystem::remove_all(getTempDir(), ec);
		}
		else {
			GManager::save();
		}
	}
};
struct $modify(EditorPauseLayer) {
	$override
	void saveLevel() {
		SKIP_SAVING_LLM = true;
		EditorPauseLayer::saveLevel();
		SKIP_SAVING_LLM = false;

    	(void)file::createDirectoryAll(getTempDir());
		auto id = getFreeIDInDir(m_editorLayer->m_level->m_levelName, getTempDir(), "gmd");
		auto save = gmd::exportLevelAsGmd(m_editorLayer->m_level, getTempDir() / id);
		if (!save) {
			log::error("Unable to save level '{}': {}", m_editorLayer->m_level->m_levelName, save.unwrapErr());
		}
	}
};
struct $modify(MenuLayer) {
    $override
    bool init() {
        if (!MenuLayer::init())
            return false;

		static bool ENTERED_MENULAYER_ONCE = false;
		if (!ENTERED_MENULAYER_ONCE) {
			ENTERED_MENULAYER_ONCE = true;
			auto count = recoverCrashedLevels();
			if (count.size()) {
				auto alert = FLAlertLayer::create(
					nullptr,
					"Levels Recovered",
					fmt::format(
						"<cy>BetterSave</c> has <cp>recovered data</c> for the following levels after a <cy>crash</c>:\n{}",
						count
					),
					"OK", nullptr, 360
				);
				alert->m_scene = this;
				alert->show();
			}
		}
        
        return true;
    }
};
