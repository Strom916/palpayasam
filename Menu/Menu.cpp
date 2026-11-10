#include "main.h"
#include <thread>
#include <fstream>
#include <Windows.h>

#include <chrono>
#include <future>
#include <string>
#include <utility>


CMenu* pMenu;



// toggle hotkeys: polled every tick, ignored while the menu is open, a game menu is up or the chat box is focused
static void PollToggleHotkeys(bool bMenuOpen)
{
	const bool bTyping = pSAMP && pSAMP->getInput() && pSAMP->getInput()->iInputEnabled;
	const bool bBlocked = bMenuOpen || Utils::isGTAMenuActive() || bTyping;

	for (int i = 0; i < g_iHotkeyBindCount; i++)
	{
		const stHotkeyBind& bind = g_HotkeyBinds[i];
		const int iKey = *bind.pKey;
		if (iKey <= 0 || iKey == VK_END) // END is the menu key
			continue;

		// always sample the key so the edge detection stays current while blocked
		const bool bPressed = isKeyPressed((uint8_t)iKey);
		if (!bPressed || bBlocked)
			continue;

		*bind.pValue = !*bind.pValue;

		if (g_Config.g_Hotkeys.bToggleNotify && pSAMP)
			pSAMP->addMessageToChat(0xFFFFFFFF, "{0080FF}[STROM] {FFFFFF}%s: {%s}%s", bind.szName, *bind.pValue ? "00FF66" : "FF4444", *bind.pValue ? "ON" : "OFF");
	}
}

static void DrawToggleBinds(int iGroup)
{
	for (int i = 0; i < g_iHotkeyBindCount; i++)
	{
		const stHotkeyBind& bind = g_HotkeyBinds[i];
		if (bind.iGroup != iGroup)
			continue;

		ImGui::PushID(i);
		ImGui::Checkbox(bind.szName, bind.pValue);
		ImGui::SameLine(205);
		ImGui::PushItemWidth(100);
		ImGui::Hotkey("##bind", bind.pKey);
		ImGui::PopItemWidth();
		ImGui::PopID();
	}
}

void CMenu::LoadConfig()
{
	ConfigFile::Init();

	// Theme() already pushed the defaults, push whatever the config file changed
	for (int i = 0; i < ImGuiCol_COUNT; i++)
		ImGui::GetStyle().Colors[i] = g_Config.g_Style.vecTheme[i];
}

void CMenu::Update()
{
	// iToggle: 0 = state changed (apply cursor), 1 = applied open, 2 = applied closed
	ImGui::GetIO().MouseDrawCursor = bOpen;

	const int iWanted = bOpen ? 1 : 2;
	if (iToggle != iWanted)
	{
		if (pSAMP)
			pSAMP->toggleSAMPCursor(bOpen ? 1 : 0);
		iToggle = iWanted;
	}

	PollToggleHotkeys(bOpen);
	ConfigFile::Update();
}
void CMenu::Theme()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.WindowPadding = ImVec2(12, 12);
	style.WindowMinSize = ImVec2(32, 32);
	style.WindowRounding = 12.0f;
	style.PopupRounding = 10.0f;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

	style.FramePadding = ImVec2(9, 6);
	style.FrameRounding = 7.0f;
	style.ChildRounding = 10.0f;
	style.ItemSpacing = ImVec2(12, 9);
	style.TouchExtraPadding = ImVec2(0, 0);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 0.0f;
	style.ScrollbarSize = 7.0f;
	style.ScrollbarRounding = 7.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 7.0f;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.DisplayWindowPadding = ImVec2(22, 22);
	style.DisplaySafeAreaPadding = ImVec2(4, 4);
	style.AntiAliasedLines = true;
	style.AntiAliasedFill = true;
	style.CurveTessellationTol = 1.f;

	g_Config.g_Style.vecTheme[ImGuiCol_Text] = ImVec4(0.91f, 0.95f, 1.00f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_TextDisabled] = ImVec4(0.43f, 0.51f, 0.62f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_WindowBg] = ImVec4(0.018f, 0.032f, 0.058f, 0.98f);
	g_Config.g_Style.vecTheme[ImGuiCol_ChildBg] = ImVec4(0.035f, 0.065f, 0.105f, 0.96f);
	g_Config.g_Style.vecTheme[ImGuiCol_PopupBg] = ImVec4(0.025f, 0.050f, 0.085f, 0.99f);
	g_Config.g_Style.vecTheme[ImGuiCol_Border] = ImVec4(0.02f, 0.36f, 0.68f, 0.78f);
	g_Config.g_Style.vecTheme[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_FrameBg] = ImVec4(0.035f, 0.065f, 0.105f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_FrameBgHovered] = ImVec4(0.055f, 0.18f, 0.30f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_FrameBgActive] = ImVec4(0.02f, 0.25f, 0.45f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_MenuBarBg] = ImVec4(0.025f, 0.060f, 0.100f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_ScrollbarBg] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.50f, 1.00f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.43f, 0.92f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_SliderGrabActive] = ImVec4(0.22f, 0.67f, 1.00f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_Button] = ImVec4(0.025f, 0.065f, 0.115f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.30f, 0.52f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.43f, 0.76f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_Header] = ImVec4(0.00f, 0.24f, 0.44f, 0.75f);
	g_Config.g_Style.vecTheme[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.36f, 0.62f, 0.85f);
	g_Config.g_Style.vecTheme[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.40f, 0.72f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_Separator] = ImVec4(0.06f, 0.24f, 0.40f, 0.75f);
	g_Config.g_Style.vecTheme[ImGuiCol_SeparatorHovered] = ImVec4(0.00f, 0.50f, 0.90f, 0.85f);
	g_Config.g_Style.vecTheme[ImGuiCol_SeparatorActive] = ImVec4(0.00f, 0.50f, 1.00f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	g_Config.g_Style.vecTheme[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	g_Config.g_Style.vecTheme[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	g_Config.g_Style.vecTheme[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.22f, 0.40f, 0.35f);
	g_Config.g_Style.vecTheme[ImGuiCol_TabActive] = ImVec4(0.00f, 0.24f, 0.43f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
	g_Config.g_Style.vecTheme[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
	g_Config.g_Style.vecTheme[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	g_Config.g_Style.vecTheme[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.50f, 1.00f, 1.00f);
	g_Config.g_Style.vecTheme[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	g_Config.g_Style.vecTheme[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	g_Config.g_Style.vecTheme[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	g_Config.g_Style.vecTheme[ImGuiCol_Logo] = ImVec4(1.f, 1.f, 1.f, 1.f);

	for (int i = 0; i < ImGuiCol_COUNT; i++)
		ImGui::GetStyle().Colors[i] = g_Config.g_Style.vecTheme[i];
}

void CMenu::ActiveTab()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(27 / 255.f, 30 / 255.f, 35 / 255.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));
}

void CMenu::NormalTab()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Tab));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(27 / 255.f, 30 / 255.f, 35 / 255.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.22f, 0.40f, 0.48f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.00f, 0.30f, 0.53f, 0.72f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
}

void CMenu::ActiveButton()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));
}

void CMenu::NormalButton()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
}

void CMenu::Tooltip(const char* szText)
{
	if (strlen(szText) && ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(szText);
		ImGui::EndTooltip();
	}
}


// Decorative sidebar glyphs are drawn separately so the existing tab button
// labels and IDs remain unchanged.
static void DrawSidebarIcon(ImDrawList* draw, int tab, const ImVec2& c, ImU32 color)
{
	const float k = 1.0f;
	switch (tab)
	{
	case 0: // reticle
		draw->AddCircle(c, 7.0f, color, 24, k);
		draw->AddCircleFilled(c, 1.7f, color, 12);
		draw->AddLine(ImVec2(c.x - 11, c.y), ImVec2(c.x - 5, c.y), color, k);
		draw->AddLine(ImVec2(c.x + 5, c.y), ImVec2(c.x + 11, c.y), color, k);
		draw->AddLine(ImVec2(c.x, c.y - 11), ImVec2(c.x, c.y - 5), color, k);
		draw->AddLine(ImVec2(c.x, c.y + 5), ImVec2(c.x, c.y + 11), color, k);
		break;
	case 1: // user
		draw->AddCircle(ImVec2(c.x, c.y - 4.5f), 4.0f, color, 20, 1.5f);
		draw->AddRect(ImVec2(c.x - 8, c.y + 1), ImVec2(c.x + 8, c.y + 10), color, 5.0f, 0, 1.5f);
		break;
	case 2: // eye
		draw->AddLine(ImVec2(c.x - 10, c.y), ImVec2(c.x - 5, c.y - 5), color, 1.5f);
		draw->AddLine(ImVec2(c.x - 5, c.y - 5), ImVec2(c.x + 5, c.y - 5), color, 1.5f);
		draw->AddLine(ImVec2(c.x + 5, c.y - 5), ImVec2(c.x + 10, c.y), color, 1.5f);
		draw->AddLine(ImVec2(c.x + 10, c.y), ImVec2(c.x + 5, c.y + 5), color, 1.5f);
		draw->AddLine(ImVec2(c.x + 5, c.y + 5), ImVec2(c.x - 5, c.y + 5), color, 1.5f);
		draw->AddLine(ImVec2(c.x - 5, c.y + 5), ImVec2(c.x - 10, c.y), color, 1.5f);
		draw->AddCircleFilled(c, 2.5f, color, 16);
		break;
	case 3: // style / shirt
		{
			const ImVec2 p[] = { ImVec2(c.x - 4, c.y - 9), ImVec2(c.x - 10, c.y - 5), ImVec2(c.x - 7, c.y), ImVec2(c.x - 4, c.y - 2), ImVec2(c.x - 4, c.y + 9), ImVec2(c.x + 4, c.y + 9), ImVec2(c.x + 4, c.y - 2), ImVec2(c.x + 7, c.y), ImVec2(c.x + 10, c.y - 5), ImVec2(c.x + 4, c.y - 9), ImVec2(c.x + 2, c.y - 5), ImVec2(c.x - 2, c.y - 5) };
			draw->AddPolyline(p, 12, color, true, 1.5f);
		}
		break;
	case 4: // four tile grid
		draw->AddRectFilled(ImVec2(c.x - 9, c.y - 9), ImVec2(c.x - 1, c.y - 1), color, 2.0f);
		draw->AddRectFilled(ImVec2(c.x + 1, c.y - 9), ImVec2(c.x + 9, c.y - 1), color, 2.0f);
		draw->AddRectFilled(ImVec2(c.x - 9, c.y + 1), ImVec2(c.x - 1, c.y + 9), color, 2.0f);
		draw->AddRectFilled(ImVec2(c.x + 1, c.y + 1), ImVec2(c.x + 9, c.y + 9), color, 2.0f);
		break;
	case 6: // keyboard
		draw->AddRect(ImVec2(c.x - 11, c.y - 6), ImVec2(c.x + 11, c.y + 7), color, 3.0f, 0, 1.5f);
		for (int i = 0; i < 4; ++i)
			draw->AddRectFilled(ImVec2(c.x - 8 + i * 4.5f, c.y - 3), ImVec2(c.x - 6 + i * 4.5f, c.y - 1), color);
		draw->AddRectFilled(ImVec2(c.x - 6, c.y + 2), ImVec2(c.x + 6, c.y + 4), color, 1.0f);
		break;
	default: // settings / gear-inspired mark
		draw->AddCircle(c, 6.0f, color, 20, 1.8f);
		draw->AddCircleFilled(c, 2.0f, color, 12);
		for (int i = 0; i < 8; ++i)
		{
			const float a = i * 0.785398163f;
			draw->AddLine(ImVec2(c.x + cosf(a) * 8.0f, c.y + sinf(a) * 8.0f), ImVec2(c.x + cosf(a) * 10.0f, c.y + sinf(a) * 10.0f), color, 2.0f);
		}
		break;
	}
}

static void DrawSTROMCursor()
{
	ImGuiIO& io = ImGui::GetIO();

	if (!io.MouseDrawCursor)
		return;

	if (!io.MousePos.x || !io.MousePos.y)
		return;

	ImDrawList* draw = ImGui::GetForegroundDrawList();

	const ImVec2 p = io.MousePos;

	// Cursor outline
	const ImVec2 outline[] =
	{
		ImVec2(p.x,       p.y),
		ImVec2(p.x,       p.y + 22.0f),
		ImVec2(p.x + 6.0f, p.y + 17.0f),
		ImVec2(p.x + 11.0f, p.y + 27.0f),
		ImVec2(p.x + 15.0f, p.y + 25.0f),
		ImVec2(p.x + 10.0f, p.y + 15.0f),
		ImVec2(p.x + 19.0f, p.y + 15.0f)
	};

	draw->AddPolyline(
		outline,
		7,
		IM_COL32(0, 0, 0, 255),
		true,
		2.0f
	);

	// Cursor fill
	const ImVec2 fill[] =
	{
		ImVec2(p.x + 2.0f,  p.y + 3.0f),
		ImVec2(p.x + 2.0f,  p.y + 18.0f),
		ImVec2(p.x + 7.0f,  p.y + 14.0f),
		ImVec2(p.x + 12.0f, p.y + 24.0f),
		ImVec2(p.x + 14.0f, p.y + 22.0f),
		ImVec2(p.x + 9.0f,  p.y + 12.0f),
		ImVec2(p.x + 16.0f, p.y + 12.0f)
	};

	draw->AddConvexPolyFilled(
		fill,
		7,
		IM_COL32(255, 255, 255, 255)
	);
}

void CMenu::Render()
{
	ImGui::SetNextWindowSize(ImVec2(960, 570));
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 11.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

	if (ImGui::Begin("STROM", &bOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
	{
		const ImVec2 windowPos = ImGui::GetWindowPos();
		const ImVec2 windowSize = ImGui::GetWindowSize();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(ImVec2(windowPos.x + 1, windowPos.y + 1), ImVec2(windowPos.x + 200, windowPos.y + windowSize.y - 1), ImColor(6, 15, 28, 245));

		// Sidebar: the original tab labels and callbacks are retained.
		const char* tabNames[] = { "Weapon", "Player", "Visual", "Style", "Extra", "Developer", "Hotkeys" };
		ImGui::BeginChild("##Sidebar", ImVec2(200, 0), false, ImGuiWindowFlags_NoScrollbar);
		{
			ImGui::Dummy(ImVec2(0, 80));
			ImGui::SetCursorPosX(18);
			ImGui::TextDisabled("NAVIGATION");
			ImGui::Dummy(ImVec2(0, 10));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
			const int visibleTabIds[] = { 0, 1, 2, 3, 5, 6 };
			static float tabHover[6] = {};

			for (int tab = 0; tab < 6; ++tab)
			{
				const int page = visibleTabIds[tab];
				ImGui::SetCursorPosX(50);
				if (iCurrentTab == page) ActiveTab(); else NormalTab();
				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.43f, 0.5f));
				if (ImGui::Button(tabNames[page], ImVec2(140, 40))) iCurrentTab = page;
				ImGui::PopStyleVar();
				const bool hovered = ImGui::IsItemHovered();
				const float targetHover = (iCurrentTab == page) ? 1.0f : (hovered ? 0.55f : 0.0f);
				const float hoverStep = ImGui::GetIO().DeltaTime * 9.0f;
				const float blend = hoverStep < 1.0f ? hoverStep : 1.0f;
				tabHover[tab] += (targetHover - tabHover[tab]) * blend;
				const ImVec2 tabMin = ImGui::GetItemRectMin();
				const ImVec2 tabMax = ImGui::GetItemRectMax();
				const ImU32 iconColor = ImGui::GetColorU32(iCurrentTab == page ? ImVec4(0.30f, 0.77f, 1.0f, 1.0f) : ImVec4(0.48f, 0.60f, 0.73f, 1.0f));
				DrawSidebarIcon(draw, page, ImVec2(tabMin.x + 25.0f, (tabMin.y + tabMax.y) * 0.5f), iconColor);
				if (tabHover[tab] > 0.01f)
				{
					const int alpha = (int)(48.0f * tabHover[tab]);
					draw->AddRect(tabMin, tabMax, ImColor(0, 127, 255, alpha), 8.0f, 0, 1.0f);
				}
				ImGui::PopStyleColor(5);
			}
			ImGui::PopStyleVar();


		}
		ImGui::EndChild();

		draw->AddLine(ImVec2(windowPos.x + 200, windowPos.y + 14), ImVec2(windowPos.x + 200, windowPos.y + windowSize.y - 14), ImColor(0, 127, 255, 65), 1.0f);
		ImGui::SameLine(0, 0);

		ImGui::BeginChild("##Workspace", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
		{
			// Compact header
			ImGui::BeginChild("##Header", ImVec2(0, 100), false, ImGuiWindowFlags_NoScrollbar);
			{
				ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 200, 60));
				ImGui::TextDisabled("ACTIVE SECTION");
				ImGui::SameLine(0, 10);
				ImGui::TextColored(ImVec4(0.28f, 0.72f, 1.0f, 1.0f), "%s", tabNames[iCurrentTab]);
			}
			ImGui::EndChild();

			draw->AddLine(ImVec2(windowPos.x + 218, windowPos.y + 100), ImVec2(windowPos.x + windowSize.x - 14, windowPos.y + 100), ImColor(0, 127, 255, 45), 1.0f);
			ImGui::SetCursorPos(ImVec2(15, 108));
			ImGui::BeginChild("##Content", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
			{
		switch (iCurrentTab)
		{
		case 0:
		{
			ImGui::Spacing();
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 6));
			ImGui::BeginChild("##Weapons", ImVec2(0, 50), true);
			{
				ImGui::PushFont(pIcons);

				if (iCurrentCategory == 0) ActiveButton(); else NormalButton();
				if (ImGui::Button("A", ImVec2(0, 32))) iCurrentCategory = 0;
				ImGui::PopStyleColor(4);
				ImGui::SameLine(90);

				if (iCurrentCategory == 1) ActiveButton(); else NormalButton();
				if (ImGui::Button("H", ImVec2(0, 32))) iCurrentCategory = 1;
				ImGui::PopStyleColor(4);
				ImGui::SameLine(170);

				if (iCurrentCategory == 2) ActiveButton(); else NormalButton();
				if (ImGui::Button("c", ImVec2(0, 32))) iCurrentCategory = 2;
				ImGui::PopStyleColor(4);
				ImGui::SameLine(270);

				if (iCurrentCategory == 3) ActiveButton(); else NormalButton();
				if (ImGui::Button("T", ImVec2(0, 32))) iCurrentCategory = 3;
				ImGui::PopStyleColor(4);
				ImGui::SameLine(370);

				if (iCurrentCategory == 4) ActiveButton(); else NormalButton();
				if (ImGui::Button("Z", ImVec2(0, 32))) iCurrentCategory = 4;
				ImGui::PopStyleColor(4);

				ImGui::PopFont();
				ImGui::PopStyleVar();
				ImGui::EndChild();
			}

			ImGui::BeginGroup();
			ImGui::BeginChild("##Combat", ImVec2(340, 190), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text(xorstr("LOCK")), ImGui::EndMenuBar();
				ImGui::Checkbox(xorstr(xorstr("Range")), &g_Config.g_Aimbot.bAimbot); ImGui::SameLine(230); ImGui::Button("Settings##Range", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text(xorstr(xorstr("Range Settings"))), ImGui::EndMenuBar();
					ImGui::Checkbox("Render Range", &g_Config.g_Aimbot.bDrawRange); ImGui::SameLine(150); ImGui::PushItemWidth(70);
					ImGui::Combo("##RangeStyle", &g_Config.g_Aimbot.iRangeStyle, "Filled\0Outline\0");
					ImGui::Checkbox("Render Tracer", &g_Config.g_Aimbot.bDrawTracer); ImGui::SameLine(150);
					ImGui::SliderFloat("##OutlineThickness", &g_Config.g_Aimbot.fOutlineThickness, 1.0f, 5.0f, "%0.1f"); Tooltip("Thickness"); ImGui::PopItemWidth();
					ImGui::PushItemWidth(161);
					ImGui::ColorEdit4("Range", (float*)&g_Config.g_Aimbot.colorRange, ImGuiColorEditFlags_AlphaBar); ImGui::PopItemWidth();
					ImGui::EndPopup();
				}
				ImGui::Checkbox("Silent##Silent", &g_Config.g_Aimbot.bSilent); ImGui::SameLine(210); ImGui::PushItemWidth(82.f);
				ImGui::SliderFloat("##Accuracy", &g_Config.g_Aimbot.fSilentAccuracy, 20.f, 100.f, "%.0f%%"); Tooltip("Silent Recoil"); ImGui::PopItemWidth();
				ImGui::Checkbox("Smooth##Smooth", &g_Config.g_Aimbot.bSmooth); ImGui::SameLine(230); ImGui::Button("Settings##Smooth", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("Smooth Settings"), ImGui::EndMenuBar();
					ImGui::Checkbox("Aim On Fire", &g_Config.g_Aimbot.bSmoothIsFire);
					ImGui::Checkbox("Stop On Body", &g_Config.g_Aimbot.bStopOnBody);
					ImGui::Checkbox("Lock Y Axis", &g_Config.g_Aimbot.bSmoothLockY);
					ImGui::EndPopup();
				}
				ImGui::Checkbox(xorstr("Pro"), &g_Config.g_Aimbot.bProAim);
				ImGui::Checkbox("Triggerbot", &g_Config.g_Aimbot.bTriggerbot); ImGui::SameLine(230); ImGui::Button("Settings##Trigger", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("Triggerbot Settings"), ImGui::EndMenuBar();
					ImGui::Checkbox("Delay", &g_Config.g_Aimbot.bTriggerDelay); ImGui::SameLine(150); ImGui::PushItemWidth(70);
					ImGui::DragInt("##TriggerDelay", &g_Config.g_Aimbot.iTriggerDelay, 1, 0, 500, "%d ms");
					if (!g_Config.g_Aimbot.bAimbot || !g_Config.g_Aimbot.bSilent)
					{
						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					}
					ImGui::Checkbox("Sync with Silent", &g_Config.g_Aimbot.bTriggerSync); ImGui::SameLine();
					if (!g_Config.g_Aimbot.bAimbot || !g_Config.g_Aimbot.bSilent)
					{
						ImGui::PopItemFlag();
						ImGui::PopStyleVar();
						Tooltip("Enable Range and Silent to use this function!");
					}
					ImGui::EndPopup();
				}
				ImGui::EndChild();
			}

			ImGui::BeginChild("##Filters", ImVec2(340, 190), true, ImGuiWindowFlags_MenuBar);
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
				if (ImGui::BeginMenuBar()) ImGui::Text(xorstr("Filters")), ImGui::EndMenuBar();
				ImGui::Checkbox("Team Protect", &g_Config.g_Aimbot.bTeamProtect);
				ImGui::Checkbox("Lock Through Objects", &g_Config.g_Aimbot.bLockThroughObjects);
				ImGui::Checkbox("Ignore Max Distance", &g_Config.g_Aimbot.bIgnoreMaxDistance);
				ImGui::Checkbox("Ignore AFK", &g_Config.g_Aimbot.bIgnoreAFK);
				ImGui::Checkbox("Ignore Everything", &g_Config.g_Aimbot.bIgnoreEverything);
				ImGui::SameLine(248.0f);
				if (g_Config.g_Aimbot.bIgnoreEverything)
					ImGui::TextColored(ImVec4(0.20f, 0.95f, 0.55f, 1.0f), "ON");
				else
					ImGui::TextColored(ImVec4(0.55f, 0.62f, 0.73f, 1.0f), "OFF");
				ImGui::PopStyleVar();
				ImGui::EndChild();
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::BeginGroup();
			ImGui::BeginChild("##Config", ImVec2(340, 140), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text(szCategory[pMenu->iCurrentCategory]), ImGui::EndMenuBar();

				for (int i = 0; i < _countof(g_Weapons); i++)
				{
					if (g_Weapons[i].iCategory != pMenu->iCurrentCategory)
						continue;

					ImGui::Checkbox(g_Weapons[i].szName.c_str(), &g_Config.g_Aimbot.bAimbotEnabled[i + 22]); ImGui::SameLine(230);
					ImGui::Button(std::string("Config##" + std::to_string(i)).c_str(), ImVec2(70, 0));
					if (ImGui::BeginPopupContextItem(0, 0))
					{
						if (ImGui::BeginMenuBar()) ImGui::Text("%s (%d) Configuration", g_Weapons[i].szName.c_str(), i + 22), ImGui::EndMenuBar();
						g_Config.g_Aimbot.bAimbot ? ImGui::Text("Range") : ImGui::TextDisabled("Range"); ImGui::SameLine(120);
						ImGui::SliderInt("##Range", &g_Config.g_Aimbot.iAimbotConfig[i + 22][RANGE], 1, 200, "%d px");
						if (ImGui::IsItemActive())
							g_Config.g_Aimbot.iRangeStyle ? ImGui::GetOverlayDrawList()->AddCircle(ImVec2(pAimbot->vecCrosshair.fX, pAimbot->vecCrosshair.fY), g_Config.g_Aimbot.iAimbotConfig[i + 22][RANGE] * 1.5f, (ImColor)g_Config.g_Aimbot.colorRange, 64, g_Config.g_Aimbot.fOutlineThickness) :
							ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(pAimbot->vecCrosshair.fX, pAimbot->vecCrosshair.fY), g_Config.g_Aimbot.iAimbotConfig[i + 22][RANGE] * 1.5f, (ImColor)g_Config.g_Aimbot.colorRange, 64);
						g_Config.g_Aimbot.bSilent ? ImGui::Text("Hitchance") : ImGui::TextDisabled("Hitchance"); ImGui::SameLine(120);
						ImGui::SliderInt("##Silent", &g_Config.g_Aimbot.iAimbotConfig[i + 22][SILENT], 1, 100, "%d%%");
						g_Config.g_Aimbot.bSmooth ? ImGui::Text("Smoothness") : ImGui::TextDisabled("Smoothness"); ImGui::SameLine(120);
						ImGui::SliderInt("##Smooth", &g_Config.g_Aimbot.iAimbotConfig[i + 22][SMOOTH], 1, 100, "%d%%");
						ImGui::EndPopup();
					}
				}
				ImGui::EndChild();
			}

			ImGui::BeginChild("##Combat", ImVec2(340, 230), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Combat Assistance"), ImGui::EndMenuBar();
				ImGui::Checkbox("No Reload", &g_Config.g_Combat.bNoReload);
				ImGui::Checkbox("Allow C-BUG", &g_Config.g_Combat.bAllowCBug); ImGui::SameLine(230);ImGui::Button("Settings##AllowCBUG", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("Allow C-BUG Settings"), ImGui::EndMenuBar();
					ImGui::Checkbox("Disable Freeze", &g_Config.g_Combat.bCBugFreeze);
					ImGui::Checkbox("Disable Anim", &g_Config.g_Combat.bCBugAnim);
					ImGui::Checkbox("Disable GameText", &g_Config.g_Combat.bCBugText);
					ImGui::Checkbox("Disable Weapon Remove", &g_Config.g_Combat.bCBugWeapon);
					ImGui::EndPopup();
				}
				ImGui::Checkbox(xorstr("Rapid"), &g_Config.g_Combat.bRapidFire); ImGui::SameLine(88); ImGui::PushItemWidth(255); ImGui::SliderFloat("##RapidFire", &g_Config.g_Combat.fRapidFire, 1.0f, 10.0f, "%0.1f"); ImGui::PopItemWidth();
				ImGui::Checkbox(xorstr("Recoil"), &g_Config.g_Combat.bNoRecoil);
				ImGui::Checkbox("Fast Crosshair", &g_Config.g_Combat.bFastCrosshair);
				ImGui::Checkbox("Weapon Switcher", &g_Config.g_Combat.bWeaponSwitcher); ImGui::SameLine(230); ImGui::Button("Settings##WeaponSwitcher", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("Weapon Switcher Settings"), ImGui::EndMenuBar();
					static int iComboWeapon = 0;
					ImGui::PushItemWidth(140); ImGui::Combo("##WeaponCombo", &iComboWeapon, szCategory, 5); ImGui::PopItemWidth(); ImGui::SameLine();
					ImGui::PushItemWidth(100); ImGui::Hotkey("##WeaponHotkey", &g_Config.g_Hotkeys.iWeaponSwitch[iComboWeapon]); ImGui::PopItemWidth();
					ImGui::Checkbox("Fast Switch", &g_Config.g_Combat.bFastSwitch);
					ImGui::EndPopup();
				}

				ImGui::EndChild();
			}
			ImGui::EndGroup();

			break;
		}
		case 1: {
			ImGui::Spacing();
			ImGui::BeginChild("##Player", ImVec2(340, 450), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Player Assistance"), ImGui::EndMenuBar();
				ImGui::PushItemWidth(70);
				ImGui::Checkbox("Anti Stun", &g_Config.g_Player.bAntiStun); ImGui::SameLine(230); ImGui::SliderInt("##AntiStun", &g_Config.g_Player.iAntiStunChance, 1, 100);
				ImGui::Checkbox("Fast Rotation", &g_Config.g_Player.bFastRotation); ImGui::SameLine(230); ImGui::SliderInt("##FastRotation", &g_Config.g_Player.iFastRotation, 15, 100);
				ImGui::Checkbox("Fast Sprint", &g_Config.g_Player.bFastSprint); ImGui::SameLine(230); ImGui::SliderFloat("##FastSprint", &g_Config.g_Player.fFastSprint, 1.0f, 1.5f, "%0.1f");
				if (ImGui::Checkbox("Air Break", &g_Config.g_Player.bAirBreak))
					g_Config.g_Player.bMapRun = false;
				ImGui::SameLine(230); ImGui::SliderInt("##AirBreak", &g_Config.g_Player.iAirBreak, 1, 100);
				if (ImGui::Checkbox("Map Run", &g_Config.g_Player.bMapRun))
					g_Config.g_Player.bAirBreak = false;
				ImGui::SameLine(230); ImGui::SliderInt("##MapRun", &g_Config.g_Player.iMapRun, 1, 100);
				ImGui::Checkbox("Infinite Energy", &g_Config.g_Player.bInfiniteRun);
				ImGui::Checkbox("Infinite Oxygen", &g_Config.g_Player.bInfiniteOxygen);
				ImGui::Checkbox("Fast Respawn", &g_Config.g_Player.bFastRespawn);
				ImGui::Checkbox("No Fall", &g_Config.g_Player.bNoFall);
				ImGui::Checkbox("Stop On Exit Veh", &g_Config.g_Player.bStopOnExitVehicle); ImGui::SameLine(230); ImGui::DragInt("##StopOnExit", &g_Config.g_Player.iStopOnExitVehicle, 1, 500, 1500, "%d ms");
				ImGui::Checkbox("Auto Bike Spam", &g_Config.g_Player.bAutoBikeSpam); ImGui::SameLine(230); ImGui::Button("Settings##AutoBikeSpam", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("Auto Bike Spam Settings"), ImGui::EndMenuBar();
					ImGui::Text("Hold Key"); ImGui::SameLine(120); ImGui::PushItemWidth(100); ImGui::Hotkey("##BikeHotkey", &g_Config.g_Hotkeys.iAutoBikeSpam);
					ImGui::Checkbox("Motorbike", &g_Config.g_Player.bMotorBikeSpam); ImGui::SameLine(120); ImGui::Hotkey("##MotorbikeHotkey", &g_Config.g_Hotkeys.iMotorBikeSpam); ImGui::PopItemWidth();
					ImGui::EndPopup();
				}
				ImGui::Checkbox("Change Skin", &g_Config.g_Player.bChangeSkin); ImGui::SameLine(230); 
				ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(1, 0)); ImGui::InputInt("##SkinID", &g_Config.g_Player.iSkinID); ImGui::PopStyleVar();
				ImGui::PopItemWidth();
				ImGui::EndChild();
			}
			ImGui::SameLine();
			ImGui::BeginChild("##Sync", ImVec2(340, 190), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Sync Modifications"), ImGui::EndMenuBar();
				ImGui::Checkbox("Upside Mode", &g_Config.g_Player.bUpsideMode);
				ImGui::Checkbox("Invert Walk", &g_Config.g_Player.bInvertWalk);
				ImGui::Checkbox("Shake Mode", &g_Config.g_Player.bShakeMode);
				ImGui::Checkbox("Invalid Aim Z", &g_Config.g_Player.bInvalidAimZ);
				ImGui::Checkbox("Invisible", &g_Config.g_Player.bInvisible);
				ImGui::PushItemWidth(70);
				ImGui::Checkbox("Fake Lag", &g_Config.g_Player.bFakeLag); ImGui::SameLine(230); ImGui::DragInt("##FakeLag", &g_Config.g_Player.iFakeLag, 1, 1, 500, "%d ms");
				ImGui::PopItemWidth();
				ImGui::EndChild();
			}
			break;
		}
		case 2: {
			ImGui::Spacing();
			ImGui::BeginGroup();
			ImGui::BeginChild("##PlayerESP", ImVec2(340, 230), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Player ESP"), ImGui::EndMenuBar();
				ImGui::Checkbox("ESP Name", &g_Config.g_Visuals.bNameTagsESP);
				ImGui::Checkbox("ESP Box", &g_Config.g_Visuals.bBoxESP); ImGui::SameLine(230);
				ImGui::PushItemWidth(50.f);
				ImGui::Combo("##RangeStyle", &g_Config.g_Visuals.iBoxStyle, "2D\0" "3D\0"); ImGui::PopItemWidth(); ImGui::SameLine(0.0f, 1.0f);
				ImGui::Checkbox("##BoxFilled", &g_Config.g_Visuals.bFilledBox); Tooltip("Filled");
				ImGui::Checkbox("ESP Chams", &g_Config.g_Visuals.bChamsESP);
				ImGui::Checkbox("ESP Snipeline", &g_Config.g_Visuals.bSnipelineESP); ImGui::SameLine(230);
				ImGui::PushItemWidth(70.f);
				ImGui::Combo("##SnipelineStyle", &g_Config.g_Visuals.iSnipelineStyle, "Above\0Down\0");
				ImGui::PopItemWidth();
				ImGui::Checkbox("ESP Bones", &g_Config.g_Visuals.bBoneESP);
				ImGui::Checkbox("ESP Weapon", &g_Config.g_Visuals.bWeaponESP);
				ImGui::Checkbox("ESP Distance", &g_Config.g_Visuals.bDistanceESP);
				ImGui::EndChild();
			}
			ImGui::BeginChild("##ESPSettings", ImVec2(340, 195), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("ESP Settings"), ImGui::EndMenuBar();
				ImGui::Text("ESP Thickness"); ImGui::SameLine(230); ImGui::PushItemWidth(70.f);
				ImGui::SliderFloat("##Thickness", &g_Config.g_Visuals.fESPThickness, 1.0f, 3.0f, "%0.1f");
				ImGui::PopItemWidth();
				ImGui::Checkbox("Disable for Teammates", &g_Config.g_Visuals.bESPTeammates);
				ImGui::Checkbox("Use Custom Colors", &g_Config.g_Visuals.bESPCustomColors);
				ImGui::PushItemWidth(160.f);
				ImGui::ColorEdit4("Visible", (float*)&g_Config.g_Visuals.colorVisible, ImGuiColorEditFlags_AlphaBar);
				ImGui::ColorEdit4("Not Visible", (float*)&g_Config.g_Visuals.colorNotVisible, ImGuiColorEditFlags_AlphaBar);
				ImGui::PopItemWidth();
				ImGui::EndChild();
			}
			ImGui::EndGroup();
			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::BeginChild("##VehicleESP", ImVec2(340, 190), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Vehicle ESP"), ImGui::EndMenuBar();
				ImGui::Checkbox("ESP Name ##Vehicle", &g_Config.g_Visuals.bVehicleNameTagsESP);
				ImGui::Checkbox("ESP Health", &g_Config.g_Visuals.bVehicleHPESP);
				ImGui::Checkbox("ESP Engine", &g_Config.g_Visuals.bVehicleEngineESP);
				ImGui::Checkbox("ESP Status", &g_Config.g_Visuals.bVehicleStatusESP);
				ImGui::Checkbox("ESP Distance", &g_Config.g_Visuals.bVehicleDistanceESP);
				ImGui::EndChild();
			}
			ImGui::BeginChild("##Visuals", ImVec2(340, 245), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text(xorstr("Visuals")), ImGui::EndMenuBar();
				ImGui::Checkbox("Aspect Ratio", &g_Config.g_Visuals.bAspectRatio); ImGui::SameLine(230); ImGui::PushItemWidth(70); ImGui::SliderFloat("##AspectRatio", &g_Config.g_Visuals.fAspectRatio, 0.5f, 2.0f, "%0.1f"); ImGui::PopItemWidth();
				ImGui::Checkbox("Damage Informer", &g_Config.g_Visuals.bDMGInformer); ImGui::SameLine(230); ImGui::Button("Settings##DMG", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("Damage Informer Settings"), ImGui::EndMenuBar();
					ImGui::Text("Font Size"); ImGui::SameLine(150); ImGui::PushItemWidth(70);
					ImGui::SliderInt("##DMGFont", &g_Config.g_Visuals.iFontSize, 7, 20, "%d px");
					ImGui::Text("Time on screen"); ImGui::SameLine(150); ImGui::PushItemWidth(70);
					ImGui::DragFloat("##DMGTick", &g_Config.g_Visuals.fDMGTick, 0.1f, 0.1f, 10.0f, "%.1f s");
					ImGui::PushItemWidth(161);
					ImGui::ColorEdit4("Text", (float*)&g_Config.g_Visuals.colorDMG, ImGuiColorEditFlags_AlphaBar);
					ImGui::PopItemWidth();
					ImGui::EndPopup();
				}
				ImGui::Checkbox("FPS Funcs", &g_Config.g_Visuals.bFPSFuncs); ImGui::SameLine(230);
				ImGui::Button("Settings##FPSFuncs", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("FPS Funcs Settings"), ImGui::EndMenuBar();
					ImGui::PushItemWidth(40.f);
					ImGui::Checkbox("FPS Unlock", &g_Config.g_Visuals.bFPSUnlock);
					ImGui::Checkbox("FPS Boost", &g_Config.g_Visuals.bFPSBoost);
					ImGui::Checkbox("Fake FPS", &g_Config.g_Visuals.bFakeFPS); ImGui::SameLine(135); 
					ImGui::InputInt("##MinFPS", &g_Config.g_Visuals.iMinFPS, 0); Tooltip("Min FPS"); ImGui::SameLine(177); ImGui::InputInt("##MaxFPS", &g_Config.g_Visuals.iMaxFPS, 0); Tooltip("Max FPS");
					if (g_Config.g_Visuals.iMinFPS > g_Config.g_Visuals.iMaxFPS)
						g_Config.g_Visuals.iMaxFPS = g_Config.g_Visuals.iMinFPS;
					ImGui::PopItemWidth();
					ImGui::End();
				}
				ImGui::Checkbox("Weather Changer", &g_Config.g_Visuals.bWeatherChanger); ImGui::SameLine(230); ImGui::Button("Settings##Weather", ImVec2(70, 0));
				if (ImGui::BeginPopupContextItem(0, 0))
				{
					if (ImGui::BeginMenuBar()) ImGui::Text("Weather Changer"), ImGui::EndMenuBar();
					ImGui::Text("Weather"); ImGui::SameLine(100); ImGui::SameLine(150); ImGui::PushItemWidth(70); ImGui::SliderInt("##Weather", &g_Config.g_Visuals.iWeather, 0, 20); 
					ImGui::Checkbox("Time Changer", &g_Config.g_Visuals.bTimeChanger); ImGui::SameLine(150); ImGui::SliderInt("##Time", &g_Config.g_Visuals.iTime, 0, 24); ImGui::PopItemWidth();
					ImGui::EndPopup();
				}
				ImGui::Checkbox("No Drugs Effect", &g_Config.g_Visuals.bNoDrugsEffect);
				ImGui::EndChild();
			}
			ImGui::EndGroup();
			break;
		}
		case 3: {
			ImGui::Spacing();
			ImGui::BeginGroup();
			ImGui::BeginChild("##Theme", ImVec2(340, 450), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Theme"), ImGui::EndMenuBar();

				if (ImGui::Button("Set Default Theme", ImVec2(259, 23)))
					pMenu->Theme();

				for (int i = 0; i < ImGuiCol_COUNT; i++)
				{
					ImGui::PushID(i);
					ImGui::Text(ImGui::GetStyleColorName(i));
					ImGui::SameLine(176);
					ImGui::PushItemWidth(90);
					ImGui::ColorEdit4("##Color", (float*)&ImGui::GetStyle().Colors[i], ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_AlphaBar);
					ImGui::PopItemWidth();
					if (memcmp(&ImGui::GetStyle().Colors[i], &g_Config.g_Style.vecTheme[i], sizeof(ImVec4)) != 0)
						g_Config.g_Style.vecTheme[i] = ImGui::GetStyle().Colors[i];
					ImGui::PopID();
				}

				ImGui::EndChild();
				ImGui::EndGroup();
			}
			ImGui::SameLine();
			ImGui::BeginChild("##Sizes", ImVec2(340, 450), true, ImGuiWindowFlags_MenuBar);
			{
				ImGui::EndChild();
			}
			break;
		}
		case 5: {
			ImGui::Spacing();
			ImGui::BeginGroup();
			ImGui::BeginChild("##Damager", ImVec2(340, 220), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Damager"), ImGui::EndMenuBar();
				ImGui::PushItemWidth(70.f);
				ImGui::Checkbox("Damager", &g_Config.g_Developer.bDamager);
				ImGui::Checkbox("Delay", &g_Config.g_Developer.bDelay); ImGui::SameLine(230);
				ImGui::InputInt("##Delay", &g_Config.g_Developer.iDelay);
				ImGui::Checkbox("Team Protect", &g_Config.g_Developer.bTeamProtect);
				ImGui::Checkbox("Send Bullet Data", &g_Config.g_Developer.bSendBulletData);
				ImGui::Checkbox("Teleport To Player", &g_Config.g_Developer.bTeleportToPlayer);
				ImGui::Checkbox("Custom Weapon", &g_Config.g_Developer.bCustomWeapon); ImGui::SameLine(230);
				ImGui::InputInt("##WeaponID", &g_Config.g_Developer.iWeaponID);
				ImGui::Checkbox("Custom Damage", &g_Config.g_Developer.bCustomDamage); ImGui::SameLine(230);
				ImGui::InputFloat("##Damage", &g_Config.g_Developer.fDamage);
				ImGui::PopItemWidth();
				ImGui::EndChild();

			}
			ImGui::BeginChild("##SAMPCAC", ImVec2(340, 190), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("SAMPCAC"), ImGui::EndMenuBar();

				ImGui::EndChild();
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::BeginChild("##Developer", ImVec2(340, 450), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Developer"), ImGui::EndMenuBar();

				if (ImGui::Button("Unload", ImVec2(248, 30)))
				{
					Cheat::Unload();
				}

				ImGui::EndChild();
			}
			break;
		}
		case 6: {
			ImGui::Spacing();
			ImGui::BeginGroup();
			ImGui::BeginChild("##HotkeysCombat", ImVec2(340, 290), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Combat Toggles"), ImGui::EndMenuBar();
				DrawToggleBinds(0);
				ImGui::EndChild();
			}
			ImGui::BeginChild("##HotkeysVisuals", ImVec2(340, 130), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Visual Toggles"), ImGui::EndMenuBar();
				DrawToggleBinds(1);
				ImGui::EndChild();
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::BeginChild("##HotkeysConfig", ImVec2(340, 428), true, ImGuiWindowFlags_MenuBar);
			{
				if (ImGui::BeginMenuBar()) ImGui::Text("Hotkeys & Config"), ImGui::EndMenuBar();

				ImGui::Checkbox("Chat Notification", &g_Config.g_Hotkeys.bToggleNotify);
				ImGui::Spacing();

				if (ImGui::Button("Clear All Hotkeys", ImVec2(300, 28)))
				{
					for (int i = 0; i < g_iHotkeyBindCount; i++)
						*g_HotkeyBinds[i].pKey = 0;
				}
				if (ImGui::Button("Save Config Now", ImVec2(300, 28)))
					ConfigFile::Save();
				if (ImGui::Button("Reload Config", ImVec2(300, 28)))
				{
					ConfigFile::Load();
					for (int i = 0; i < ImGuiCol_COUNT; i++)
						ImGui::GetStyle().Colors[i] = g_Config.g_Style.vecTheme[i];
				}

				ImGui::Separator();
				ImGui::Spacing();

				const int iSaved = ConfigFile::SecondsSinceSave();
				ImGui::TextColored(ImVec4(0.28f, 0.72f, 1.0f, 1.0f), "Auto-save: ON");
				if (iSaved >= 0)
					ImGui::Text("Last saved %ds ago", iSaved);
				else
					ImGui::Text("Not saved yet");

				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Text, g_Config.g_Style.vecTheme[ImGuiCol_TextDisabled]);
				ImGui::TextWrapped("%s", ConfigFile::GetPath());
				ImGui::Spacing();
				ImGui::TextWrapped("Click a key box and press the key you want. ESC clears it. END is reserved for the menu.");
				ImGui::PopStyleColor();

				ImGui::EndChild();
			}
			break;
		}
		}

			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
		draw->AddRect(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), ImColor(0, 127, 255, 135), 11.0f, 0, 1.0f);
		if (pTextures && pTextures->tFrame)
		{
			ImDrawList* foreground = ImGui::GetForegroundDrawList();
			// Keep the source aspect ratio and grow the artwork outside the menu
			// bounds so its transparent center frames the UI without covering it.
			const float frameWidth = windowSize.x + 82.0f;
			const float frameHeight = windowSize.y + 140.0f;
			const ImVec2 frameMin(windowPos.x - 41.0f, windowPos.y - 104.0f);
			const ImVec2 frameMax(frameMin.x + frameWidth, frameMin.y + frameHeight);
			foreground->AddImage((ImTextureID)pTextures->tFrame, frameMin, frameMax);
		}
		ImGui::End();
	}
	ImGui::PopStyleVar(4);

	DrawSTROMCursor();
}
