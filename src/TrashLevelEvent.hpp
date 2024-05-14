#pragma once

#include <BetterSave.hpp>

using namespace geode::prelude;
using namespace save;

class TrashLevelEvent::Impl final {
public:
	Ref<GJGameLevel> level;
	enum { Delete, Trash, Untrash } mode;
};


