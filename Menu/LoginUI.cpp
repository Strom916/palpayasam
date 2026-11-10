#include "main.h"
#include "LoginUI.h"
#include "../Cheat/Auth.h"
#include "CloudConfig.h"

#include <chrono>
#include <cstring>
#include <future>
#include <string>
#include <utility>

namespace
{
	struct LoginTaskResult
	{
		bool success;
		bool restoreCheck;
		std::string sessionToken;
		std::string hwid;
		std::string license;
		std::string message;
	};
}

void RenderLoginUI()
{
	static char username[96] = {};
	static char password[96] = {};
	static bool rememberDevice = false;
	static bool attempted = false;
	static float fade = 0.0f;
	static bool authPending = false;
	static bool restoreCheckStarted = false;
	static bool restoreCheckPending = false;
	static std::future<LoginTaskResult> authTask;
	static std::string authMessage;

	

	if (authPending && authTask.valid() &&
		authTask.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
	{
		const LoginTaskResult result = authTask.get();
		authPending = false;
		restoreCheckPending = false;
		attempted = !result.success && !result.restoreCheck;
		authMessage = result.message;
		if (result.success && !result.restoreCheck)
		{
			SetActiveSession(result.sessionToken, result.hwid);
			if (rememberDevice)
				StoreRememberedSession(result.sessionToken);
			else
				ClearRememberedSession();
		}
		if (result.restoreCheck && !result.success)
		{
			ClearActiveSession();
			ClearRememberedSession();
			attempted = true;
			if (authMessage.empty()) authMessage = "Saved session expired. Sign in again.";
		}
		if (result.success && pMenu)
		{
			if (result.restoreCheck)
				SetActiveSession(result.sessionToken, result.hwid);
			pMenu->bLoggedIn = true;
			pMenu->bShowLogin = false;
			pMenu->bOpen = true;
		}
	}

	const float delta = ImGui::GetIO().DeltaTime;
	fade = fade + (1.0f - fade) * (delta * 8.0f < 1.0f ? delta * 8.0f : 1.0f);

	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		ImVec2(0.0f, 0.0f), displaySize, ImColor(3, 7, 15, (int)(115.0f * fade)));

	ImGui::SetNextWindowSize(ImVec2(440.0f, 290.0f), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f),
		ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 12.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 11.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.014f, 0.025f, 0.045f, 0.96f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.50f, 1.0f, 0.55f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.025f, 0.045f, 0.075f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.035f, 0.085f, 0.145f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.025f, 0.070f, 0.125f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 0.50f, 1.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.91f, 0.95f, 1.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

	// Restore ImGui focus on reopen and after Windows gives the game focus back.
	if (pMenu && pMenu->bLoginFocusPending)
	{
		ImGui::SetNextWindowFocus();
		pMenu->bLoginFocusPending = false;
	}
	ImGui::Begin("STROM Login##Demo", NULL,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar);

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 windowMin = ImGui::GetWindowPos();
	const ImVec2 windowSize = ImGui::GetWindowSize();
	const ImVec2 windowMax(windowMin.x + windowSize.x, windowMin.y + windowSize.y);
	draw->AddRect(ImVec2(windowMin.x + 2.0f, windowMin.y + 2.0f),
		ImVec2(windowMax.x - 2.0f, windowMax.y - 2.0f),
		ImColor(0, 0, 0, 24), 13.0f, 0, 1.0f);

	if (pTextures && pTextures->tLogo)
	{
		const float logoWidth = 185.0f;
		ImGui::SetCursorPos(ImVec2((windowSize.x - logoWidth) * 0.5f, 0.0f));
		ImGui::Image(pTextures->tLogo, ImVec2(logoWidth, 56.0f));
	}
	else
	{
		const char* brand = "STROM";
		const ImVec2 brandSize = ImGui::CalcTextSize(brand);
		ImGui::SetCursorPos(ImVec2((windowSize.x - brandSize.x) * 0.5f, 13.0f));
		ImGui::TextColored(ImVec4(0.0f, 0.50f, 1.0f, 1.0f), "%s", brand);
	}

	const char* subtitle = "Secure License Authentication";
	const ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle);
	ImGui::SetCursorPos(ImVec2((windowSize.x - subtitleSize.x) * 0.5f, 54.0f));
	ImGui::TextColored(ImVec4(0.62f, 0.69f, 0.79f, 1.0f), "%s", subtitle);

	ImGui::SetCursorPos(ImVec2(18.0f, 70.0f));
	ImGui::TextColored(ImVec4(0.70f, 0.79f, 0.91f, 1.0f), "License ID");
	ImGui::SetCursorPos(ImVec2(18.0f, 82.0f));
	ImGui::SetNextItemWidth(windowSize.x - 36.0f);
	if (pMenu && pMenu->bLoginFieldFocusPending)
	{
		ImGui::SetKeyboardFocusHere();
		pMenu->bLoginFieldFocusPending = false;
	}
	ImGui::InputText("##LoginUsername", username, sizeof(username));

	ImGui::SetCursorPos(ImVec2(18.0f, 120.0f));
	ImGui::TextColored(ImVec4(0.70f, 0.79f, 0.91f, 1.0f), "Password");
	ImGui::SetCursorPos(ImVec2(18.0f, 132.0f));
	ImGui::SetNextItemWidth(windowSize.x - 36.0f);
	ImGui::InputText("##LoginPassword", password, sizeof(password), ImGuiInputTextFlags_Password);

	ImGui::SetCursorPos(ImVec2(18.0f, 171.0f));
	if (ImGui::Checkbox("Remember Device", &rememberDevice) && !rememberDevice)
		ClearRememberedSession();

	const char* statusText = authPending ? (restoreCheckPending ? "Checking saved device..." : "Connecting...") :
		(attempted ? (authMessage.empty() ? "Invalid License" : authMessage.c_str()) : "Ready");
	if (std::strlen(statusText) > 48)
		statusText = attempted ? "Invalid License" : (restoreCheckPending ? "Checking saved device..." : "Connecting...");
	const ImVec2 statusTextSize = ImGui::CalcTextSize(statusText);
	const float statusGroupWidth = statusTextSize.x + 16.0f;
	const float statusLeft = (windowSize.x - statusGroupWidth) * 0.5f;
	ImGui::SetCursorPos(ImVec2(statusLeft, 192.0f));
	const ImVec2 statusPos = ImGui::GetCursorScreenPos();
	if (attempted)
	{
		draw->AddCircleFilled(ImVec2(statusPos.x + 4.0f, statusPos.y + 8.0f), 3.0f,
			ImColor(255, 87, 97, (int)(255.0f * fade)));
		ImGui::SetCursorPosX(statusLeft + 13.0f);
		ImGui::TextColored(ImVec4(1.0f, 0.34f, 0.38f, 1.0f), "%s", statusText);
	}
	else if (authPending)
	{
		draw->AddCircleFilled(ImVec2(statusPos.x + 4.0f, statusPos.y + 8.0f), 3.0f,
			ImColor(0, 127, 255, (int)(255.0f * fade)));
		ImGui::SetCursorPosX(statusLeft + 13.0f);
		ImGui::TextColored(ImVec4(0.25f, 0.66f, 1.0f, 1.0f), "%s", statusText);
	}
	else
	{
		draw->AddCircleFilled(ImVec2(statusPos.x + 4.0f, statusPos.y + 8.0f), 3.0f,
			ImColor(145, 158, 178, (int)(255.0f * fade)));
		ImGui::SetCursorPosX(statusLeft + 13.0f);
		ImGui::TextColored(ImVec4(0.62f, 0.69f, 0.79f, 1.0f), "%s", statusText);
	}

	ImGui::SetCursorPos(ImVec2(18.0f, 210.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
	if (ImGui::Button("Activate", ImVec2(windowSize.x - 36.0f, 40.0f)) && !authPending)
	{
		const std::string submittedUsername(username);
		const std::string submittedPassword(password);
		const std::string deviceId = GetMachineId();
		const bool shouldRemember = rememberDevice;
		attempted = false;
		authMessage.clear();
		authPending = true;
		authTask = std::async(std::launch::async,
			[submittedUsername, submittedPassword, deviceId, shouldRemember]()
			{
				LoginTaskResult result = {};
				result.restoreCheck = false;
				result.hwid = deviceId;
				result.success = Authenticate(submittedUsername, submittedPassword, deviceId,
					shouldRemember, result.sessionToken, result.message);
				return result;
			});
	}
	ImGui::PopStyleColor();

	const bool buttonHovered = ImGui::IsItemHovered();
	const ImVec2 buttonMin = ImGui::GetItemRectMin();
	const ImVec2 buttonMax = ImGui::GetItemRectMax();
	const int buttonAlpha = buttonHovered ? 255 : 235;
	if (buttonHovered)
		draw->AddRect(ImVec2(buttonMin.x - 2.0f, buttonMin.y - 2.0f),
			ImVec2(buttonMax.x + 2.0f, buttonMax.y + 2.0f),
			ImColor(0, 127, 255, (int)(75.0f * fade)), 12.0f, 0, 2.0f);
	draw->AddRectFilled(buttonMin, buttonMax,
		ImColor(0, 127, 255, (int)(buttonAlpha * fade)), 10.0f);
	draw->PushClipRect(ImVec2(buttonMin.x + 10.0f, buttonMin.y),
		ImVec2(buttonMax.x - 10.0f, buttonMax.y), true);
	draw->AddRectFilledMultiColor(buttonMin, buttonMax,
		ImColor(0, 127, 255, (int)(buttonAlpha * fade)),
		ImColor(41, 166, 255, (int)(buttonAlpha * fade)),
		ImColor(41, 166, 255, (int)(buttonAlpha * fade)),
		ImColor(0, 127, 255, (int)(buttonAlpha * fade)));
	draw->PopClipRect();

	const char* buttonText = "Activate";
	const ImVec2 buttonTextSize = ImGui::CalcTextSize(buttonText);
	draw->AddText(ImVec2(buttonMin.x + (buttonMax.x - buttonMin.x - buttonTextSize.x) * 0.5f,
		buttonMin.y + (buttonMax.y - buttonMin.y - buttonTextSize.y) * 0.5f),
		ImColor(255, 255, 255, (int)(255.0f * fade)), buttonText);

	ImGui::SetCursorPos(ImVec2(18.0f, 256.0f));
	ImGui::TextColored(ImVec4(0.48f, 0.57f, 0.69f, 1.0f),
		"Sign-ins record device, network, player and server details.");

	ImGui::End();

	ImDrawList* foreground = ImGui::GetForegroundDrawList();
	foreground->AddRect(ImVec2(windowMin.x - 2.0f, windowMin.y - 2.0f),
		ImVec2(windowMax.x + 2.0f, windowMax.y + 2.0f),
		ImColor(0, 127, 255, (int)(24.0f * fade)), 16.0f, 0, 4.0f);
	foreground->AddRect(ImVec2(windowMin.x - 1.0f, windowMin.y - 1.0f),
		ImVec2(windowMax.x + 1.0f, windowMax.y + 1.0f),
		ImColor(0, 127, 255, (int)(52.0f * fade)), 15.0f, 0, 1.0f);
	foreground->AddText(ImVec2(windowMax.x - 82.0f, windowMax.y - 18.0f),
		ImColor(135, 157, 184, (int)(190.0f * fade)), "STROM v1.0.0");

	ImGui::PopStyleColor(10);
	ImGui::PopStyleVar(7);
}
