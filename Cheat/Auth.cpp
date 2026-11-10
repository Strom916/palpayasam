#include "main.h"
#include "Auth.h"

#include <Windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <dpapi.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

namespace
{
	const wchar_t* kApiHost = L"milan-01.vexanode.gg";
	const INTERNET_PORT kApiPort = 19178;
	const wchar_t* kApiPath = L"/api/login";
	const wchar_t* kSessionPath = L"/api/session/validate";
	std::mutex gActiveSessionMutex;
	std::string gActiveSessionToken;
	std::string gActiveSessionHwid;
	std::string gActiveLicense;

	std::string ToHex(const std::vector<unsigned char>& bytes)
	{
		std::ostringstream out;
		out << std::hex << std::setfill('0');
		for (unsigned char byte : bytes)
			out << std::setw(2) << static_cast<unsigned int>(byte);
		return out.str();
	}

	bool Sha256(const std::string& input, std::string& output)
	{
		BCRYPT_ALG_HANDLE algorithm = nullptr;
		BCRYPT_HASH_HANDLE hash = nullptr;
		DWORD objectSize = 0, hashSize = 0, bytes = 0;
		std::vector<unsigned char> object, digest;
		bool ok = false;

		if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
			return false;
		if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &bytes, 0) < 0 ||
			BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
			reinterpret_cast<PUCHAR>(&hashSize), sizeof(hashSize), &bytes, 0) < 0)
			goto cleanup;

		object.resize(objectSize);
		digest.resize(hashSize);
		if (BCryptCreateHash(algorithm, &hash, object.data(), objectSize, nullptr, 0, 0) < 0)
			goto cleanup;
		if (!input.empty() && BCryptHashData(hash,
			reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())),
			static_cast<ULONG>(input.size()), 0) < 0)
			goto cleanup;
		if (BCryptFinishHash(hash, digest.data(), hashSize, 0) < 0)
			goto cleanup;
		output = ToHex(digest);
		ok = true;

	cleanup:
		if (hash) BCryptDestroyHash(hash);
		if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
		return ok;
	}

	std::string JsonEscape(const std::string& value)
	{
		std::ostringstream out;
		for (unsigned char c : value)
		{
			switch (c)
			{
			case '"': out << "\\\""; break;
			case '\\': out << "\\\\"; break;
			case '\b': out << "\\b"; break;
			case '\f': out << "\\f"; break;
			case '\n': out << "\\n"; break;
			case '\r': out << "\\r"; break;
			case '\t': out << "\\t"; break;
			default:
				if (c >= 0x20) out << static_cast<char>(c);
				break;
			}
		}
		return out.str();
	}

	bool JsonBool(const std::string& json, const char* key, bool& value)
	{
		const std::string token = std::string("\"") + key + "\"";
		size_t pos = json.find(token);
		if (pos == std::string::npos || (pos = json.find(':', pos + token.size())) == std::string::npos)
			return false;
		++pos;
		while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
		if (json.compare(pos, 4, "true") == 0) { value = true; return true; }
		if (json.compare(pos, 5, "false") == 0) { value = false; return true; }
		return false;
	}

	std::string JsonString(const std::string& json, const char* key)
	{
		const std::string token = std::string("\"") + key + "\"";
		size_t pos = json.find(token);
		if (pos == std::string::npos || (pos = json.find(':', pos + token.size())) == std::string::npos)
			return {};
		pos = json.find('"', pos + 1);
		if (pos == std::string::npos) return {};
		std::string result;
		for (++pos; pos < json.size(); ++pos)
		{
			const char c = json[pos];
			if (c == '"') break;
			if (c == '\\' && pos + 1 < json.size())
			{
				const char next = json[++pos];
				switch (next)
				{
				case 'n': result.push_back('\n'); break;
				case 'r': result.push_back('\r'); break;
				case 't': result.push_back('\t'); break;
				default: result.push_back(next); break;
				}
			}
			else result.push_back(c);
		}
		return result;
	}

	bool HttpPostJson(const wchar_t* path, const std::string& body,
		std::string& response, std::string& errorMessage)
	{
		HINTERNET session = WinHttpOpen(L"STROM Desktop/2.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!session) { errorMessage = "Unable to connect to the login service."; return false; }
		WinHttpSetTimeouts(session, 5000, 5000, 7000, 7000);
		HINTERNET connection = WinHttpConnect(session, kApiHost, kApiPort, 0);
		HINTERNET request = connection ? WinHttpOpenRequest(connection, L"POST", path, nullptr,
			WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0) : nullptr;
		if (!connection || !request)
		{
			if (request) WinHttpCloseHandle(request);
			if (connection) WinHttpCloseHandle(connection);
			WinHttpCloseHandle(session);
			errorMessage = "Unable to connect to the login service.";
			return false;
		}

		const wchar_t* headers = L"Content-Type: application/json\r\nAccept: application/json\r\n";
		BOOL sent = WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L),
			const_cast<char*>(body.data()), static_cast<DWORD>(body.size()),
			static_cast<DWORD>(body.size()), 0);
		BOOL received = sent ? WinHttpReceiveResponse(request, nullptr) : FALSE;
		DWORD statusCode = 0, statusSize = sizeof(statusCode);
		if (received)
			WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

		response.clear();
		if (received)
		{
			DWORD available = 0;
			while (WinHttpQueryDataAvailable(request, &available) && available > 0)
			{
				const size_t oldSize = response.size();
				response.resize(oldSize + available);
				DWORD downloaded = 0;
				if (!WinHttpReadData(request, &response[oldSize], available, &downloaded))
				{
					response.resize(oldSize);
					break;
				}
				response.resize(oldSize + downloaded);
			}
		}
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);
		if (!received || statusCode != 200)
		{
			errorMessage = "Unable to connect to the login service.";
			return false;
		}
		return true;
	}

	bool MakeNonce(std::string& nonce)
	{
		std::array<unsigned char, 16> bytes = {};
		if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()),
			BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
			return false;
		std::vector<unsigned char> vector(bytes.begin(), bytes.end());
		nonce = ToHex(vector);
		return true;
	}

	std::wstring RememberedSessionPath()
	{
		wchar_t appData[MAX_PATH] = {};
		const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
		if (!length || length >= MAX_PATH) return {};
		std::wstring directory(appData);
		directory += L"\\STROM";
		CreateDirectoryW(directory.c_str(), nullptr);
		return directory + L"\\remembered-session.bin";
	}

	std::string GetPlayerNameSafe()
	{
		if (!pSAMP || !pSAMP->isInited || !pSAMP->getPlayers()) return {};
		const char* name = pSAMP->getPlayerName(pSAMP->getPlayers()->sLocalPlayerID);
		return name ? name : "";
	}

	std::string GetServerIpSafe()
	{
		if (!pSAMP || !pSAMP->isInited || !pSAMP->getInfo()) return {};
		return std::string(pSAMP->getInfo()->szIP);
	}

	unsigned int GetServerPortSafe()
	{
		if (!pSAMP || !pSAMP->isInited || !pSAMP->getInfo()) return 0;
		return pSAMP->getInfo()->ulPort;
	}
}

std::string GetMachineId()
{
	char computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
	DWORD computerNameSize = sizeof(computerName);
	GetComputerNameA(computerName, &computerNameSize);

	char windowsPath[MAX_PATH] = {};
	GetWindowsDirectoryA(windowsPath, MAX_PATH);
	char root[] = "C:\\";
	if (windowsPath[0] && windowsPath[1] == ':') root[0] = windowsPath[0];
	DWORD volumeSerial = 0;
	GetVolumeInformationA(root, nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0);

	std::ostringstream identity;
	identity << computerName << ':' << std::hex << volumeSerial;
	std::string hashed;
	if (Sha256(identity.str(), hashed)) return hashed;
	return identity.str();
}

bool Authenticate(const std::string& username, const std::string& password, const std::string& hwid)
{
	std::string errorMessage;
	return Authenticate(username, password, hwid, errorMessage);
}

bool Authenticate(const std::string& username, const std::string& password,
	const std::string& hwid, std::string& errorMessage)
{
	std::string unusedSessionToken;
	return Authenticate(username, password, hwid, false, unusedSessionToken, errorMessage);
}

bool Authenticate(const std::string& username, const std::string& password,
	const std::string& hwid, bool rememberDevice,
	std::string& sessionToken, std::string& errorMessage)
{
 
	errorMessage.clear();
	sessionToken.clear();
	if (username.empty() || password.empty())
	{
		errorMessage = "Enter your license ID and password.";
		return false;
	}

	std::string passwordHash;
	if (!Sha256(password, passwordHash))
	{
		errorMessage = "Could not prepare login request.";
		return false;
	}

	std::string nonce;
	if (!MakeNonce(nonce))
	{
		errorMessage = "Could not prepare login request.";
		return false;
	}
	std::string normalizedUsername = username;
	std::transform(normalizedUsername.begin(), normalizedUsername.end(), normalizedUsername.begin(),
		[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	const std::string machineId = hwid.empty() ? GetMachineId() : hwid;

	std::ostringstream body;
	body << "{\"username\":\"" << JsonEscape(normalizedUsername)
		<< "\",\"passwordHash\":\"" << passwordHash
		<< "\",\"hwid\":\"" << JsonEscape(machineId)
		<< "\",\"nonce\":\"" << nonce
		<< "\",\"rememberDevice\":" << (rememberDevice ? "true" : "false")
		<< ",\"playerName\":\"" << JsonEscape(GetPlayerNameSafe())
		<< "\",\"serverIp\":\"" << JsonEscape(GetServerIpSafe())
		<< "\",\"serverPort\":" << GetServerPortSafe() << '}';
	std::string response;
	if (!HttpPostJson(kApiPath, body.str(), response, errorMessage)) return false;
	bool success = false;
	if (JsonBool(response, "success", success) && success)
	{
		sessionToken = JsonString(response, "sessionToken");

		SetActiveLicense(normalizedUsername);

		return true;
	}

	errorMessage = JsonString(response, "errorMessage");
	if (errorMessage.empty())
		errorMessage = "Invalid License";
	return false;
}

bool ValidateRememberedSession(const std::string& sessionToken,
	const std::string& hwid, std::string& errorMessage, bool heartbeat)
{
	errorMessage.clear();
	if (sessionToken.size() != 64 || hwid.empty())
	{
		errorMessage = "Remembered session is invalid.";
		return false;
	}
	std::string nonce;
	if (!MakeNonce(nonce))
	{
		errorMessage = "Could not prepare login request.";
		return false;
	}
	std::ostringstream body;
	body << "{\"sessionToken\":\"" << JsonEscape(sessionToken)
		<< "\",\"hwid\":\"" << JsonEscape(hwid)
		<< "\",\"nonce\":\"" << nonce
		<< "\",\"heartbeat\":" << (heartbeat ? "true" : "false")
		<< ",\"playerName\":\"" << JsonEscape(GetPlayerNameSafe())
		<< "\",\"serverIp\":\"" << JsonEscape(GetServerIpSafe())
		<< "\",\"serverPort\":" << GetServerPortSafe() << '}';
	std::string response;
	if (!HttpPostJson(kSessionPath, body.str(), response, errorMessage)) return false;
	bool success = false;
	if (JsonBool(response, "success", success) && success)
	{
		std::string license = JsonString(response, "username");

		if (license.empty())
		{
			errorMessage = "Session response did not include username.";
			return false;
		}

		std::transform(
			license.begin(),
			license.end(),
			license.begin(),
			[](unsigned char c)
			{
				return static_cast<char>(std::toupper(c));
			}
		);

		SetActiveLicense(license);

		return true;
	}
	errorMessage = JsonString(response, "errorMessage");
	if (errorMessage.empty()) errorMessage = "Remembered session expired. Sign in again.";
	return false;
}

bool LoadRememberedSession(std::string& sessionToken)
{
	sessionToken.clear();
	const std::wstring path = RememberedSessionPath();
	if (path.empty()) return false;
	HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, nullptr);
	if (file == INVALID_HANDLE_VALUE) return false;
	DWORD size = GetFileSize(file, nullptr);
	if (!size || size > 4096)
	{
		CloseHandle(file);
		return false;
	}
	std::vector<unsigned char> encrypted(size);
	DWORD read = 0;
	const BOOL readOk = ReadFile(file, encrypted.data(), size, &read, nullptr);
	CloseHandle(file);
	if (!readOk || read != size) return false;

	const std::string machineId = GetMachineId();
	DATA_BLOB input = { size, encrypted.data() };
	DATA_BLOB entropy = { static_cast<DWORD>(machineId.size()),
		reinterpret_cast<BYTE*>(const_cast<char*>(machineId.data())) };
	DATA_BLOB output = {};
	if (!CryptUnprotectData(&input, nullptr, &entropy, nullptr, nullptr,
		CRYPTPROTECT_UI_FORBIDDEN, &output)) return false;
	if (output.cbData == 64)
		sessionToken.assign(reinterpret_cast<char*>(output.pbData), output.cbData);
	LocalFree(output.pbData);
	return sessionToken.size() == 64;
}

bool StoreRememberedSession(const std::string& sessionToken)
{
	if (sessionToken.size() != 64) return false;
	const std::wstring path = RememberedSessionPath();
	if (path.empty()) return false;
	const std::string machineId = GetMachineId();
	DATA_BLOB input = { static_cast<DWORD>(sessionToken.size()),
		reinterpret_cast<BYTE*>(const_cast<char*>(sessionToken.data())) };
	DATA_BLOB entropy = { static_cast<DWORD>(machineId.size()),
		reinterpret_cast<BYTE*>(const_cast<char*>(machineId.data())) };
	DATA_BLOB encrypted = {};
	if (!CryptProtectData(&input, L"STROM remembered device", &entropy, nullptr, nullptr,
		CRYPTPROTECT_UI_FORBIDDEN, &encrypted)) return false;
	HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		LocalFree(encrypted.pbData);
		return false;
	}
	DWORD written = 0;
	const BOOL writeOk = WriteFile(file, encrypted.pbData, encrypted.cbData, &written, nullptr);
	CloseHandle(file);
	LocalFree(encrypted.pbData);
	return writeOk && written == encrypted.cbData;
}

void ClearRememberedSession()
{
	const std::wstring path = RememberedSessionPath();
	if (!path.empty()) DeleteFileW(path.c_str());
}

void SetActiveSession(const std::string& sessionToken, const std::string& hwid)
{
	std::lock_guard<std::mutex> lock(gActiveSessionMutex);
	gActiveSessionToken = sessionToken;
	gActiveSessionHwid = hwid;
}

bool GetActiveSession(std::string& sessionToken, std::string& hwid)
{
	std::lock_guard<std::mutex> lock(gActiveSessionMutex);
	sessionToken = gActiveSessionToken;
	hwid = gActiveSessionHwid;
	return !sessionToken.empty() && !hwid.empty();
}

void ClearActiveSession()
{
	std::lock_guard<std::mutex> lock(gActiveSessionMutex);
	gActiveSessionToken.clear();
	gActiveSessionHwid.clear();
}
void SetActiveLicense(const std::string& license)
{
	std::lock_guard<std::mutex> lock(gActiveSessionMutex);
	gActiveLicense = license;
}

bool GetActiveLicense(std::string& license)
{
	std::lock_guard<std::mutex> lock(gActiveSessionMutex);

	license = gActiveLicense;
	return !license.empty();
}
