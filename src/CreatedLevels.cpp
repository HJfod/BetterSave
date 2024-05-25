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

template <>
struct matjson::Serialize<ListsMetadata> {
    static matjson::Value to_json(ListsMetadata const& meta) {
		return matjson::Object({
			{ "list-order", meta.listOrder }
		});
	}
    static ListsMetadata from_json(matjson::Value const& value) {
		auto meta = ListsMetadata();
		auto obj = value.as_object();
		meta.listOrder = matjson::Serialize<std::vector<std::string>>::from_json(obj["list-order"]);
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

ghc::filesystem::path CreatedLevels::getLevelsPath() const {
    return save::getLevelsSaveDir() / "created";
}
ghc::filesystem::path CreatedLevels::getListsPath() const {
    return save::getLevelsSaveDir() / "lists";
}
void CreatedLevels::onLoad(ghc::filesystem::path const& dir, GmdExportable exportable) {
	// The list is sorted later in `onLoadFinished`
	exportable.visit(makeVisitor {
		[](Ref<GJGameLevel> level) {
    		LocalLevelManager::get()->m_localLevels->addObject(level);
		},
		[](Ref<GJLevelList> list) {
    		LocalLevelManager::get()->m_localLists->addObject(list);
		},
	});
}
void CreatedLevels::onLoadFinished() {
	// Load correct level order
	auto levelsMeta = this->loadLevelsMetadata();

	// Sort based on saved order
	auto levels = LocalLevelManager::get()->m_localLevels->data;
	std::sort(
		levels->arr, levels->arr + levels->num,
		[&levelsMeta](CCObject* first, CCObject* second) -> bool {
			return ranges::indexOf(levelsMeta.levelOrder, static_cast<GJGameLevel*>(first)->getID()) < 
				ranges::indexOf(levelsMeta.levelOrder, static_cast<GJGameLevel*>(second)->getID());
		}
	);

	// Load correct level order
	auto listsMeta = this->loadListsMetadata();

	// Sort based on saved order
	auto lists = LocalLevelManager::get()->m_localLists->data;
	std::sort(
		lists->arr, lists->arr + lists->num,
		[&listsMeta](CCObject* first, CCObject* second) -> bool {
			return ranges::indexOf(listsMeta.listOrder, static_cast<GJLevelList*>(first)->getID()) < 
				ranges::indexOf(listsMeta.listOrder, static_cast<GJLevelList*>(second)->getID());
		}
	);
}
void CreatedLevels::onAdd(GmdExportable exportable, bool isNew) {
    // Add the new level/list at the start
	exportable.visit(makeVisitor {
		[](Ref<GJGameLevel> level) {
   			LocalLevelManager::get()->m_localLevels->insertObject(level, 0);
		},
		[](Ref<GJLevelList> list) {
    		LocalLevelManager::get()->m_localLists->insertObject(list, 0);
		},
	});

    if (!isNew) {
        auto ev = TrashLevelEvent();
        ev.m_impl->level = exportable;
        ev.m_impl->mode = TrashLevelEvent::Impl::Untrash;
        ev.post();
    }
}
void CreatedLevels::onRemove(GmdExportable exportable) {
	exportable.visit(makeVisitor {
		[](Ref<GJGameLevel> level) {
    		LocalLevelManager::get()->m_localLevels->removeObject(level);
		},
		[](Ref<GJLevelList> list) {
    		LocalLevelManager::get()->m_localLists->removeObject(list);
		},
	});
}
std::vector<Ref<GJGameLevel>> CreatedLevels::getAllLevels() const {
	std::vector<Ref<GJGameLevel>> res;
	for (auto level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
		res.push_back(level);
	}
	return res;
}
std::vector<Ref<GJLevelList>> CreatedLevels::getAllLists() const {
	std::vector<Ref<GJLevelList>> res;
	for (auto list : CCArrayExt<GJLevelList*>(LocalLevelManager::get()->m_localLists)) {
		res.push_back(list);
	}
	return res;
}
std::vector<GmdExportable> CreatedLevels::getAll() const {
	std::vector<GmdExportable> res;
	for (auto level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
		// Some mods may introduce non-editor levels to localLevels - ignore them!
		if (auto exportable = GmdExportable::from(level)) {
			res.push_back(*exportable);
		}
	}
	for (auto list : CCArrayExt<GJLevelList*>(LocalLevelManager::get()->m_localLists)) {
		// Some mods may introduce non-editor lists to localLevels - ignore them!
		if (auto exportable = GmdExportable::from(list)) {
			res.push_back(*exportable);
		}
	}
    return res;
}

void CreatedLevels::migrate() {
	log::debug("Migrating old levels");

	// Make a backup of CCLocalLevels just in case
	(void)file::createDirectoryAll(save::getLevelsSaveDir() / ".ccbackup");

	// Check if CCLocalLevels has any data; if it's empty (or the size of an empty plist file), 
	// then we don't need to back it up
	std::error_code ec;
	if (ghc::filesystem::file_size(dirs::getSaveDir() / "CCLocalLevels.dat", ec) > 96) {
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

	size_t migratedLevelsCount = 0;
	size_t migratedListsCount = 0;

	log::debug(
		"There are {} levels and {} lists in LocalLevelManager",
		LocalLevelManager::get()->m_localLevels->count(),
		LocalLevelManager::get()->m_localLists->count()
	);

	// Migrate all existing local levels
	// Create a copy of the list because we will possibly be mutating the original array
	for (auto level : this->getAll()) {
		// Check that this level isn't already categorized
        if (!CategoryInfo::from(level)) {
			// Check if this is likely a duplicate of an already-loaded level
			bool duplicate = false;
			for (auto other : this->getAll()) {
				// If this level has the same name and object count as an 
				// already categorized level, it's most likely a duplicate
				if (CategoryInfo::from(other) && other == level) {
					duplicate = true;
				}
			}
			if (duplicate) {
				level.visit(makeVisitor {
					[](Ref<GJGameLevel> level) {
						LocalLevelManager::get()->m_localLevels->removeObject(level);
					},
					[](Ref<GJLevelList> list) {
						LocalLevelManager::get()->m_localLists->removeObject(list);
					},
				});
				continue;
			}

			// Categorize this level
            CategoryInfo::from(level, this);
			migratedLevelsCount += 1;
        }
	}

	// NOTE: there was a bug here if the game crashes / is deliberately force-closed 
	// that causes levels to be duplicated (since CCLocalLevels is never cleared)
	// This prevents that by DELETING CCLOCALLEVELS (!!!)
	(void)file::writeBinary(dirs::getSaveDir() / "CCLocalLevels.dat", {});
	(void)file::writeBinary(dirs::getSaveDir() / "CCLocalLevels2.dat", {});

	// Save metadata here already in case the game crashes or some shit
	if (migratedLevelsCount > 0) {
		saveLevelsMetadata();
	}
	if (migratedListsCount > 0) {
		saveListsMetadata();
	}

	log::debug("Migrated {} levels and {} lists", migratedLevelsCount, migratedListsCount);
}
void CreatedLevels::recoverLists() {
	log::debug("Recovering lists");

	size_t recovered = 0;
	for (auto dat : file::readDirectory(save::getLevelsSaveDir() / ".ccbackup").unwrapOrDefault()) {
		if (!ghc::filesystem::is_regular_file(dat) || dat.extension() != ".dat") {
			continue;
		}
		auto dict = std::make_unique<DS_Dictionary>();
		dict->loadRootSubDictFromCompressedFile(dat.string().c_str());

		if (dict->stepIntoSubDictWithKey("LLM_03")) {
			for (auto key : dict->getAllKeys()) {
				if (!std::string(key).starts_with("k_")) {
					continue;
				}
				if (dict->stepIntoSubDictWithKey(key.c_str())) {
					auto list = GJLevelList::create();
					list->dataLoaded(dict.get());
					list->m_isEditable = true;
					list->m_listType = 2;

					// Duplicates will be deleted in migrate()
					LocalLevelManager::get()->m_localLists->addObject(list);

					recovered += 1;

					dict->stepOutOfSubDict();
				}
			}
		} 
	}

	// Run migrations if any were recovered
	if (recovered > 0) {
		this->migrate();
	}

	log::debug("Recovered ~{} lists (may contain duplicates)", recovered);
}

void CreatedLevels::saveLevelsMetadata() const {
	LevelsMetadata meta;
	for (auto level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
		meta.levelOrder.push_back(level->getID());
	}
	(void)file::writeToJson(this->getLevelsPath() / "metadata.json", meta);
}
LevelsMetadata CreatedLevels::loadLevelsMetadata() const {
	// Try to load metadata, but don't hard-fail if not possible
	auto metaRes = file::readFromJson<LevelsMetadata>(this->getLevelsPath() / "metadata.json");
	if (!metaRes) {
		log::error("Unable to read metadata: {}", metaRes.unwrapErr());
	}
	return metaRes.unwrapOr(LevelsMetadata());
}
void CreatedLevels::saveListsMetadata() const {
	ListsMetadata meta;
	for (auto list : CCArrayExt<GJLevelList*>(LocalLevelManager::get()->m_localLists)) {
		meta.listOrder.push_back(list->getID());
	}
	(void)file::writeToJson(this->getListsPath() / "metadata.json", meta);
}
ListsMetadata CreatedLevels::loadListsMetadata() const {
	// Try to load metadata, but don't hard-fail if not possible
	auto metaRes = file::readFromJson<ListsMetadata>(this->getListsPath() / "metadata.json");
	if (!metaRes) {
		log::error("Unable to read metadata: {}", metaRes.unwrapErr());
	}
	return metaRes.unwrapOr(ListsMetadata());
}
