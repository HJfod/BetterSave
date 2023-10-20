#pragma once

#include "API.hpp"
#include <Geode/binding/GJGameLevel.hpp>

namespace save {
    class LevelCategory;

    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getLevelsSaveDir();

    HJFOD_BETTERSAVE_DLL void moveLevelToCategory(GJGameLevel* level, LevelCategory* category);
    HJFOD_BETTERSAVE_DLL LevelCategory* getCategoryForLevel(GJGameLevel* level);

    /**
     * Categories enable having levels in different folders
     */
    class HJFOD_BETTERSAVE_DLL LevelCategory {
    private:
        std::vector<Ref<GJGameLevel>> m_levels;

        // ! Do not manually call these!!
        // !`moveLevelToCategory` is the only one who should call these!

        virtual void addLevel(GJGameLevel* level);
        virtual void removeLevel(GJGameLevel* level);

        friend void moveLevelToCategory(GJGameLevel* level, LevelCategory* category);
        
    public:
        virtual ~LevelCategory() = default;
        virtual std::string getID() const = 0;

        std::vector<Ref<GJGameLevel>> getLevels() const;
        ghc::filesystem::path getDir() const;
    };

    HJFOD_BETTERSAVE_DLL void loadLevelsForCategory(LevelCategory* category);

    HJFOD_BETTERSAVE_DLL LevelCategory* getLocalLevelsCategory();
    HJFOD_BETTERSAVE_DLL LevelCategory* getTrashcanCategory();
}
