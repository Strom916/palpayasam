#pragma once

#include <string>

std::string GetMachineId();

bool Authenticate(const std::string& username, const std::string& password, const std::string& hwid);
bool Authenticate(const std::string& username, const std::string& password,
	const std::string& hwid, std::string& errorMessage);
bool Authenticate(const std::string& username, const std::string& password,
	const std::string& hwid, bool rememberDevice,
	std::string& sessionToken, std::string& errorMessage);
bool ValidateRememberedSession(const std::string& sessionToken,
	const std::string& hwid, std::string& errorMessage, bool heartbeat = false);
bool LoadRememberedSession(std::string& sessionToken);
bool StoreRememberedSession(const std::string& sessionToken);
void ClearRememberedSession();
void SetActiveSession(const std::string& sessionToken, const std::string& hwid);
bool GetActiveSession(std::string& sessionToken, std::string& hwid);
void ClearActiveSession();
void SetActiveLicense(const std::string& license);
bool GetActiveLicense(std::string& license);
