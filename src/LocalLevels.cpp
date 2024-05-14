// #include <LocalLevels.hpp>
// #include <Geode/utils/cocos.hpp>
// #include <Geode/modify/LevelEditorLayer.hpp>
// #include <Geode/modify/LocalLevelManager.hpp>
// #include <Geode/modify/EditorPauseLayer.hpp>
// #include <Geode/modify/GameLevelManager.hpp>
// #include <Geode/modify/GJGameLevel.hpp>
// #include <Geode/modify/GJAccountManager.hpp>
// #include <Geode/loader/Dirs.hpp>
// #include <hjfod.gmd-api/include/GMD.hpp>
// #include <matjson/stl_serialize.hpp>
// #include "TrashcanPopup.hpp"

// using namespace geode::prelude;
// using namespace save;

// class TrashLevelEvent::Impl final {
// public:
// 	Ref<GJGameLevel> level;
// 	enum { Delete, Trash, Untrash } mode;
// };

// TrashLevelEvent::TrashLevelEvent() : m_impl(std::make_unique<TrashLevelEvent::Impl>()) {}
// TrashLevelEvent::~TrashLevelEvent() {}

// GJGameLevel* TrashLevelEvent::getLevel() const {
// 	return m_impl->level;
// }
// bool TrashLevelEvent::isPermanentDelete() const {
// 	return m_impl->mode == Impl::Delete;
// }
// bool TrashLevelEvent::isTrash() const {
// 	return m_impl->mode == Impl::Trash;
// }
// bool TrashLevelEvent::isUntrash() const {
// 	return m_impl->mode == Impl::Untrash;
// }

// static std::string currentTimeAsString() {
//     auto t = std::time(nullptr);
//     auto tm = *std::localtime(&t);
//     std::ostringstream oss;
//     oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
// 	return oss.str();
// }
// static ByteVector timeToBytes(TrashTime time) {
// 	return toByteArray(std::chrono::duration_cast<TrashUnit>(time.time_since_epoch()).count());
// }

// struct LevelsMetadata final {
// 	std::vector<std::string> levelOrder;
// };

// template <>
// struct matjson::Serialize<LevelsMetadata> {
//     static matjson::Value to_json(LevelsMetadata const& meta) {
// 		return matjson::Object({
// 			{ "level-order", meta.levelOrder }
// 		});
// 	}
//     static LevelsMetadata from_json(matjson::Value const& value) {
// 		auto meta = LevelsMetadata();
// 		auto obj = value.as_object();
// 		meta.levelOrder = matjson::Serialize<std::vector<std::string>>::from_json(obj["level-order"]);
// 		return meta;
// 	}
// 	static bool is_json(matjson::Value const& value) {
// 		return value.is_object();
// 	}
// };

// static std::vector<TrashedLevel> TRASHCAN {};

// static std::string convertToKebabCase(std::string const& str) {
// 	std::string res {};
// 	char last = '\0';
// 	for (auto c : str) {
// 		// add a dash if the character is in uppercase (camelCase / PascalCase) 
// 		// or a space (Normal case) or an underscore (snake_case) and the 
// 		// built result string isn't empty and make sure there's only a 
// 		// singular dash
// 		// don't add a dash if the previous character was also uppercase 
// 		// (SCREAMING CASE should be just screaming-case)
// 		if ((std::isupper(c) && !std::isupper(last)) || std::isspace(c) || c == '_') {
// 			if (res.size() && res.back() != '-') {
// 				res.push_back('-');
// 			}
// 		}
// 		// only preserve alphanumeric characters
// 		if (std::isalnum(c)) {
// 			res.push_back(std::tolower(c));
// 		}
// 		last = c;
// 	}
// 	// if there is a dash at the end (for example because the name ended in a 
// 	// space) then get rid of that
// 	if (res.back() == '-') {
// 		res.pop_back();
// 	}
// 	return res;
// }

// static std::vector<GJGameLevel*> iterLoadedLevels() {
// 	auto levels = ccArrayToVector<GJGameLevel*>(LocalLevelManager::get()->m_localLevels);
// 	for (auto& trashed : TRASHCAN) {
// 		levels.push_back(trashed.getLevel());
// 	}
// 	return levels;
// }

// static void saveMetadata() {
// 	LevelsMetadata meta;
// 	for (auto level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
// 		meta.levelOrder.push_back(level->getID());
// 	}
// 	(void)file::writeToJson(save::getLevelsSaveDir() / "metadata.json", meta);
// }

// static void assignIDToLevel(GJGameLevel* level) {
// 	// synthesize an ID for the level by taking the level name in kebab-case 
// 	// and then adding an incrementing number at the end until there exists 
// 	// no folder with the same name already
// 	auto name = convertToKebabCase(level->m_levelName);
// 	auto id = name;
// 	size_t counter = 0;
// 	while (
// 		// check both save dir and trashcan for free ID
// 		ghc::filesystem::exists(save::getCreatedLevelsDir() / id) ||
// 		ghc::filesystem::exists(save::getTrashcanDir() / id)
// 	) {
// 		id = fmt::format("{}-{}", name, counter);
// 		counter += 1;
// 	}

// 	// create the level's save directory in order to reserve the ID
// 	(void)file::createDirectoryAll(save::getLevelsSaveDir() / id);

// 	level->setID(id);

// 	log::debug("Level {} was assigned ID '{}'", level->m_levelName, level->getID());
// }

// static void saveLevel(GJGameLevel* level) {
// 	if (level->getID().empty()) {
// 		assignIDToLevel(level);
// 	}

// 	// save the level's data as a .gmd file
// 	auto res = gmd::exportLevelAsGmd(level, save::getLevelSaveDir(level) / "level.gmd");
// 	if (!res) {
// 		log::error("Unable to save level '{}': {}", level->getID(), res.error());
// 	}
// 	else {
// 		log::debug("Level '{}' was saved to level.gmd", level->getID());
// 	}
// }

// static void migrateLevels() {
// 	log::debug("Migrating old levels...");

// 	// make sure to create the save directory
// 	(void)file::createDirectoryAll(save::getLevelsSaveDir());

// 	// Make a backup of CCLocalLevels just in case
// 	(void)file::createDirectoryAll(save::getLevelsSaveDir() / ".ccbackup");

// 	// Check if CCLocalLevels has any data; if it's empty (or the size of an empty plist file), 
// 	// then we don't need to back it up
// 	if (ghc::filesystem::file_size(dirs::getSaveDir() / "CCLocalLevels.dat") > 96) {
// 		std::error_code ec;
// 		ghc::filesystem::copy_file(
// 			dirs::getSaveDir() / "CCLocalLevels.dat",
// 			save::getLevelsSaveDir() / ".ccbackup" / fmt::format("{}.dat", currentTimeAsString()),
// 			ec
// 		);
// 		if (ec) {
// 			log::error("Unable to backup CCLocalLevels! Causing a deliberate null-read to prevent data loss!");

// 			// todo: use geode::utils::terminate once it's available on main
// 			volatile int* addr = nullptr;
// 			volatile int p = *std::launder(addr);
// 		}
// 		else {
// 			log::debug("Backed up CCLocalLevels");
// 		}
// 	}

// 	size_t count = 0;

// 	log::debug("There are {} levels in LocalLevelManager", LocalLevelManager::get()->m_localLevels->count());

// 	// Migrate all existing local levels
// 	for (auto& level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
// 		if (level->getID().empty()) {
// 			saveLevel(level);
// 			count += 1;
// 		}
// 	}

// 	// NOTE: there was a bug here if the game crashes / is deliberately force-closed 
// 	// that causes levels to be duplicated (since CCLocalLevels is never cleared)
// 	// This prevents that by DELETING CCLOCALLEVELS (!!!)
// 	(void)file::writeBinary(dirs::getSaveDir() / "CCLocalLevels.dat", {});
// 	(void)file::writeBinary(dirs::getSaveDir() / "CCLocalLevels2.dat", {});

// 	// Save metadata here already in case the game crashes or some shit
// 	if (count > 0) {
// 		saveMetadata();
// 	}

// 	log::debug("Migrated {} levels", count);
// }

// static void loadLevels(bool trash) {
// 	// Try to load metadata, but don't hard-fail if not possible
// 	auto metaRes = file::readFromJson<LevelsMetadata>(save::getLevelsSaveDir() / "metadata.json");
// 	if (!metaRes) {
// 		log::error("Unable to read metadata: {}", metaRes.unwrapErr());
// 	}
// 	auto meta = metaRes.unwrapOr(LevelsMetadata());

// 	auto dir = trash ? save::getTrashcanDir() : save::getLevelsSaveDir();

// 	// read levels
// 	for (auto levelDir : file::readDirectory(dir).unwrapOr(std::vector<ghc::filesystem::path> {})) {
// 		// skip non-dirs; all levels should be in their own category
// 		if (!ghc::filesystem::is_directory(levelDir)) {
// 			continue;
// 		}
// 		// skip .trash and any folders like that
// 		if (levelDir.has_extension()) {
// 			continue;
// 		}
// 		auto id = levelDir.filename().string();
// 		// check that this level isn't already loaded
// 		for (auto& level : iterLoadedLevels()) {
// 			if (level->getID() == id) {
// 				continue;
// 			}
// 		}
// 		// load level from GMD file
// 		auto levelRes = gmd::importGmdAsLevel(levelDir / "level.gmd");
// 		if (!levelRes) {
// 			log::error("Unable to load {}: {}", dir / "level.gmd", levelRes.unwrapErr());
// 			continue;
// 		}
// 		auto level = levelRes.unwrap();
// 		level->setID(id);

// 		if (trash) {
// 			auto timeBin = file::readBinary(levelDir / ".trashtime").unwrapOr(timeToBytes(TrashClock::now()));
// 			TrashUnit dur(*reinterpret_cast<TrashUnit::rep*>(timeBin.data()));
// 			TRASHCAN.push_back(TrashedLevel::create(level, TrashTime(dur)));
// 		}
// 		else {
// 			LocalLevelManager::get()->m_localLevels->addObject(level);
// 		}

// 		log::debug("Loaded '{}'", id);
// 	}

// 	// Sort based on saved order
// 	auto levels = LocalLevelManager::get()->m_localLevels->data;
// 	std::sort(
// 		levels->arr, levels->arr + levels->num,
// 		[&meta](CCObject* first, CCObject* second) -> bool {
// 			return ranges::indexOf(meta.levelOrder, static_cast<GJGameLevel*>(first)->getID()) < 
// 				ranges::indexOf(meta.levelOrder, static_cast<GJGameLevel*>(second)->getID());
// 		}
// 	);
// }

// ghc::filesystem::path save::getLevelsSaveDir() {
// 	return dirs::getSaveDir() / "levels";
// }
// ghc::filesystem::path save::getCreatedLevelsDir() {
// 	return dirs::getSaveDir() / "levels" / "created";
// }
// ghc::filesystem::path save::getLevelDir(GJGameLevel* level) {
// 	if (isLevelTrashed(level)) {
// 		return save::getTrashcanDir() / level->getID();
// 	}
// 	return save::getCreatedLevelsDir() / level->getID();
// }
// ghc::filesystem::path save::getTrashcanDir() {
// 	return dirs::getSaveDir() / "levels" / "trash";
// }

// Result<> save::moveLevelToTrash(GJGameLevel* level) {
// 	if (isLevelTrashed(level)) return Ok();

// 	auto target = save::getTrashcanDir() / level->getID();

// 	// Make sure trashcan directory exists
// 	GEODE_UNWRAP(file::createDirectoryAll(save::getTrashcanDir()));

// 	// Move the level there
// 	std::error_code ec;
// 	ghc::filesystem::rename(save::getLevelDir(level), target, ec);
// 	if (ec) {
// 		return Err("Unable to move the level folder");
// 	}

// 	// Save the trashing time
// 	auto time = TrashClock::now();
// 	GEODE_UNWRAP(file::writeBinary(target / ".trashtime", timeToBytes(time))
// 		.expect("Unable to save removal time: {error}"));

// 	// Add the level to the trashcan
// 	TRASHCAN.push_back(TrashedLevel::create(level, time));
// 	LocalLevelManager::get()->m_localLevels->removeObject(level);

// 	auto ev = TrashLevelEvent();
// 	ev.m_impl->level = level;
// 	ev.m_impl->mode = TrashLevelEvent::Impl::Trash;
// 	ev.post();

// 	return Ok();
// }
// Result<> save::untrashLevel(GJGameLevel* level) {
// 	if (!isLevelTrashed(level)) return Ok();

// 	auto target = save::getLevelDir(level);

// 	// Move the level out of the trash
// 	std::error_code ec;
// 	ghc::filesystem::rename(save::getTrashcanDir() / level->getID(), target, ec);
// 	if (ec) {
// 		return Err("Unable to move level folder");
// 	}

// 	// Delete the trashtime file; we don't really care if this fails
// 	ghc::filesystem::remove(target / ".trashtime", ec);

// 	// Update in-memory containers
// 	ranges::remove(TRASHCAN, [&](TrashedLevel const& trashed) { return trashed.getLevel() == level; });
// 	LocalLevelManager::get()->m_localLevels->insertObject(level, 0);

// 	auto ev = TrashLevelEvent();
// 	ev.m_impl->level = level;
// 	ev.m_impl->mode = TrashLevelEvent::Impl::Untrash;
// 	ev.post();

// 	return Ok();
// }
// Result<> save::permanentlyDelete(GJGameLevel* level) {
// 	// Delete the level's save directory
// 	std::error_code ec;
// 	ghc::filesystem::remove_all(getLevelDir(level), ec);
// 	if (ec) {
// 		return Err("Unable to delete level directory: {} (error code {})", ec.message(), ec.value());
// 	}

// 	// Delete from in-memory container
// 	if (isLevelTrashed(level)) {
// 		ranges::remove(TRASHCAN, [&](TrashedLevel const& trashed) { return trashed.getLevel() == level; });
// 	}
// 	else {
// 		LocalLevelManager::get()->m_localLevels->removeObject(level);
// 	}

// 	auto ev = TrashLevelEvent();
// 	ev.m_impl->level = level;
// 	ev.m_impl->mode = TrashLevelEvent::Impl::Delete;
// 	ev.post();

// 	return Ok();
// }
// Result<> save::clearTrash() {
// 	if (!ghc::filesystem::exists(getTrashcanDir())) {
// 		return Ok();
// 	}
	
// 	std::error_code ec;
// 	ghc::filesystem::remove_all(getTrashcanDir(), ec);
// 	if (ec) {
// 		return Err("Unable to delete the trashcan directory: {} (error code {})", ec.message(), ec.value());
// 	}
// 	TRASHCAN.clear();

// 	auto ev = TrashLevelEvent();
// 	ev.m_impl->level = nullptr;
// 	ev.m_impl->mode = TrashLevelEvent::Impl::Delete;
// 	ev.post();

// 	return Ok();
// }
// std::vector<TrashedLevel> save::getLevelsInTrash() {
// 	return TRASHCAN;
// }
// void save::openTrashcanPopup() {
// 	if (TRASHCAN.empty()) {
// 		FLAlertLayer::create(
// 			"Trash is Empty",
// 			"You have not <co>trashed</c> any levels!",
// 			"OK"
// 		)->show();
// 	}
// 	else {
// 		TrashcanPopup::create()->show();
// 	}
// }
// bool save::isLevelTrashed(GJGameLevel* level) {
// 	for (auto& t : TRASHCAN) {
// 		if (t.getLevel() == level) {
// 			return true;
// 		}
// 	}
// 	return false;
// }

// static bool IS_BACKING_UP = false;
// struct $modify(LocalLevelManager) {
// 	void encodeDataTo(DS_Dictionary* dict) {
// 		// If the user is backing up, then run the function normally
// 		if (IS_BACKING_UP) {
// 			// Backup only does this in-memory so we don't need to do filesystem 
// 			// cleanup afterwards
// 			return LocalLevelManager::encodeDataTo(dict);
// 		}
		
// 		// If the mod is being disabled or uninstalled, save normally to CCLocalLevels
// 		auto action = Mod::get()->getRequestedAction();
// 		if (
// 			action == ModRequestedAction::Disable || 
// 			action == ModRequestedAction::Uninstall || 
// 			action == ModRequestedAction::UninstallWithSaveData
// 		) {
// 			LocalLevelManager::encodeDataTo(dict);
// 		}
// 		// Otherwise levels are saved individually in EditorPauseLayer::saveLevel
// 		else {
// 			// Save the level order here though, since filesystem is alphabetically sorted
// 			saveMetadata();
// 		}
// 	}
// 	void dataLoaded(DS_Dictionary* dict) {
// 		LocalLevelManager::dataLoaded(dict);

// 		// Migrate old levels if there are any that haven't been migrated yet
// 		migrateLevels();
// 	}
// };
// struct $modify(GJAccountManager) {
// 	bool backupAccount(gd::string idk) {
// 		// For backup we need to temporarily allow saving CCLocalLevels normally
// 		IS_BACKING_UP = true;
// 		auto ret = GJAccountManager::backupAccount(idk);
// 		IS_BACKING_UP = false;
// 		return ret;
// 	}
// };
// struct $modify(EditorPauseLayer) {
// 	void saveLevel() {
// 		EditorPauseLayer::saveLevel();
// 		// this also assigns ids to any newly created levels
// 		::saveLevel(m_editorLayer->m_level);
// 	}
// };
// struct $modify(GameLevelManager) {
// 	void deleteLevel(GJGameLevel* level) {
// 		if (level->m_levelType == GJLevelType::Editor) {
// 			auto res = save::moveLevelToTrash(level);
// 			if (!res) {
// 				log::error("Unable to move level to trash: {} - refusing to delete", res.unwrapErr());
// 				FLAlertLayer::create(
// 					"Error Trashing Level",
// 					fmt::format("Unable to move level to trash: {}", res.unwrapErr()),
// 					"OK"
// 				)->show();
// 			}
// 			return;
// 		}
// 		GameLevelManager::deleteLevel(level);
// 	}
// };

// $on_mod(Loaded) {
// 	log::debug("Loading levels");

// 	// If some levels have already been loaded through LocalLevelManager, run migrations
// 	if (LocalLevelManager::get()->m_localLevels->count()) {
// 		migrateLevels();
// 	}

// 	// load normal levels first, trashcan afterwards (in case of ID conflicts)
// 	loadLevels(false);
// 	loadLevels(true);

// 	log::debug("Loading done");
// }
