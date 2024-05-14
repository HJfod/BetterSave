#pragma once

#include <BetterSave.hpp>
#include "Category.hpp"

using namespace geode::prelude;
using namespace save;

struct LevelsMetadata final {
	std::vector<std::string> levelOrder;
};

class CreatedLevels : public Category {
protected:
    void onLoad(ghc::filesystem::path const& dir, GJGameLevel* level) override;
    void onLoadFinished() override;
    void onAdd(GJGameLevel* level, bool isNew) override;
    void onRemove(GJGameLevel* level) override;

public:
    static CreatedLevels* get();

    ghc::filesystem::path getPath() const override;
    std::vector<GJGameLevel*> getAllLevels() const;

    void migrate();

    void saveMetadata() const;
    LevelsMetadata loadMetadata() const;
};
