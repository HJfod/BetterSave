#pragma once

#include "API.hpp"
#include <Geode/binding/GJGameLevel.hpp>

namespace save {
    class LevelCategory;

    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getLevelsSaveDir();
    HJFOD_BETTERSAVE_DLL ghc::filesystem::path getTrashcanDir();
}
