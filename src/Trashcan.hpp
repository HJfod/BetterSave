#pragma once

#include <BetterSave.hpp>
#include "Category.hpp"

using namespace geode::prelude;
using namespace save;

class Trashcan : public Category {
public:
    // If the impl ever gains more fields, replace this def instead of modifying 
    // the public API!
    using Level = trashcan::TrashedLevel;

protected:
    std::vector<Level> m_levels;

    friend class Trash;

    void onLoad(ghc::filesystem::path const& dir, GJGameLevel* level) override;
    void onAdd(GJGameLevel* level, bool isNew) override;
    void onRemove(GJGameLevel* level) override;

public:
    static Trashcan* get();

    ghc::filesystem::path getPath() const override;
    std::vector<Level> getAllLevels() const;
    
    Result<> clear();
};
