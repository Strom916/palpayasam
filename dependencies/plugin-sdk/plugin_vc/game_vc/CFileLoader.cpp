/*
    Plugin-SDK (Grand Theft Auto Vice City) source file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "CFileLoader.h"

RpClump* CFileLoader::LoadAtomicFile2Return(const char* file) {
    return plugin::CallAndReturn<RpClump*, 0x48D7C0, const char*>(file);
}
