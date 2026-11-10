#include "main.h"
#include <sstream>
#include <unordered_map>
#include <locale>
#include <cmath>

struct stConfig g_Config;
CConfig* pConfig = new CConfig();

CConfig::CConfig()
{
	memset(g_Config.g_Aimbot.bAimbotEnabled, false, sizeof(g_Config.g_Aimbot.bAimbotEnabled));

	for (int i = 22; i <= 34; i++)
	{
		g_Config.g_Aimbot.iAimbotConfig[i][RANGE] = 1;
		g_Config.g_Aimbot.iAimbotConfig[i][SILENT] = 1;
		g_Config.g_Aimbot.iAimbotConfig[i][SMOOTH] = 1;
	}
}

const stHotkeyBind g_HotkeyBinds[] =
{
	{ "Aimbot",         &g_Config.g_Aimbot.bAimbot,         &g_Config.g_Hotkeys.iToggleAimbot,        0 },
	{ "Silent Aim",     &g_Config.g_Aimbot.bSilent,         &g_Config.g_Hotkeys.iToggleSilent,        0 },
	{ "Smooth Aim",     &g_Config.g_Aimbot.bSmooth,         &g_Config.g_Hotkeys.iToggleSmooth,        0 },
	{ "Pro Aim",        &g_Config.g_Aimbot.bProAim,         &g_Config.g_Hotkeys.iToggleProAim,        0 },
	{ "Triggerbot",     &g_Config.g_Aimbot.bTriggerbot,     &g_Config.g_Hotkeys.iToggleTriggerbot,    0 },
	{ "No Recoil",      &g_Config.g_Combat.bNoRecoil,       &g_Config.g_Hotkeys.iToggleNoRecoil,      0 },
	{ "Rapid Fire",     &g_Config.g_Combat.bRapidFire,      &g_Config.g_Hotkeys.iToggleRapidFire,     0 },
	{ "No Reload",      &g_Config.g_Combat.bNoReload,       &g_Config.g_Hotkeys.iToggleNoReload,      0 },
	{ "Fast Crosshair", &g_Config.g_Combat.bFastCrosshair,  &g_Config.g_Hotkeys.iToggleFastCrosshair, 0 },
	{ "Name Tags",      &g_Config.g_Visuals.bNameTagsESP,   &g_Config.g_Hotkeys.iToggleNameESP,       1 },
	{ "Box ESP",        &g_Config.g_Visuals.bBoxESP,        &g_Config.g_Hotkeys.iToggleBoxESP,        1 },
	{ "Bone ESP",       &g_Config.g_Visuals.bBoneESP,       &g_Config.g_Hotkeys.iToggleBoneESP,       1 },
};
const int g_iHotkeyBindCount = _countof(g_HotkeyBinds);

// ---------------------------------------------------------------------------
// config file: %APPDATA%\STROM\config.ini, plain "key=value" lines
// ---------------------------------------------------------------------------
namespace
{
	enum eEntryType { E_BOOL, E_INT, E_FLOAT, E_COLOR };

	struct stEntry
	{
		std::string szKey;
		eEntryType type;
		void* pData;
	};

	stConfig g_Snapshot;
	DWORD g_dwLastCheck = 0;
	DWORD g_dwLastSave = 0;

	std::vector<stEntry>& Registry()
	{
		static std::vector<stEntry> vec;
		if (!vec.empty())
			return vec;

		auto addBool = [&](const std::string& k, bool* p) { vec.push_back({ k, E_BOOL, p }); };
		auto addInt = [&](const std::string& k, int* p) { vec.push_back({ k, E_INT, p }); };
		auto addFloat = [&](const std::string& k, float* p) { vec.push_back({ k, E_FLOAT, p }); };
		auto addColor = [&](const std::string& k, ImVec4* p) { vec.push_back({ k, E_COLOR, p }); };

#define CFG_BOOL(sec, name)  addBool(#sec "." #name, &g_Config.g_##sec.name)
#define CFG_INT(sec, name)   addInt(#sec "." #name, &g_Config.g_##sec.name)
#define CFG_FLOAT(sec, name) addFloat(#sec "." #name, &g_Config.g_##sec.name)
#define CFG_COLOR(sec, name) addColor(#sec "." #name, &g_Config.g_##sec.name)

		// aimbot
		CFG_BOOL(Aimbot, bAimbot); CFG_BOOL(Aimbot, bSilent); CFG_BOOL(Aimbot, bSmooth); CFG_BOOL(Aimbot, bProAim); CFG_BOOL(Aimbot, bTriggerbot);
		CFG_FLOAT(Aimbot, fSilentAccuracy);
		CFG_BOOL(Aimbot, bSmoothIsFire); CFG_BOOL(Aimbot, bStopOnBody); CFG_BOOL(Aimbot, bSmoothLockY);
		CFG_BOOL(Aimbot, bTriggerDelay); CFG_INT(Aimbot, iTriggerDelay); CFG_BOOL(Aimbot, bTriggerSync);
		CFG_BOOL(Aimbot, bTeamProtect); CFG_BOOL(Aimbot, bLockThroughObjects); CFG_BOOL(Aimbot, bIgnoreMaxDistance);
		CFG_BOOL(Aimbot, bIgnoreAFK); CFG_BOOL(Aimbot, bIgnoreEverything);
		CFG_INT(Aimbot, iRangeStyle); CFG_BOOL(Aimbot, bDrawRange); CFG_BOOL(Aimbot, bDrawTracer);
		CFG_FLOAT(Aimbot, fOutlineThickness); CFG_COLOR(Aimbot, colorRange);
		for (int w = 0; w < 35; w++)
		{
			addBool("Aimbot.bAimbotEnabled." + std::to_string(w), &g_Config.g_Aimbot.bAimbotEnabled[w]);
			for (int c = 0; c < 3; c++)
				addInt("Aimbot.iAimbotConfig." + std::to_string(w) + "." + std::to_string(c), &g_Config.g_Aimbot.iAimbotConfig[w][c]);
		}

		// combat
		CFG_BOOL(Combat, bNoReload); CFG_BOOL(Combat, bAllowCBug); CFG_BOOL(Combat, bCBugFreeze); CFG_BOOL(Combat, bCBugAnim);
		CFG_BOOL(Combat, bCBugText); CFG_BOOL(Combat, bCBugWeapon); CFG_BOOL(Combat, bRapidFire); CFG_FLOAT(Combat, fRapidFire);
		CFG_BOOL(Combat, bFastCrosshair); CFG_BOOL(Combat, bNoRecoil); CFG_BOOL(Combat, bWeaponSwitcher); CFG_BOOL(Combat, bFastSwitch);

		// player
		CFG_BOOL(Player, bAntiStun); CFG_INT(Player, iAntiStunChance); CFG_BOOL(Player, bFastRotation); CFG_INT(Player, iFastRotation);
		CFG_BOOL(Player, bFastSprint); CFG_FLOAT(Player, fFastSprint); CFG_BOOL(Player, bAirBreak); CFG_INT(Player, iAirBreak);
		CFG_BOOL(Player, bMapRun); CFG_INT(Player, iMapRun); CFG_BOOL(Player, bInfiniteRun); CFG_BOOL(Player, bInfiniteOxygen);
		CFG_BOOL(Player, bNoFall); CFG_BOOL(Player, bChangeSkin); CFG_INT(Player, iSkinID); CFG_BOOL(Player, bFastRespawn);
		CFG_BOOL(Player, bUpsideMode); CFG_BOOL(Player, bInvertWalk); CFG_BOOL(Player, bShakeMode); CFG_BOOL(Player, bInvalidAimZ);
		CFG_BOOL(Player, bInvisible); CFG_BOOL(Player, bFakeLag); CFG_INT(Player, iFakeLag);
		CFG_BOOL(Player, bStopOnExitVehicle); CFG_INT(Player, iStopOnExitVehicle);
		CFG_BOOL(Player, bAutoBikeSpam); CFG_BOOL(Player, bMotorBikeSpam);

		// visuals
		CFG_BOOL(Visuals, bNameTagsESP); CFG_BOOL(Visuals, bWeaponESP); CFG_BOOL(Visuals, bSnipelineESP); CFG_INT(Visuals, iSnipelineStyle);
		CFG_BOOL(Visuals, bBoxESP); CFG_INT(Visuals, iBoxStyle); CFG_BOOL(Visuals, bBoneESP); CFG_BOOL(Visuals, bDistanceESP);
		CFG_BOOL(Visuals, bChamsESP); CFG_FLOAT(Visuals, fESPThickness); CFG_BOOL(Visuals, bESPTeammates); CFG_BOOL(Visuals, bESPCustomColors);
		CFG_BOOL(Visuals, bFilledBox); CFG_COLOR(Visuals, colorVisible); CFG_COLOR(Visuals, colorNotVisible);
		CFG_BOOL(Visuals, bVehicleNameTagsESP); CFG_BOOL(Visuals, bVehicleHPESP); CFG_BOOL(Visuals, bVehicleStatusESP);
		CFG_BOOL(Visuals, bVehicleEngineESP); CFG_BOOL(Visuals, bVehicleDistanceESP);
		CFG_BOOL(Visuals, bDMGInformer); CFG_INT(Visuals, iFontSize); CFG_FLOAT(Visuals, fDMGTick); CFG_COLOR(Visuals, colorDMG);
		CFG_BOOL(Visuals, bFPSFuncs); CFG_BOOL(Visuals, bFPSUnlock); CFG_BOOL(Visuals, bFPSBoost); CFG_BOOL(Visuals, bFakeFPS);
		CFG_INT(Visuals, iMinFPS); CFG_INT(Visuals, iMaxFPS); CFG_BOOL(Visuals, bNoDrugsEffect);
		CFG_BOOL(Visuals, bAspectRatio); CFG_FLOAT(Visuals, fAspectRatio);
		CFG_BOOL(Visuals, bWeatherChanger); CFG_INT(Visuals, iWeather); CFG_BOOL(Visuals, bTimeChanger); CFG_INT(Visuals, iTime);

		// developer
		CFG_BOOL(Developer, bDamager); CFG_BOOL(Developer, bDelay); CFG_INT(Developer, iDelay); CFG_BOOL(Developer, bTeamProtect);
		CFG_BOOL(Developer, bTeleportToPlayer); CFG_BOOL(Developer, bSendBulletData); CFG_BOOL(Developer, bCustomWeapon);
		CFG_INT(Developer, iWeaponID); CFG_FLOAT(Developer, fDamage); CFG_BOOL(Developer, bCustomDamage);

		// hotkeys
		for (int i = 0; i < 5; i++)
			addInt("Hotkeys.iWeaponSwitch." + std::to_string(i), &g_Config.g_Hotkeys.iWeaponSwitch[i]);
		CFG_INT(Hotkeys, iAutoBikeSpam); CFG_INT(Hotkeys, iMotorBikeSpam);
		CFG_INT(Hotkeys, iToggleAimbot); CFG_INT(Hotkeys, iToggleSilent); CFG_INT(Hotkeys, iToggleSmooth); CFG_INT(Hotkeys, iToggleProAim);
		CFG_INT(Hotkeys, iToggleTriggerbot); CFG_INT(Hotkeys, iToggleNoRecoil); CFG_INT(Hotkeys, iToggleRapidFire); CFG_INT(Hotkeys, iToggleNoReload);
		CFG_INT(Hotkeys, iToggleFastCrosshair); CFG_INT(Hotkeys, iToggleNameESP); CFG_INT(Hotkeys, iToggleBoxESP); CFG_INT(Hotkeys, iToggleBoneESP);
		CFG_BOOL(Hotkeys, bToggleNotify);

		// style
		for (int i = 0; i < ImGuiCol_COUNT; i++)
			addColor("Style.vecTheme." + std::to_string(i), &g_Config.g_Style.vecTheme[i]);

#undef CFG_BOOL
#undef CFG_INT
#undef CFG_FLOAT
#undef CFG_COLOR
		return vec;
	}

	std::string Format(const stEntry& e)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		ss.precision(9);
		switch (e.type)
		{
		case E_BOOL:  ss << (*(bool*)e.pData ? 1 : 0); break;
		case E_INT:   ss << *(int*)e.pData; break;
		case E_FLOAT: ss << *(float*)e.pData; break;
		case E_COLOR:
		{
			const ImVec4* c = (const ImVec4*)e.pData;
			ss << c->x << ',' << c->y << ',' << c->z << ',' << c->w;
			break;
		}
		}
		return ss.str();
	}

	bool Apply(const stEntry& e, const std::string& szValue)
	{
		std::istringstream ss(szValue);
		ss.imbue(std::locale::classic());
		switch (e.type)
		{
		case E_BOOL:
		{
			int i = 0;
			if (!(ss >> i)) return false;
			*(bool*)e.pData = (i != 0);
			return true;
		}
		case E_INT:
		{
			int i = 0;
			if (!(ss >> i)) return false;
			*(int*)e.pData = i;
			return true;
		}
		case E_FLOAT:
		{
			float f = 0.f;
			if (!(ss >> f) || !std::isfinite(f)) return false;
			*(float*)e.pData = f;
			return true;
		}
		case E_COLOR:
		{
			float r = 0.f, g = 0.f, b = 0.f, a = 0.f;
			char c1 = 0, c2 = 0, c3 = 0;
			if (!(ss >> r >> c1 >> g >> c2 >> b >> c3 >> a)) return false;
			if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b) || !std::isfinite(a)) return false;
			*(ImVec4*)e.pData = ImVec4(r, g, b, a);
			return true;
		}
		}
		return false;
	}

	void Sanitize()
	{
		// szKeyNames only goes up to VK 0xA5, anything else is a bad file
		auto fixKey = [](int& k) { if (k < 0 || k > 0xA5) k = 0; };

		for (int& k : g_Config.g_Hotkeys.iWeaponSwitch)
			fixKey(k);
		fixKey(g_Config.g_Hotkeys.iAutoBikeSpam);
		fixKey(g_Config.g_Hotkeys.iMotorBikeSpam);
		for (int i = 0; i < g_iHotkeyBindCount; i++)
			fixKey(*g_HotkeyBinds[i].pKey);
	}

	const std::wstring& GetPathW()
	{
		static std::wstring szPath;
		if (!szPath.empty())
			return szPath;

		std::wstring szDir;
		wchar_t szBuf[MAX_PATH] = {};
		const DWORD dwLen = GetEnvironmentVariableW(L"APPDATA", szBuf, MAX_PATH);
		if (dwLen > 0 && dwLen < MAX_PATH)
			szDir = std::wstring(szBuf) + L"\\STROM";
		else
		{
			// no APPDATA: next to the game exe
			GetModuleFileNameW(NULL, szBuf, MAX_PATH);
			szDir = szBuf;
			const size_t pos = szDir.find_last_of(L"\\/");
			szDir = (pos == std::wstring::npos ? std::wstring() : szDir.substr(0, pos)) + L"\\STROM";
		}

		CreateDirectoryW(szDir.c_str(), NULL);
		szPath = szDir + L"\\config.ini";
		return szPath;
	}

	bool WriteWholeFile(const std::wstring& szPath, const std::string& szData)
	{
		HANDLE hFile = CreateFileW(szPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		DWORD dwWritten = 0;
		const BOOL bOk = WriteFile(hFile, szData.data(), (DWORD)szData.size(), &dwWritten, NULL);
		CloseHandle(hFile);
		return bOk && dwWritten == szData.size();
	}

	bool ReadWholeFile(const std::wstring& szPath, std::string& szOut)
	{
		HANDLE hFile = CreateFileW(szPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		const DWORD dwSize = GetFileSize(hFile, NULL);
		if (dwSize == INVALID_FILE_SIZE || dwSize > (4u << 20)) // config is ~10 KB, anything huge isn't ours
		{
			CloseHandle(hFile);
			return false;
		}

		szOut.resize(dwSize);
		DWORD dwRead = 0;
		const BOOL bOk = dwSize == 0 ? TRUE : ReadFile(hFile, &szOut[0], dwSize, &dwRead, NULL);
		CloseHandle(hFile);
		if (!bOk) return false;
		szOut.resize(dwRead);
		return true;
	}
}

bool ConfigFile::Save()
{
	std::string szData = "# STROM config v1\n";
	for (const stEntry& e : Registry())
		szData += e.szKey + "=" + Format(e) + "\n";

	// write to a temp file first so a crash mid-write can't eat the real config
	const std::wstring& szPath = GetPathW();
	const std::wstring szTmp = szPath + L".tmp";
	if (!WriteWholeFile(szTmp, szData))
		return false;

	if (!MoveFileExW(szTmp.c_str(), szPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		DeleteFileW(szTmp.c_str());
		return false;
	}

	g_dwLastSave = GetTickCount();
	if (g_dwLastSave == 0) g_dwLastSave = 1;
	return true;
}

bool ConfigFile::Load()
{
	std::string szData;
	if (!ReadWholeFile(GetPathW(), szData))
		return false;

	std::unordered_map<std::string, std::string> map;
	std::istringstream in(szData);
	std::string szLine;
	while (std::getline(in, szLine))
	{
		if (!szLine.empty() && szLine.back() == '\r')
			szLine.pop_back();
		if (szLine.empty() || szLine[0] == '#')
			continue;

		const size_t eq = szLine.find('=');
		if (eq == std::string::npos)
			continue;
		map[szLine.substr(0, eq)] = szLine.substr(eq + 1);
	}

	// keys missing from the file keep their defaults, unknown keys are ignored
	for (const stEntry& e : Registry())
	{
		auto it = map.find(e.szKey);
		if (it != map.end())
			Apply(e, it->second);
	}

	Sanitize();
	return true;
}

void ConfigFile::Init()
{
	if (!Load())
		Save(); // first run: create the file

	memcpy(&g_Snapshot, &g_Config, sizeof(stConfig));
	g_dwLastCheck = GetTickCount();
}

void ConfigFile::Update()
{
	const DWORD dwNow = GetTickCount();
	if (dwNow - g_dwLastCheck < 500)
		return;
	g_dwLastCheck = dwNow;

	if (memcmp(&g_Snapshot, &g_Config, sizeof(stConfig)) == 0)
		return;

	Save();
	memcpy(&g_Snapshot, &g_Config, sizeof(stConfig));
}

const char* ConfigFile::GetPath()
{
	static std::string szUtf8;
	if (szUtf8.empty())
	{
		const std::wstring& szPath = GetPathW();
		const int iLen = WideCharToMultiByte(CP_UTF8, 0, szPath.c_str(), -1, NULL, 0, NULL, NULL);
		if (iLen > 1)
		{
			szUtf8.resize(iLen - 1);
			WideCharToMultiByte(CP_UTF8, 0, szPath.c_str(), -1, &szUtf8[0], iLen, NULL, NULL);
		}
	}
	return szUtf8.c_str();
}

int ConfigFile::SecondsSinceSave()
{
	if (g_dwLastSave == 0)
		return -1;
	return (int)((GetTickCount() - g_dwLastSave) / 1000);
}
