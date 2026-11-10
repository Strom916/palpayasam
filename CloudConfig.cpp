#include "main.h"
#include "CloudConfig.h"
#include <windows.h>
#include <winhttp.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

bool Cloud::DownloadConfig(
    const std::string& license,
    const std::string& hwid,
    CloudConfig& out)
{
    HINTERNET hSession = WinHttpOpen(
        L"STROM/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"milan-01.vexanode.gg",
        19178,
        0
    );

    if (!hConnect)
        return false;

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        L"/api/config/load",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpSetTimeouts(
        hRequest,
        3000,
        3000,
        5000,
        5000
    );

    json requestBody;

    requestBody["license"] = license;
    requestBody["hwid"] = hwid;

    std::string body = requestBody.dump();

    const wchar_t* headers =
        L"Content-Type: application/json\r\n";

    WinHttpSendRequest(
        hRequest,
        headers,
        -1L,
        (LPVOID)body.c_str(),
        body.length(),
        body.length(),
        0);

    WinHttpReceiveResponse(hRequest, NULL);

    std::string response;
    DWORD size = 0;

    do
    {
        WinHttpQueryDataAvailable(hRequest, &size);

        if (!size)
            break;

        char* buffer = new char[size + 1];
        ZeroMemory(buffer, size + 1);

        DWORD downloaded = 0;
        WinHttpReadData(hRequest, buffer, size, &downloaded);

        response += std::string(buffer, downloaded);

        delete[] buffer;

    } while (size);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    auto j = json::parse(response, nullptr, false);

    if (j.is_discarded())
        return false;

    if (!j["success"])
        return false;

    auto cfg = j["config"];

    out.version = j["version"];

    out.weapon.smooth = cfg["weapon"]["smooth"];
    out.weapon.silent = cfg["weapon"]["silent"];
    out.weapon.rapid = cfg["weapon"]["rapid"];
    out.weapon.recoil = cfg["weapon"]["recoil"];
    out.weapon.smoothFov = cfg["weapon"]["smoothFov"];
    out.weapon.silentFov = cfg["weapon"]["silentFov"];

    out.visual.esp = cfg["visual"]["esp"];
    out.visual.box = cfg["visual"]["box"];
    out.visual.filledBox = cfg["visual"]["filledBox"];
    out.visual.skeleton = cfg["visual"]["skeleton"];
    out.visual.health = cfg["visual"]["health"];
    out.visual.armor = cfg["visual"]["armor"];
    out.visual.name = cfg["visual"]["name"];
    out.visual.distance = cfg["visual"]["distance"];
    out.visual.weaponEsp = cfg["visual"]["weaponEsp"];
    out.visual.snapline = cfg["visual"]["snapline"];

    return true;
}

bool Cloud::UploadConfig(
    const std::string& license,
    const std::string& hwid,
    const CloudConfig& cfg
)
{
    json body;

    body["license"] = license;
    body["hwid"] = hwid;
    body["version"] = cfg.version;

    body["config"]["weapon"] = {
        {"smooth", cfg.weapon.smooth},
        {"silent", cfg.weapon.silent},
        {"rapid", cfg.weapon.rapid},
        {"recoil", cfg.weapon.recoil},
        {"smoothFov", cfg.weapon.smoothFov},
        {"silentFov", cfg.weapon.silentFov}
    };

    body["config"]["visual"] = {
        {"esp", cfg.visual.esp},
        {"box", cfg.visual.box},
        {"filledBox", cfg.visual.filledBox},
        {"skeleton", cfg.visual.skeleton},
        {"health", cfg.visual.health},
        {"armor", cfg.visual.armor},
        {"name", cfg.visual.name},
        {"distance", cfg.visual.distance},
        {"weaponEsp", cfg.visual.weaponEsp},
        {"snapline", cfg.visual.snapline}
    };

    std::string payload = body.dump();

    HINTERNET hSession = WinHttpOpen(
        L"STROM/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"milan-01.vexanode.gg",
        19178,
        0
    );

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        L"/api/config/save",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );
    if (!hRequest)
    {
        OutputDebugStringA("[STROM CLOUD] WinHttpOpenRequest FAILED\n");

        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return false;
    }

    WinHttpSetTimeouts(
        hRequest,
        3000,
        3000,
        5000,
        5000
    );

    const wchar_t* headers =
        L"Content-Type: application/json\r\n";

    BOOL ok = WinHttpSendRequest(
        hRequest,
        headers,
        -1L,
        (LPVOID)payload.c_str(),
        (DWORD)payload.size(),
        (DWORD)payload.size(),
        0);

    if (ok)
        ok = WinHttpReceiveResponse(hRequest, NULL);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return ok == TRUE;
}