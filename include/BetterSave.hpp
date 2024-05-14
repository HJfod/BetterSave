#pragma once

#include "API.hpp"
#include <Geode/binding/GJGameLevel.hpp>

class Category;
class CategoryInfo;
class CreatedLevels;
class Trashcan;

namespace save {
    using geode::Result;

    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getLevelsSaveDir();
    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getCurrentLevelSaveDir(GJGameLevel* level);

    namespace created {
        HJFOD_BETTERSAVE_DLL ghc::filesystem::path getCreatedLevelsDir();
    }

    namespace trashcan {
        using Clock = std::chrono::system_clock;
        using TimePoint = std::chrono::time_point<Clock>;
        using Unit = std::chrono::minutes;

        struct TrashedLevel final {
            geode::Ref<GJGameLevel> level;
            TimePoint trashTime;
        };

        HJFOD_BETTERSAVE_DLL ghc::filesystem::path getTrashDir();
        HJFOD_BETTERSAVE_DLL bool isTrashed(GJGameLevel* level);
        HJFOD_BETTERSAVE_DLL std::vector<TrashedLevel> getTrashedLevels();
        HJFOD_BETTERSAVE_DLL Result<> clearTrash();
        HJFOD_BETTERSAVE_DLL void openTrashcanPopup();
    }

    HJFOD_BETTERSAVE_DLL Result<> trash(GJGameLevel* level);
    HJFOD_BETTERSAVE_DLL Result<> untrash(GJGameLevel* level);
    HJFOD_BETTERSAVE_DLL Result<> permanentlyDelete(GJGameLevel* level);

    class HJFOD_BETTERSAVE_DLL TrashLevelEvent final : public geode::Event {
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;

        TrashLevelEvent();

        friend class Category;
        friend class CategoryInfo;
        friend class CreatedLevels;
        friend class Trashcan;

    public:
        ~TrashLevelEvent();

        // Might be null if this is a permanent delete for all trash levels
        GJGameLevel* getLevel() const;
        bool isPermanentDelete() const;
        bool isTrash() const;
        bool isUntrash() const;
    };
}
