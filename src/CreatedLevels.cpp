#include "CreatedLevels.hpp"
#include "TrashLevelEvent.hpp"
#include <matjson/stl_serialize.hpp>

static std::string currentTimeAsString() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
	return oss.str();
}

template <>
struct matjson::Serialize<LevelsMetadata> {
    static matjson::Value to_json(LevelsMetadata const& meta) {
		return matjson::Object({
			{ "level-order", meta.levelOrder }
		});
	}
    static LevelsMetadata from_json(matjson::Value const& value) {
		auto meta = LevelsMetadata();
		auto obj = value.as_object();
		meta.levelOrder = matjson::Serialize<std::vector<std::string>>::from_json(obj["level-order"]);
		return meta;
	}
	static bool is_json(matjson::Value const& value) {
		return value.is_object();
	}
};

CreatedLevels* CreatedLevels::get() {
    static auto inst = new CreatedLevels();
    return inst;
}

ghc::filesystem::path CreatedLevels::getPath() const {
    return save::getLevelsSaveDir() / "created";
}
void CreatedLevels::onLoad(ghc::filesystem::path const& dir, GJGameLevel* level) {
	// The list is sorted later in `onLoadFinished`
    LocalLevelManager::get()->m_localLevels->addObject(level);
}
void CreatedLevels::onLoadFinished() {
	// Load correct level order
	auto meta = this->loadMetadata();

	// Sort based on saved order
	auto levels = LocalLevelManager::get()->m_localLevels->data;
	std::sort(
		levels->arr, levels->arr + levels->num,
		[&meta](CCObject* first, CCObject* second) -> bool {
			return ranges::indexOf(meta.levelOrder, static_cast<GJGameLevel*>(first)->getID()) < 
				ranges::indexOf(meta.levelOrder, static_cast<GJGameLevel*>(second)->getID());
		}
	);
}
void CreatedLevels::onAdd(GJGameLevel* level, bool isNew) {
    // Add the new level at the start
    LocalLevelManager::get()->m_localLevels->insertObject(level, 0);

    if (!isNew) {
        auto ev = TrashLevelEvent();
        ev.m_impl->level = level;
        ev.m_impl->mode = TrashLevelEvent::Impl::Untrash;
        ev.post();
    }
}
void CreatedLevels::onRemove(GJGameLevel* level) {
    LocalLevelManager::get()->m_localLevels->removeObject(level);
}
std::vector<GJGameLevel*> CreatedLevels::getAllLevels() const {
    return ccArrayToVector<GJGameLevel*>(LocalLevelManager::get()->m_localLevels);
}

void CreatedLevels::migrate() {
	log::debug("Migrating old levels...");

	// Make a backup of CCLocalLevels just in case
	(void)file::createDirectoryAll(save::getLevelsSaveDir() / ".ccbackup");

	// Check if CCLocalLevels has any data; if it's empty (or the size of an empty plist file), 
	// then we don't need to back it up
	if (ghc::filesystem::file_size(dirs::getSaveDir() / "CCLocalLevels.dat") > 96) {
		std::error_code ec;
		ghc::filesystem::copy_file(
			dirs::getSaveDir() / "CCLocalLevels.dat",
			save::getLevelsSaveDir() / ".ccbackup" / fmt::format("{}.dat", currentTimeAsString()),
			ec
		);
		if (ec) {
			log::error("Unable to backup CCLocalLevels! Causing a deliberate null-read to prevent data loss!");

			// todo: use geode::utils::terminate once it's available on main
			volatile int* addr = nullptr;
			volatile int p = *std::launder(addr);
		}
		else {
			log::debug("Backed up CCLocalLevels");
		}
	}

	size_t count = 0;

	log::debug("There are {} levels in LocalLevelManager", LocalLevelManager::get()->m_localLevels->count());

	// Migrate all existing local levels
	// Create a copy of the list because we will possibly be mutating the original array
	auto copy = ccArrayToVector<GJGameLevel*>(LocalLevelManager::get()->m_localLevels);
	for (auto& level : copy) {
		// Check that this level isn't already categorized
        if (!CategoryInfo::from(level)) {
			// Check if this is likely a duplicate of an already-loaded level
			bool duplicate = false;
			for (auto test : this->getAllLevels()) {
				// If this level has the same name and object count as an 
				// already categorized level, it's most likely a duplicate
				if (
					CategoryInfo::from(test) &&
					test->m_levelName == level->m_levelName &&
					test->m_objectCount == level->m_objectCount
				) {
					duplicate = true;
				}
			}
			if (duplicate) {
				LocalLevelManager::get()->m_localLevels->removeObject(level);
				continue;
			}

			// Categorize this level
            CategoryInfo::from(level, this);
			count += 1;
        }
	}

	// NOTE: there was a bug here if the game crashes / is deliberately force-closed 
	// that causes levels to be duplicated (since CCLocalLevels is never cleared)
	// This prevents that by DELETING CCLOCALLEVELS (!!!)
	(void)file::writeBinary(dirs::getSaveDir() / "CCLocalLevels.dat", {});
	(void)file::writeBinary(dirs::getSaveDir() / "CCLocalLevels2.dat", {});

	// Save metadata here already in case the game crashes or some shit
	if (count > 0) {
		saveMetadata();
	}

	log::debug("Migrated {} levels", count);
}

void CreatedLevels::saveMetadata() const {
	LevelsMetadata meta;
	for (auto level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
		meta.levelOrder.push_back(level->getID());
	}
	(void)file::writeToJson(this->getPath() / "metadata.json", meta);
}
LevelsMetadata CreatedLevels::loadMetadata() const {
	// Try to load metadata, but don't hard-fail if not possible
	auto metaRes = file::readFromJson<LevelsMetadata>(this->getPath() / "metadata.json");
	if (!metaRes) {
		log::error("Unable to read metadata: {}", metaRes.unwrapErr());
	}
	return metaRes.unwrapOr(LevelsMetadata());
}
