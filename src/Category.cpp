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
    }
}

CategoryInfo* CategoryInfo::from(GJGameLevel* level, Category* defaultCategory, std::string const& existingID) {
    if (level->m_levelType != GJLevelType::Editor) {
        terminate(
            "Attempted to categorize a non-editor level. "
            "This implies some code is wrongly passing an arbitary GJGameLevel to CategoryInfo"
        );
    }
    if (auto existing = level->getUserObject("category"_spr)) {
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
        auto name = convertToKebabCase(level->m_levelName);
        
        // Prevent names that are too long (some people might use input bypass 
        // to give levels absurdly long names)
        if (name.size() > 20) {
            name = name.substr(0, 20);
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
            file::createDirectoryAll(defaultCategory->getPath() / id)
                .expect("Unable to create directory '{}': {error}", defaultCategory->getPath() / id)
        );
        level->setID(id);

        // Save level data
        ASSERT_OR_TERMINATE("Saving level data", info->saveLevel());
    }
    // Otherwise just set the ID based on the one loaded from disk
    else {
        level->setID(existingID);
    }

    // Store the created info object in the level
    level->setUserObject("category"_spr, info);

    return info;
}

Category* CategoryInfo::getCategory() const {
    return m_category;
}
GJGameLevel* CategoryInfo::getLevel() const {
    return m_level;
}
ghc::filesystem::path CategoryInfo::getSaveDir() const {
    return m_category->getPath() / m_level->getID();
}
std::string CategoryInfo::getID() const {
    return m_level->getID();
}

Result<> CategoryInfo::saveLevel() const {
    // Ensure directory exists for the level
    GEODE_UNWRAP(file::createDirectoryAll(this->getSaveDir()));

    // Save the level as a .gmd file
    auto res = gmd::exportLevelAsGmd(m_level, this->getSaveDir() / "level.gmd");
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
    m_level->setUserObject("category"_spr, nullptr);

	return Ok();
}

Category::Category() {}

void Category::onLoadFinished() {}

Result<> Category::add(GJGameLevel* level) {
    auto info = CategoryInfo::from(level);
    bool isNew = false;

    // Move from category to another
    if (info) {
        log::debug("Moving level '{}' to another category", level->getID());

        // If the level is already in this category, there's nothing to do
        if (info->m_category == this) {
            return Ok();
        }

        // Make sure the category directory exists
        ASSERT_OR_TERMINATE("Creating category directory", file::createDirectoryAll(this->getPath()));

        // Move the directory to the other category
        std::error_code ec;
        ghc::filesystem::rename(info->getSaveDir(), this->getPath() / info->getID(), ec);
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
        log::debug("Added level '{}' to category system", level->getID());
    }

    // Add level to this category
    info->m_category = this;
    this->onAdd(level, isNew);

    return Ok();
}
void Category::loadLevels() {
	// Read levels directory
	for (auto levelDir : file::readDirectory(this->getPath()).unwrapOrDefault()) {
		// Skip non-dirs; all levels should be in their own directory
		if (!ghc::filesystem::is_directory(levelDir)) {
			continue;
		}
		auto id = levelDir.filename().string();

		// Load level from GMD file
		auto levelRes = gmd::importGmdAsLevel(levelDir / "level.gmd");
		if (!levelRes) {
			log::error("Unable to load {}: {}", levelDir / "level.gmd", levelRes.unwrapErr());
			continue;
		}

		auto level = levelRes.unwrap();

        // Add category info
        auto info = CategoryInfo::from(level, this, id);
        if (!info) {
            // This should be unreachable!
            terminate("Unable to categorize '{}'", id);
        }

		level->setID(id);
        this->onLoad(levelDir, level);

		log::debug("Loaded '{}'", id);
	}
    this->onLoadFinished();
}
