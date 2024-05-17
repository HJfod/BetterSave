#pragma once

#include <BetterSave.hpp>
#include "Category.hpp"

using namespace geode::prelude;
using namespace save;

struct LevelsMetadata final {
	std::vector<std::string> levelOrder;
};
struct ListsMetadata final {
	std::vector<std::string> listOrder;
};

class CreatedLevels : public Category {
protected:
    void onLoad(ghc::filesystem::path const& dir, GmdExportable level) override;
    void onLoadFinished() override;
    void onAdd(GmdExportable level, bool isNew) override;
    void onRemove(GmdExportable level) override;

public:
    static CreatedLevels* get();

    ghc::filesystem::path getLevelsPath() const override;
    ghc::filesystem::path getListsPath() const override;
    std::vector<Ref<GJGameLevel>> getAllLevels() const;
    std::vector<Ref<GJLevelList>> getAllLists() const;
    std::vector<GmdExportable> getAll() const;

    void migrate();
    void recoverLists();

    void saveLevelsMetadata() const;
    LevelsMetadata loadLevelsMetadata() const;
    void saveListsMetadata() const;
    ListsMetadata loadListsMetadata() const;
};
