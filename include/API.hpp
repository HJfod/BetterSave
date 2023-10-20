#pragma once

#ifdef GEODE_IS_WINDOWS
    #ifdef EXPORTING_HJFOD_BETTERSAVE
        #define HJFOD_BETTERSAVE_DLL __declspec(dllexport)
    #else
        #define HJFOD_BETTERSAVE_DLL __declspec(dllimport)
    #endif
#else
    #define HJFOD_BETTERSAVE_DLL
#endif
