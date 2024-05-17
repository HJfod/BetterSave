#pragma once

#include <BetterSave.hpp>
#include "Category.hpp"

using namespace geode::prelude;
using namespace save;

class Trashcan : public Category {
public:
    struct Level final {
        GmdExportable level;
        trashcan::TimePoint trashTime;
    };

protected:
    std::vector<Level> m_levels;

    friend class Trash;

    void onLoad(ghc::filesystem::path const& dir, GmdExportable level) override;
    void onAdd(GmdExportable level, bool isNew) override;
    void onRemove(GmdExportable level) override;

public:
    static Trashcan* get();

    ghc::filesystem::path getPath() const;
    ghc::filesystem::path getLevelsPath() const override;
    ghc::filesystem::path getListsPath() const override;
    std::vector<trashcan::TrashedLevel> getAllLevels() const;
    std::vector<trashcan::TrashedList> getAllLists() const;
    std::vector<Level> getAll() const;
    
    Result<> clear();
};
