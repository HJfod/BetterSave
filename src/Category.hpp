#pragma once

#include <BetterSave.hpp>

using namespace geode::prelude;
using namespace save;

class Category;
class CategoryInfo;

class GmdExportable final {
private:
    std::variant<Ref<GJGameLevel>, Ref<GJLevelList>> m_value;

    template <class L>
    GmdExportable(L* lvl) : m_value(Ref(lvl)) {}

public:
    GmdExportable() = default;

    static Result<GmdExportable> from(GJGameLevel* level);
    static Result<GmdExportable> from(GJLevelList* list);
    static Result<GmdExportable> importFrom(ghc::filesystem::path const& dir);

    static ghc::filesystem::path getSaveDirFor(Category* category, GmdExportable exportable);

    bool operator==(GmdExportable const& other) const;

    std::string getID() const;
    void setID(std::string const& id);
    std::string getName() const;
    CategoryInfo* getCategoryInfo() const;
    void setCategoryInfo(CategoryInfo* info);

    Result<> exportTo(ghc::filesystem::path const& dir) const;

    template <class T>
    auto visit(T&& visitor) const {
        return std::visit(visitor, m_value);
    }

    GJGameLevel* asLevel() const;
    GJLevelList* asList() const;
};

class CategoryInfo : public CCObject {
private:
    Category* m_category;
    GmdExportable m_level;

    friend class Category;

public:
    // Might be null if the level isn't an editor level
    static CategoryInfo* from(GmdExportable level, Category* defaultCategory = nullptr, std::string const& id = "");

    Category* getCategory() const;
    GmdExportable getLevel() const;
    ghc::filesystem::path getSaveDir() const;
    std::string getID() const;

    Result<> save() const;
    Result<> permanentlyDelete();
};

class Category {
private:
    virtual void onLoad(ghc::filesystem::path const& dir, GmdExportable level) = 0;
    virtual void onLoadFinished();
    virtual void onAdd(GmdExportable level, bool isNew) = 0;
    virtual void onRemove(GmdExportable level) = 0;

    friend class CategoryInfo;

public:
    Category();

    virtual ghc::filesystem::path getLevelsPath() const = 0;
    virtual ghc::filesystem::path getListsPath() const = 0;
    virtual ~Category() = default;

    Result<> add(GmdExportable level);
    void loadLevels();
};
