#pragma once

#include <BetterSave.hpp>
#include "Category.hpp"

using namespace geode::prelude;
using namespace save;

class TrashLevelEvent::Impl final {
public:
	GmdExportable level;
	enum { Delete, Trash, Untrash } mode;
};


