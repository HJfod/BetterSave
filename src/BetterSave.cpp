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

Result<> save::trash(GJGameLevel* level) {
    return Trashcan::get()->add(level);
}
Result<> save::untrash(GJGameLevel* level) {
    return CreatedLevels::get()->add(level);
}
Result<> save::permanentlyDelete(GJGameLevel* level) {
    auto info = CategoryInfo::from(level);
	if (!info) {
		return Err("Level is not part of the BetterSave category system");
	}
	return info->permanentlyDelete();
}

// Trashcan

ghc::filesystem::path trashcan::getTrashDir() {
    return Trashcan::get()->getPath();
}
bool trashcan::isTrashed(GJGameLevel* level) {
    return CategoryInfo::from(level, Trashcan::get());
}
std::vector<trashcan::TrashedLevel> trashcan::getTrashedLevels() {
    return Trashcan::get()->getAllLevels();
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

ghc::filesystem::path created::getCreatedLevelsDir() {
    return CreatedLevels::get()->getPath();
}
