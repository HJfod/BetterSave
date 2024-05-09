#pragma once

#include "API.hpp"
#include <Geode/binding/GJGameLevel.hpp>

namespace save {
    using TrashClock = std::chrono::system_clock;
    using TrashTime = std::chrono::time_point<std::chrono::system_clock>;
    using TrashUnit = std::chrono::minutes;

    class HJFOD_BETTERSAVE_DLL TrashedLevel final {
    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;

        TrashedLevel();

    public:
        static TrashedLevel create(GJGameLevel* level, TrashTime const& time);
        ~TrashedLevel();

        GJGameLevel* getLevel() const;
        TrashTime getTrashTime() const;
    };

    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getLevelsSaveDir();
    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getLevelSaveDir(GJGameLevel* level);
    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getTrashcanDir();

    HJFOD_BETTERSAVE_DLL geode::Result<> moveLevelToTrash(GJGameLevel* level);
    HJFOD_BETTERSAVE_DLL geode::Result<> untrashLevel(GJGameLevel* level);
    HJFOD_BETTERSAVE_DLL geode::Result<> permanentlyDelete(GJGameLevel* level);
    HJFOD_BETTERSAVE_DLL geode::Result<> clearTrash();
    HJFOD_BETTERSAVE_DLL std::vector<TrashedLevel> getLevelsInTrash();
    HJFOD_BETTERSAVE_DLL bool isLevelTrashed(GJGameLevel* level);

    HJFOD_BETTERSAVE_DLL void openTrashcanPopup();

    class HJFOD_BETTERSAVE_DLL TrashLevelEvent final : public geode::Event {
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;

        TrashLevelEvent();

        friend geode::Result<> moveLevelToTrash(GJGameLevel* level);
        friend geode::Result<> untrashLevel(GJGameLevel* level);
        friend geode::Result<> permanentlyDelete(GJGameLevel* level);
        friend geode::Result<> clearTrash();

    public:
        ~TrashLevelEvent();

        // Might be null if this is a permanent delete for all trash levels
        GJGameLevel* getLevel() const;
        bool isPermanentDelete() const;
        bool isTrash() const;
        bool isUntrash() const;
    };
}
