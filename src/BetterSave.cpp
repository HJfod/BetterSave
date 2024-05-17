#include <BetterSave.hpp>
#include "CreatedLevels.hpp"
#include "Trashcan.hpp"
#include "TrashcanPopup.hpp"

using namespace geode::prelude;
using namespace save;

// Global

ghc::filesystem::path save::getLevelsSaveDir() {
	return dirs::getSaveDir() / "levels";
}
ghc::filesystem::path save::getCurrentLevelSaveDir(GJGameLevel* level) {
	auto exportable = GmdExportable::from(level);
	if (!exportable) return ghc::filesystem::path();
    auto info = CategoryInfo::from(*exportable);
	if (!info) return ghc::filesystem::path();
	return info->getSaveDir();
}

Result<> save::trash(GJGameLevel* level) {
	GEODE_UNWRAP_INTO(auto gmd, GmdExportable::from(level));
    return Trashcan::get()->add(gmd);
}
Result<> save::untrash(GJGameLevel* level) {
	GEODE_UNWRAP_INTO(auto gmd, GmdExportable::from(level));
    return CreatedLevels::get()->add(gmd);
}
Result<> save::permanentlyDelete(GJGameLevel* level) {
	GEODE_UNWRAP_INTO(auto gmd, GmdExportable::from(level));
    auto info = CategoryInfo::from(gmd);
	if (!info) {
		return Err("Level is not part of the BetterSave category system");
	}
	return info->permanentlyDelete();
}

Result<> save::trash(GJLevelList* list) {
	GEODE_UNWRAP_INTO(auto gmd, GmdExportable::from(list));
    return Trashcan::get()->add(gmd);
}
Result<> save::untrash(GJLevelList* list) {
	GEODE_UNWRAP_INTO(auto gmd, GmdExportable::from(list));
    return CreatedLevels::get()->add(gmd);
}
Result<> save::permanentlyDelete(GJLevelList* list) {
	GEODE_UNWRAP_INTO(auto gmd, GmdExportable::from(list));
    auto info = CategoryInfo::from(gmd);
	if (!info) {
		return Err("List is not part of the BetterSave category system");
	}
	return info->permanentlyDelete();
}

// Trashcan

ghc::filesystem::path trashcan::getTrashDir() {
    return Trashcan::get()->getPath();
}
bool trashcan::isTrashed(GJGameLevel* level) {
	auto gmd = GmdExportable::from(level);
    return gmd ? CategoryInfo::from(*gmd, Trashcan::get()) : nullptr;
}
bool trashcan::isTrashed(GJLevelList* list) {
	auto gmd = GmdExportable::from(list);
    return gmd ? CategoryInfo::from(*gmd, Trashcan::get()) : nullptr;
}
std::vector<trashcan::TrashedLevel> trashcan::getTrashedLevels() {
    return Trashcan::get()->getAllLevels();
}
std::vector<trashcan::TrashedList> trashcan::getTrashedLists() {
    return Trashcan::get()->getAllLists();
}
Result<> trashcan::clearTrash() {
    return Trashcan::get()->clear();
}
void trashcan::openTrashcanPopup() {
	if (trashcan::getTrashedLevels().empty()) {
		FLAlertLayer::create(
			"Trash is Empty",
			"You have not <co>trashed</c> any levels!",
			"OK"
		)->show();
	}
	else {
		TrashcanPopup::create()->show();
	}
}

// Created levels

VersionInfo save::getExistingBetterSaveVersion() {
	auto path = save::getLevelsSaveDir() / ".version";

	// If no version file exists, then we're in v1
	auto version = VersionInfo(1, 0, 0);

	if (ghc::filesystem::exists(path)) {
		if (auto res = file::readString(path)) {
			if (auto v = VersionInfo::parse(*res)) {
				version = *v;
			}
		}
	}
	return version;
}
VersionInfo save::getCurrentBetterSaveVersion() {
	return Mod::get()->getVersion();
}

ghc::filesystem::path created::getCreatedLevelsDir() {
    return CreatedLevels::get()->getLevelsPath();
}
ghc::filesystem::path created::getCreatedListsDir() {
    return CreatedLevels::get()->getListsPath();
}
Result<> created::saveLevel(GJGameLevel* level) {
	GEODE_UNWRAP_INTO(auto gmd, GmdExportable::from(level));
	auto info = CategoryInfo::from(gmd);
	if (!info) {
		return Err("Level is not part of the BetterSave category system");
	}
	return info->save();
}
