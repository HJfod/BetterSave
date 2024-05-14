#pragma once

#include <BetterSave.hpp>

using namespace geode::prelude;
using namespace save;

class Category;

class CategoryInfo : public CCObject {
private:
    Category* m_category;
    GJGameLevel* m_level;

    friend class Category;

public:
    // Might be null if the level isn't an editor level
    static CategoryInfo* from(GJGameLevel* level, Category* defaultCategory = nullptr, std::string const& id = "");

    Category* getCategory() const;
    GJGameLevel* getLevel() const;
    ghc::filesystem::path getSaveDir() const;
    std::string getID() const;

    Result<> saveLevel() const;
    Result<> permanentlyDelete();
};

class Category {
private:
    virtual void onLoad(ghc::filesystem::path const& dir, GJGameLevel* level) = 0;
    virtual void onLoadFinished();
    virtual void onAdd(GJGameLevel* level, bool isNew) = 0;
    virtual void onRemove(GJGameLevel* level) = 0;

    friend class CategoryInfo;

public:
    Category();

    virtual ghc::filesystem::path getPath() const = 0;
    virtual ~Category() = default;

    Result<> add(GJGameLevel* level);
    void loadLevels();
};
