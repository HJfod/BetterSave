
#include <LocalLevels.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/LocalLevelManager.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/loader/Dirs.hpp>
#include <hjfod.gmd-api/include/GMD.hpp>

using namespace geode::prelude;

using TrashClock = std::chrono::system_clock;
using TrashTime = std::chrono::time_point<std::chrono::system_clock>;
using TrashUnit = std::chrono::minutes;

static ByteVector currentTimeAsBytes() {
	return toByteArray(
		std::chrono::duration_cast<TrashUnit>(
			TrashClock::now().time_since_epoch()
		).count()
	);
}

struct TrashedLevel {
	Ref<GJGameLevel> level;
	TrashTime time;
};

static std::vector<TrashedLevel> TRASHCAN {};

static std::string convertToKebabCase(std::string const& str) {
	std::string res {};
	char last = '\0';
	for (auto c : str) {
		// add a dash if the character is in uppercase (camelCase / PascalCase) 
		// or a space (Normal case) or an underscore (snake_case) and the 
		// built result string isn't empty and make sure there's only a 
		// singular dash
		// don't add a dash if the previous character was also uppercase 
		// (SCREAMING CASE should be just screaming-case)
		if ((std::isupper(c) && !std::isupper(last)) || std::isspace(c) || c == '_') {
			if (res.size() && res.back() != '-') {
				res.push_back('-');
			}
		}
		// only preserve alphanumeric characters
		if (std::isalnum(c)) {
			res.push_back(std::tolower(c));
		}
		last = c;
	}
	// if there is a dash at the end (for example because the name ended in a 
	// space) then get rid of that
	if (res.back() == '-') {
		res.pop_back();
	}
	return res;
}

static std::vector<GJGameLevel*> iterLoadedLevels() {
	auto levels = std::vector<GJGameLevel*> {};
	auto local = ccArrayToVector<GJGameLevel*>(LocalLevelManager::get()->m_localLevels);
	levels.insert(levels.begin(), local.begin(), local.end());
	for (auto& trashed : TRASHCAN) {
		levels.push_back(trashed.level.data());
	}
	return levels;
}

static void assignIDToLevel(GJGameLevel* level) {
	// synthesize an ID for the level by taking the level name in kebab-case 
	// and then adding an incrementing number at the end until there exists 
	// no folder with the same name already
	auto name = convertToKebabCase(level->m_levelName);
	auto id = name;
	size_t counter = 0;
	while (
		// check both save dir and trashcan for free ID
		ghc::filesystem::exists(save::getLevelsSaveDir() / id) ||
		ghc::filesystem::exists(save::getTrashcanDir() / id)
	) {
		id = name + "-" + std::to_string(counter);
	}

	// create the level's save directory in order to reserve the ID
	(void)file::createDirectoryAll(save::getLevelsSaveDir() / id);

	level->setID(id);

	log::info("Level {} was assigned ID '{}'", level->m_levelName, level->getID());
}

static void saveLevel(GJGameLevel* level) {
	if (level->getID().empty()) {
		assignIDToLevel(level);
	}

	// save the level's data as a .gmd file
	auto res = gmd::exportLevelAsGmd(level, save::getLevelsSaveDir() / level->getID() / "level.gmd");
	if (!res) {
		log::error("Unable to save level '{}': {}", level->getID(), res.error());
	}
	else {
		log::info("Level '{}' was saved to level.gmd", level->getID());
	}
}

static void migrateLevels() {
	log::info("Migrating old levels...");

	// make sure to create the save directory
	(void)file::createDirectoryAll(save::getLevelsSaveDir());

	size_t count = 0;

	// migrate all existing local levels
	for (auto& level : CCArrayExt<GJGameLevel>(LocalLevelManager::get()->m_localLevels)) {
		if (level->getID().empty()) {
			saveLevel(level);
			count += 1;
		}
	}

	log::info("Migrated {} levels", count);
}

static void loadLevels(bool trash) {
	auto dir = trash ? save::getTrashcanDir() : save::getLevelsSaveDir();
	// read levels
	for (auto levelDir : file::readDirectory(dir).unwrapOr(std::vector<ghc::filesystem::path> {})) {
		// skip non-dirs; all levels should be in their own category
		if (!ghc::filesystem::is_directory(levelDir)) {
			continue;
		}
		// skip .trash and any folders like that
		if (levelDir.has_extension()) {
			continue;
		}
		auto id = levelDir.filename().string();
		// check that this level isn't already loaded
		for (auto& level : iterLoadedLevels()) {
			if (level->getID() == id) {
				continue;
			}
		}
		// load level from GMD file
		auto levelRes = gmd::importGmdAsLevel(levelDir / "level.gmd");
		if (!levelRes) {
			log::info("Unable to load {}: {}", dir / "level.gmd", levelRes.unwrapErr());
			continue;
		}
		auto level = levelRes.unwrap();
		level->setID(id);

		if (trash) {
			auto timeBin = file::readBinary(levelDir / "trashtime")
				.unwrapOr(currentTimeAsBytes());
			TrashUnit dur(*reinterpret_cast<TrashUnit::rep*>(timeBin.data()));
			TRASHCAN.push_back(TrashedLevel {
				.level = level,
				.time = TrashTime(dur),
			});
		}
		else {
			LocalLevelManager::get()->m_localLevels->addObject(level);
		}

		log::info("Loaded '{}'", id);
	}
}

static void loadLocalLevels() {
	log::info("Loading levels");

	// migrate old levels if there are any that haven't been migrated yet
	migrateLevels();

	// load normal levels first, trashcan afterwards
	loadLevels(false);
	loadLevels(true);

	log::info("Loading done");
}

ghc::filesystem::path save::getLevelsSaveDir() {
	return dirs::getSaveDir() / "levels";
}

ghc::filesystem::path save::getTrashcanDir() {
	return dirs::getSaveDir() / "levels" / ".trash";
}

Result<> save::moveLevelToTrash(GJGameLevel* level) {
	try {
		auto target = save::getTrashcanDir() / level->getID();
		ghc::filesystem::rename(
			save::getLevelsSaveDir() / level->getID(), target
		);
		GEODE_UNWRAP(file::writeBinary(target / "trashtime", currentTimeAsBytes())
			.expect("Unable to save removal time: {error}"));
		log::info("Moved level '{}' to trash", level->getID());
		return Ok();
	}
	catch(std::exception& e) {
		return Err("Unable to move level to trash: {}", e.what());
	}
}

Result<> save::untrashLevel(GJGameLevel* level) {
	try {
		auto target = save::getLevelsSaveDir() / level->getID();
		ghc::filesystem::rename(
			save::getTrashcanDir() / level->getID(), target
		);
		ghc::filesystem::remove(target / "trashtime");
		log::info("Restored level '{}' from trash", level->getID());
		return Ok();
	}
	catch(std::exception& e) {
		return Err("Unable to return level from trash: {}", e.what());
	}
}

struct $modify(LocalLevelManager) {
	void encodeDataTo(DS_Dictionary*) {
		// levels are saved individually in EditorPauseLayer::saveLevel
	}

	void dataLoaded(DS_Dictionary* dict) {
		LocalLevelManager::dataLoaded(dict);
		// switch to better saving if possible already
		loadLocalLevels();
	}
};

struct $modify(EditorPauseLayer) {
	void saveLevel() {
		EditorPauseLayer::saveLevel();
		// this also assigns ids to any newly created levels
		::saveLevel(m_editorLayer->m_level);
		log::info("Saved '{}'", m_editorLayer->m_level->getID());
	}
};

struct $modify(GameLevelManager) {
	void deleteLevel(GJGameLevel* level) {
		if (level->m_levelType == GJLevelType::Editor) {
			auto res = save::moveLevelToTrash(level);
			if (!res) {
				log::error("Unable to move level to trash: {} - refusing to delete", res.unwrapErr());
				return;
			}
		}
		GameLevelManager::deleteLevel(level);
	}
};

$on_mod(Loaded) {
	loadLocalLevels();
}
