#include "Mod.hpp"
#include <Geode/modify/LocalLevelManager.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <hjfod.gmd-api/include/GMD.hpp>

using namespace geode::prelude;

static std::filesystem::path getTempDir() {
    return dirs::getSaveDir() / "bettersave.temp";
}

static void recoverCrashedLevels() {
	for (auto file : file::readDirectory(getTempDir()).unwrapOrDefault()) {
		auto levelRes = gmd::importGmdAsLevel(file);
		if (!levelRes) {
			log::error("Unable to recover level '{}': {}", file.filename(), levelRes.unwrapErr());
			continue;
		}
		LocalLevelManager::get()->m_localLevels->insertObject(*levelRes, 0);
	}
	std::error_code ec;
	std::filesystem::remove_all(getTempDir(), ec);
}

static bool IS_SAVING_INDIVIDUAL_LEVEL = false;
struct $modify(LocalLevelManager) {
	$override
	void encodeDataTo(DS_Dictionary* dict) {
		// If we're saving individual level, skip
		if (IS_SAVING_INDIVIDUAL_LEVEL) {
			return;
		}
		LocalLevelManager::encodeDataTo(dict);

		// If saving local levels succeeded, delete temp dir
		std::error_code ec;
		std::filesystem::remove_all(getTempDir(), ec);
	}
	$override
	void dataLoaded(DS_Dictionary* dict) {
		LocalLevelManager::dataLoaded(dict);
		recoverCrashedLevels();
	}
};
struct $modify(EditorPauseLayer) {
	$override
	void saveLevel() {
		IS_SAVING_INDIVIDUAL_LEVEL = true;
		EditorPauseLayer::saveLevel();
		IS_SAVING_INDIVIDUAL_LEVEL = false;

    	(void)file::createDirectoryAll(getTempDir());
		auto id = getFreeIDInDir(m_editorLayer->m_level->m_levelName, getTempDir(), "gmd");
		auto save = gmd::exportLevelAsGmd(m_editorLayer->m_level, getTempDir() / id);
		if (!save) {
			log::error("Unable to save level '{}': {}", m_editorLayer->m_level->m_levelName, save.unwrapErr());
		}
	}
};
