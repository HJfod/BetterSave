#include "Category.hpp"
#include "Terminate.hpp"
#include "TrashLevelEvent.hpp"
#include <hjfod.gmd-api/include/GMD.hpp>

static std::string convertToKebabCase(std::string const& str) {
	std::string res {};
	char last = '\0';
	for (auto c : str) {
		// Add a dash if the character is in uppercase (camelCase / PascalCase) 
		// or a space (Normal case) or an underscore (snake_case) and the 
		// built result string isn't empty and make sure there's only a 
		// singular dash
		// Don't add a dash if the previous character was also uppercase or a 
		// number (SCREAM1NG L33TCASE should be just scream1ng-l33tcase)
		if ((std::isupper(c) && !(std::isupper(last) || std::isdigit(last))) || std::isspace(c) || c == '_') {
			if (res.size() && res.back() != '-') {
				res.push_back('-');
			}
		}
		// Only preserve alphanumeric characters
		if (std::isalnum(c)) {
			res.push_back(std::tolower(c));
		}
		last = c;
	}
	// If there is a dash at the end (for example because the name ended in a 
	// space) then get rid of that
	if (res.back() == '-') {
		res.pop_back();
	}
	return res;
}
static void checkReservedFilenames(std::string& name) {
    switch (hash(name.c_str())) {
        case hash("con"): case hash("prn"): case hash("aux"): case hash("nul"):
        // This was in https://www.boost.org/doc/libs/1_36_0/libs/filesystem/doc/portability_guide.htm?
        // Never heard of it before though
        case hash("clock$"):
        case hash("com1"): case hash("com2"): case hash("com3"): case hash("com4"):
        case hash("com5"): case hash("com6"): case hash("com7"): case hash("com8"): case hash("com9"):
        case hash("lpt1"): case hash("lpt2"): case hash("lpt3"): case hash("lpt4"):
        case hash("lpt5"): case hash("lpt6"): case hash("lpt7"): case hash("lpt8"): case hash("lpt9"):
        {
            name += "-0";
        }
        break;

        default: {} break;
    }
}

GmdExportable GmdExportable::assertFrom(GJGameLevel* level) {
    ASSERT_OR_TERMINATE_INTO(auto res, "Creating GmdExportable from level", GmdExportable::from(level));
    return res;
}
GmdExportable GmdExportable::assertFrom(GJLevelList* list) {
    ASSERT_OR_TERMINATE_INTO(auto res, "Creating GmdExportable from list", GmdExportable::from(list));
    return res;
}
Result<GmdExportable> GmdExportable::from(GJGameLevel* level) {
    if (level->m_levelType != GJLevelType::Editor) {
        return Err("Attempted to introduce a non-editor level to the BetterSave category system");
    }
    return Ok(GmdExportable(level));
}
Result<GmdExportable> GmdExportable::from(GJLevelList* list) {
    if (list->m_listType != 2) {
        return Err("Attempted to introduce a non-editor list to the BetterSave category system");
    }
    return Ok(GmdExportable(list));
}
Result<GmdExportable> GmdExportable::importFrom(ghc::filesystem::path const& dir) {
    if (ghc::filesystem::exists(dir / "level.gmd")) {
        GEODE_UNWRAP_INTO(auto level, gmd::importGmdAsLevel(dir / "level.gmd"));
        return GmdExportable::from(level);
    }
    if (ghc::filesystem::exists(dir / "list.gmdl")) {
        GEODE_UNWRAP_INTO(auto list, gmd::importGmdAsList(dir / "list.gmdl"));
        return GmdExportable::from(list);
    }
    return Err("Directory does not have a 'level.gmd' or 'list.gmdl' file!");
}

ghc::filesystem::path GmdExportable::getSaveDirFor(Category* category, GmdExportable exportable) {
    return exportable.visit(makeVisitor {
        [&](Ref<GJGameLevel> level) {
            return category->getLevelsPath() / level->getID();
        },
        [&](Ref<GJLevelList> list) {
            return category->getListsPath() / list->getID();
        },
    });
}

bool GmdExportable::operator==(GmdExportable const& other) const {
    return std::visit(makeVisitor {
        [&](Ref<GJGameLevel> level, Ref<GJGameLevel> other) {
            return
                (level->getID() == other->getID()) ||
                (
					level->m_levelName == other->m_levelName &&
					level->m_objectCount == other->m_objectCount
                );
        },
        [&](Ref<GJLevelList> list, Ref<GJLevelList> other) {
            return
                (list->getID() == other->getID()) ||
                (
					list->m_listName == other->m_listName &&
					std::vector(list->m_levels) == std::vector(other->m_levels)
                );
        },
        [&](auto, auto) {
            return false;
        }
    }, m_value, other.m_value);
}

std::string GmdExportable::getID() const {
    return std::visit(makeVisitor {
        [&](Ref<GJGameLevel> level) {
            return level->getID();
        },
        [&](Ref<GJLevelList> list) {
            return list->getID();
        },
    }, m_value);
}
void GmdExportable::setID(std::string const& id) {
    return std::visit(makeVisitor {
        [&](Ref<GJGameLevel> level) {
            return level->setID(id);
        },
        [&](Ref<GJLevelList> list) {
            return list->setID(id);
        },
    }, m_value);
}
std::string GmdExportable::getName() const {
    return std::visit(makeVisitor {
        [](Ref<GJGameLevel> level) {
            return level->m_levelName;
        },
        [](Ref<GJLevelList> list) {
            return list->m_listName;
        },
    }, m_value);
}
CategoryInfo* GmdExportable::getCategoryInfo() const {
    return static_cast<CategoryInfo*>(std::visit(makeVisitor {
        [](Ref<GJGameLevel> level) {
            return level->getUserObject("category"_spr);
        },
        [](Ref<GJLevelList> list) {
            return list->getUserObject("category"_spr);
        },
    }, m_value));
}
void GmdExportable::setCategoryInfo(CategoryInfo* info) {
    return std::visit(makeVisitor {
        [&](Ref<GJGameLevel> level) {
            return level->setUserObject("category"_spr, info);
        },
        [&](Ref<GJLevelList> list) {
            return list->setUserObject("category"_spr, info);
        },
    }, m_value);
}

Result<> GmdExportable::exportTo(ghc::filesystem::path const& dir) const {
    return std::visit(makeVisitor {
        [&](Ref<GJGameLevel> level) {
            return gmd::exportLevelAsGmd(level, dir / "level.gmd");
        },
        [&](Ref<GJLevelList> list) {
            return gmd::exportListAsGmd(list, dir / "list.gmdl");
        },
    }, m_value);
}

GJGameLevel* GmdExportable::asLevel() const {
    if (auto r = std::get_if<Ref<GJGameLevel>>(&m_value)) {
        return *r;
    }
    return nullptr;
}
GJLevelList* GmdExportable::asList() const {
    if (auto r = std::get_if<Ref<GJLevelList>>(&m_value)) {
        return *r;
    }
    return nullptr;
}

CategoryInfo* CategoryInfo::from(GmdExportable level, Category* defaultCategory, std::string const& existingID) {
    if (auto existing = level.getCategoryInfo()) {
        auto info = static_cast<CategoryInfo*>(existing);
        if (defaultCategory && info->m_category != defaultCategory) {
            return nullptr;
        }
        return info;
    }
    if (!defaultCategory) {
        return nullptr;
    }
    
    // Create new info and return that
    auto info = new CategoryInfo();
    info->autorelease();
    info->m_level = level;
    info->m_category = defaultCategory;

    // If the level wasn't loaded from disk, save it to disk
    if (existingID.empty()) {
        // Synthesize an ID for the level by taking the level name in kebab-case 
        // and then adding an incrementing number at the end until there exists 
        // no folder with the same name already
        auto name = convertToKebabCase(level.getName());
        
        // Prevent names that are too long (some people might use input bypass 
        // to give levels absurdly long names)
        if (name.size() > 20) {
            name = name.substr(0, 20);
        }
        if (name.empty()) {
            name = "unnamed";
        }

        // Check that no one has made a level called CON
        checkReservedFilenames(name);

        auto id = name;
        size_t counter = 0;

        // Check all subdirectories of `/levels/`; it is possible some external 
        // programs / other mods / later updates may create directories with more 
        // levels there, and though that is not adviced, this ensures ID conflicts 
        // shouldn't happen unless they do something really stupid
        auto dirs = file::readDirectory(getLevelsSaveDir()).unwrapOrDefault();
        while ([&] {
            for (auto dir : dirs) {
                if (ghc::filesystem::exists(dir / id)) {
                    return true;
                }
            }
            return false;
        }()) {
            id = fmt::format("{}-{}", name, counter);
            counter += 1;
        }
        
        // Reserve the ID by creating a folder for the category
        ASSERT_OR_TERMINATE(
            "Creating level save directory",
            file::createDirectoryAll(info->getSaveDir())
                .expect("Unable to create directory '{}': {error}", info->getSaveDir())
        );
        level.setID(id);

        // Save level data
        ASSERT_OR_TERMINATE("Saving level data", info->save());
    }
    // Otherwise just set the ID based on the one loaded from disk
    else {
		log::debug("using existing non-empty ID '{}'", existingID);
        level.setID(existingID);
    }

    // Store the created info object in the level
    level.setCategoryInfo(info);

    return info;
}

Category* CategoryInfo::getCategory() const {
    return m_category;
}
GmdExportable CategoryInfo::getLevel() const {
    return m_level;
}
ghc::filesystem::path CategoryInfo::getSaveDir() const {
    return GmdExportable::getSaveDirFor(m_category, m_level);
}
std::string CategoryInfo::getID() const {
    return m_level.getID();
}

Result<> CategoryInfo::save() const {
    // Ensure directory exists for the level
    GEODE_UNWRAP(file::createDirectoryAll(this->getSaveDir()));

    // Save the level as a .gmd file
    auto res = m_level.exportTo(this->getSaveDir());
    if (!res) {
        return Err("Unable to save level data: {}", res.unwrapErr());
    }

    return Ok();
}
Result<> CategoryInfo::permanentlyDelete() {
	// Delete the level's save directory
	std::error_code ec;
	ghc::filesystem::remove_all(this->getSaveDir(), ec);
	if (ec) {
		return Err("Unable to delete level directory: {} (error code {})", ec.message(), ec.value());
	}

	// Delete from in-memory container. The level and this CategoryInfo still 
    // stay in memory because they both have a strong reference to each other
    m_category->onRemove(m_level);

	auto ev = TrashLevelEvent();
	ev.m_impl->level = m_level;
	ev.m_impl->mode = TrashLevelEvent::Impl::Delete;
	ev.post();

    // Remove the Category user object to allow the memory to be freed
    // SAFETY: This function is the *only way* to remove a level from the 
    // category system - if any function comes in the future that can also 
    // do that, it should ALWAYS set the category object to null to prevent a 
    // circular reference!
    m_level.setCategoryInfo(nullptr);

	return Ok();
}

Category::Category() {}

void Category::onLoadFinished() {}

Result<> Category::add(GmdExportable level) {
    auto info = CategoryInfo::from(level);
    bool isNew = false;

    // Move from category to another
    if (info) {
        log::debug("Moving level '{}' to another category", level.getID());

        // If the level is already in this category, there's nothing to do
        if (info->m_category == this) {
            return Ok();
        }

        // Make sure the category directory exists
        ASSERT_OR_TERMINATE("Creating category levels directory", file::createDirectoryAll(this->getLevelsPath()));
        ASSERT_OR_TERMINATE("Creating category lists directory", file::createDirectoryAll(this->getListsPath()));

        // Move the directory to the other category
        std::error_code ec;
        ghc::filesystem::rename(info->getSaveDir(), GmdExportable::getSaveDirFor(this, info->getLevel()), ec);
        if (ec) {
            return Err("Unable to move level directory: {} (code {})", ec.message(), ec.value());
        }

        // Remove from other category's memory
        info->m_category->onRemove(level);
    }
    // Brand nuevo nivel
    else {
        info = CategoryInfo::from(level, this);
        isNew = true;
        log::debug("Added level '{}' to category system", level.getID());
    }

    // Add level to this category
    info->m_category = this;
    this->onAdd(level, isNew);

    return Ok();
}
void Category::loadLevels() {
    std::vector<ghc::filesystem::path> paths;
    ranges::push(paths, file::readDirectory(this->getLevelsPath()).unwrapOrDefault());
    
    // In some categories (trashcan), lists and levels are stored in the same place
    if (this->getLevelsPath() != this->getListsPath()) {
        ranges::push(paths, file::readDirectory(this->getListsPath()).unwrapOrDefault());
    }

	// Read levels directory
	for (auto levelDir : std::move(paths)) {
		// Skip non-dirs; all levels should be in their own directory
		if (!ghc::filesystem::is_directory(levelDir)) {
			continue;
		}
		auto id = levelDir.filename().string();

		// Load level from GMD file
        auto levelRes = GmdExportable::importFrom(levelDir);
		if (!levelRes) {
			log::error("Unable to load {}: {}", levelDir, levelRes.unwrapErr());
			continue;
		}
		auto level = levelRes.unwrap();

        // Add category info
        auto info = CategoryInfo::from(level, this, id);
        if (!info) {
            // This should be unreachable!
            terminate("Unable to categorize '{}'", id);
        }
        this->onLoad(levelDir, level);

		log::debug("Loaded '{}'", id);
	}
    this->onLoadFinished();
}
