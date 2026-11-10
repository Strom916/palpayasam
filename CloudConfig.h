#pragma once

#include <string>

struct WeaponConfig
{
    bool smooth;
    bool silent;
    bool rapid;
    bool recoil;
    int smoothFov;
    int silentFov;
};

struct VisualConfig
{
    bool esp;
    bool box;
    bool filledBox;
    bool skeleton;
    bool health;
    bool armor;
    bool name;
    bool distance;
    bool weaponEsp;
    bool snapline;
};

struct CloudConfig
{
    WeaponConfig weapon;
    VisualConfig visual;
    std::string version;
};

namespace Cloud
{
    bool DownloadConfig(
        const std::string& license,
        const std::string& hwid,
        CloudConfig& outConfig
    );
    bool UploadConfig(
        const std::string& license,
        const std::string& hwid,
        const CloudConfig& config
    );
}