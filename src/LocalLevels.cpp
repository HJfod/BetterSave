
#include <LocalLevels.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/LocalLevelManager.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/loader/Dirs.hpp>
#include <hjfod.gmd-api/include/GMD.hpp>

using namespace geode::prelude;

struct $modify(CategorizedLevel, GJGameLevel) {
	save::LevelCategory* category = nullptr;
};

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

static std::string freeIDForLevel(GJGameLevel* level, LevelCategory* category) {
	
}

static void migrateLevels() {
	log::info("Migrating old levels...");

	// make sure to create the save directory
	(void)file::createDirectoryAll(save::getLevelsSaveDir());

	// migrate all existing local levels
	for (auto& level : CCArrayExt<GJGameLevel>(LocalLevelManager::get()->m_localLevels)) {
		save::moveLevelToCategory(level, save::getLocalLevelsCategory());
	}

	log::info("Migration done");
}

static void loadLocalLevels() {
	log::info("Loading levels");

	// migrate old levels if there are any that haven't been migrated yet
	migrateLevels();

	save::loadLevelsForCategory(save::getLocalLevelsCategory());
	save::loadLevelsForCategory(save::getTrashcanCategory());

	log::info("Loading done");
}

void save::loadLevelsForCategory(LevelCategory* category) {
	log::info("Loading levels for category '{}'", category->getID());

	// read levels
	for (auto dir : file::readDirectory(category->getDir())) {
		// skip non-dirs; all levels should be in their own category
		if (!ghc::filesystem::is_directory(dir)) {
			continue;
		}
		// load level from GMD file
		auto levelRes = gmd::importGmdAsLevel(dir / "level.gmd");
		if (!levelRes) {
			log::info("Unable to load {}: {}", dir / "level.gmd", levelRes.unwrapErr());
			continue;
		}
		auto level = levelRes.unwrap();
		// add level to category
		// preset the category field so moveLevelToCategory doesn't do anything 
		static_cast<CategorizedLevel*>(level)->m_fields->category = category;
		category->addLevel(level);
	}
}

ghc::filesystem::path save::getLevelsSaveDir() {
	return dirs::getSaveDir() / "levels";
}

void save::moveLevelToCategory(GJGameLevel* level, LevelCategory* category) {
	// this is in case the category that previously held this level is 
	// the only strong ref to the GJGameLevel
	auto guard = Ref(level);

	// remove from old category
	if (auto cat = save::getCategoryForLevel(level)) {
		cat->removeLevel(level);
	}
	
	// if a new category was provided, move level's save data there
	if (category) {

	}
	// add to new one
	category->addLevel(level);
	static_cast<CategorizedLevel*>(level)->m_fields->category = category;
}

LevelCategory* save::getCategoryForLevel(GJGameLevel* level) {
	static_cast<CategorizedLevel*>(level)->m_fields->category;
}

void save::LevelCategory::addLevel(GJGameLevel* level) {
	m_levels.push_back(level);

	auto categoryID = this->getID();

	// synthesize an ID for the level by taking the level name in kebab-case 
	// and then adding an incrementing number at the end until there exists 
	// no folder with the same name already in savedata/levels
	auto name = convertToKebabCase(level->m_levelName);
	auto id = name;
	size_t counter = 0;
	while (ghc::filesystem::exists(save::getLevelsSaveDir() / categoryID / id)) {
		id = name + "-" + std::to_string(counter);
	}

	// create the level's save directory in order to reserve the ID
	auto saveDir = save::getLevelsSaveDir() / categoryID / id;
	(void)file::createDirectoryAll(saveDir);

	// actually set the id
	level->setID(id);

	log::info("Level {} was assigned ID '{}'", level->m_levelName, level->getID());

	// if the level has data already, save it as a .gmd file
	if (!level->m_levelString.empty()) {
		auto res = gmd::exportLevelAsGmd(level, saveDir / "level.gmd");
		if (!res) {
			log::error("Unable to save level '{}': {}", id, res.error());
		}
		else {
			log::info("Level '{}' was saved to level.gmd", level->getID());
		}
	}
}

void save::LevelCategory::removeLevel(GJGameLevel* level) {
	// make sure the level has an ID (if you do path / "" it results in just path
	// which is very undesirable)
	if (level->getID().empty()) {
		return;
	}
	// todo: move all file logic to moveLevelToCategory
	// todo: preserve all files in the directory
	auto categoryID = this->getID();
	auto saveDir = save::getLevelsSaveDir() / categoryID / level->getID();
	if (ghc::filesystem::exists(saveDir)) {
		// delete level's save directory
		ghc::filesystem::remove_all(saveDir);
		// reset ID to mark it as not being in the system anymore
		level->setID("");
		// remove the level from m_levels
		ranges::remove(m_levels, [](Ref<GJGameLevel> lvl) {
			return lvl == level;
		});
	}
}

std::vector<Ref<GJGameLevel>> save::LevelCategory::getLevels() const {
	return m_levels;
}

ghc::filesystem::path save::LevelCategory::getDir() const {
	return save::getLevelsSaveDir() / category->getID();
}

class LocalLevelsCategory : public save::LevelCategory {
	void addLevel(GJGameLevel* level) override {
		LevelCategory::addLevel(level);
		LocalLevelManager::get()->m_localLevels->addObject(level);
	}

	void removeLevel(GJGameLevel* level) override {
		LevelCategory::removeLevel(level);
		LocalLevelManager::get()->m_localLevels->removeObject(level);
	}

public:
	std::string getID() const override {
		return "local";
	}

	static LocalLevelsCategory* get() {
		static auto ret = new LocalLevelsCategory();
		return ret;
	}
};

class TrashcanCategory : public save::LevelCategory {
	void addLevel(GJGameLevel* level) override {
		LevelCategory::addLevel(level);
		// save the time this level was added to the trashcan
		file::writeBinary(
			save::getLevelsSaveDir() / level->getID() / ".trashed",
			toByteArray(std::chrono::system_clock::now().time_since_epoch().count())
		)
	}

public:
	std::string getID() const override {
		return "trashcan";
	}

	static TrashcanCategory* get() {
		static auto ret = new TrashcanCategory();
		return ret;
	}
};

LevelCategory* save::getLocalLevelsCategory() {
	return LocalLevelsCategory::get();
}

LevelCategory* save::getTrashcanCategory() {
	return TrashcanCategory::get();
}

struct $modify(LocalLevelManager) {
	void encodeDataTo(DS_Dictionary*) {
		// levels are saved individually
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
	}
};

struct $modify(GameLevelManager) {
	void deleteLevel(GJGameLevel* level) {
		if (level->m_levelType == GJLevelType::Editor) {
			save::moveLevelToCategory(level, TrashcanCategory::get());
		}
		GameLevelManager::deleteLevel(level);
	}
};

$on_mod(Loaded) {
	loadLocalLevels();
}
