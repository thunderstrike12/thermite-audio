#pragma once

#include <imgui.h>

namespace tmt::theme {

// Main colors - very dark theme
constexpr ImVec4 BG_DARK = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);           // #191919 - darkest background
constexpr ImVec4 BG_WINDOW = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);         // #1e1e1e - window background
constexpr ImVec4 BG_CHILD = ImVec4(0.129f, 0.129f, 0.129f, 1.0f);          // #212121 - child window background
constexpr ImVec4 BG_POPUP = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);          // #252525 - popup background
constexpr ImVec4 BG_FRAME = ImVec4(0.161f, 0.161f, 0.161f, 1.0f);          // #292929 - frame background
constexpr ImVec4 BG_FRAME_HOVERED = ImVec4(0.200f, 0.200f, 0.200f, 1.0f);  // #333333
constexpr ImVec4 BG_FRAME_ACTIVE = ImVec4(0.239f, 0.239f, 0.239f, 1.0f);   // #3d3d3d

// Border colors
constexpr ImVec4 BORDER_COLOR = ImVec4(0.216f, 0.216f, 0.216f, 1.0f);  // #373737 - subtle borders
constexpr ImVec4 BORDER_SHADOW = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

// Text colors
constexpr ImVec4 TEXT_COLOR = ImVec4(0.847f, 0.847f, 0.847f, 1.0f);     // #d8d8d8 - main text
constexpr ImVec4 TEXT_DISABLED = ImVec4(0.500f, 0.500f, 0.500f, 1.0f);  // #808080 - disabled text

// Header colors (collapsing headers, tree nodes)
constexpr ImVec4 HEADER_COLOR = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);      // #3d3d3d
constexpr ImVec4 HEADER_HOVERED = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);    // #3d3d3d
constexpr ImVec4 HEADER_ACTIVE = ImVec4(0.278f, 0.278f, 0.278f, 1.0f);  // #474747

// Button colors
constexpr ImVec4 BUTTON_COLOR = ImVec4(0.200f, 0.200f, 0.200f, 1.0f);    // #333333
constexpr ImVec4 BUTTON_HOVERED = ImVec4(0.278f, 0.278f, 0.278f, 1.0f);  // #474747
constexpr ImVec4 BUTTON_ACTIVE = ImVec4(0.318f, 0.318f, 0.318f, 1.0f);   // #515151

// Tab colors
constexpr ImVec4 TAB_COLOR = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);             // #252525
constexpr ImVec4 TAB_HOVERED = ImVec4(0.278f, 0.278f, 0.278f, 1.0f);           // #474747
constexpr ImVec4 TAB_ACTIVE = ImVec4(0.200f, 0.200f, 0.200f, 1.0f);            // #333333
constexpr ImVec4 TAB_UNFOCUSED = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);         // #1e1e1e
constexpr ImVec4 TAB_UNFOCUSED_ACTIVE = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);  // #2d2d2d

// Title bar colors
constexpr ImVec4 TITLE_BG = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);            // #191919
constexpr ImVec4 TITLE_BG_ACTIVE = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);     // #1e1e1e
constexpr ImVec4 TITLE_BG_COLLAPSED = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);  // #191919

// Scrollbar colors
constexpr ImVec4 SCROLLBAR_BG = ImVec4(0.098f, 0.098f, 0.098f, 0.5f);
constexpr ImVec4 SCROLLBAR_GRAB = ImVec4(0.318f, 0.318f, 0.318f, 1.0f);
constexpr ImVec4 SCROLLBAR_GRAB_HOVERED = ImVec4(0.400f, 0.400f, 0.400f, 1.0f);
constexpr ImVec4 SCROLLBAR_GRAB_ACTIVE = ImVec4(0.478f, 0.478f, 0.478f, 1.0f);

// Slider/Checkbox/Progress colors
constexpr ImVec4 SLIDER_GRAB = ImVec4(0.478f, 0.478f, 0.478f, 1.0f);
constexpr ImVec4 SLIDER_GRAB_ACTIVE = ImVec4(0.600f, 0.600f, 0.600f, 1.0f);
constexpr ImVec4 CHECK_MARK = ImVec4(0.847f, 0.847f, 0.847f, 1.0f);

// Separator
constexpr ImVec4 SEPARATOR_COLOR = ImVec4(0.216f, 0.216f, 0.216f, 1.0f);
constexpr ImVec4 SEPARATOR_HOVERED = ImVec4(0.400f, 0.400f, 0.400f, 1.0f);
constexpr ImVec4 SEPARATOR_ACTIVE = ImVec4(0.478f, 0.478f, 0.478f, 1.0f);

// Resize grip
constexpr ImVec4 RESIZE_GRIP = ImVec4(0.278f, 0.278f, 0.278f, 0.2f);
constexpr ImVec4 RESIZE_GRIP_HOVERED = ImVec4(0.400f, 0.400f, 0.400f, 0.67f);
constexpr ImVec4 RESIZE_GRIP_ACTIVE = ImVec4(0.478f, 0.478f, 0.478f, 0.95f);

// Docking
constexpr ImVec4 DOCKING_PREVIEW = ImVec4(0.278f, 0.478f, 0.678f, 0.7f);
constexpr ImVec4 DOCKING_EMPTY_BG = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);

// Menu bar
constexpr ImVec4 MENUBAR_BG = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);

// Table colors
constexpr ImVec4 TABLE_HEADER_BG = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);
constexpr ImVec4 TABLE_BORDER_STRONG = ImVec4(0.216f, 0.216f, 0.216f, 1.0f);
constexpr ImVec4 TABLE_BORDER_LIGHT = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);
constexpr ImVec4 TABLE_ROW_BG = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
constexpr ImVec4 TABLE_ROW_BG_ALT = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

// Nav highlight
constexpr ImVec4 NAV_HIGHLIGHT = ImVec4(0.278f, 0.478f, 0.678f, 1.0f);
constexpr ImVec4 NAV_WINDOWING_HIGHLIGHT = ImVec4(1.0f, 1.0f, 1.0f, 0.70f);
constexpr ImVec4 NAV_WINDOWING_DIM_BG = ImVec4(0.8f, 0.8f, 0.8f, 0.20f);

// Modal dim
constexpr ImVec4 MODAL_DIM = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

inline void apply_dark_theme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Apply colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = TEXT_COLOR;
    colors[ImGuiCol_TextDisabled] = TEXT_DISABLED;
    colors[ImGuiCol_WindowBg] = BG_WINDOW;
    colors[ImGuiCol_ChildBg] = BG_CHILD;
    colors[ImGuiCol_PopupBg] = BG_POPUP;
    colors[ImGuiCol_Border] = BORDER_COLOR;
    colors[ImGuiCol_BorderShadow] = BORDER_SHADOW;
    colors[ImGuiCol_FrameBg] = BG_FRAME;
    colors[ImGuiCol_FrameBgHovered] = BG_FRAME_HOVERED;
    colors[ImGuiCol_FrameBgActive] = BG_FRAME_ACTIVE;
    colors[ImGuiCol_TitleBg] = TITLE_BG;
    colors[ImGuiCol_TitleBgActive] = TITLE_BG_ACTIVE;
    colors[ImGuiCol_TitleBgCollapsed] = TITLE_BG_COLLAPSED;
    colors[ImGuiCol_MenuBarBg] = MENUBAR_BG;
    colors[ImGuiCol_ScrollbarBg] = SCROLLBAR_BG;
    colors[ImGuiCol_ScrollbarGrab] = SCROLLBAR_GRAB;
    colors[ImGuiCol_ScrollbarGrabHovered] = SCROLLBAR_GRAB_HOVERED;
    colors[ImGuiCol_ScrollbarGrabActive] = SCROLLBAR_GRAB_ACTIVE;
    colors[ImGuiCol_CheckMark] = CHECK_MARK;
    colors[ImGuiCol_SliderGrab] = SLIDER_GRAB;
    colors[ImGuiCol_SliderGrabActive] = SLIDER_GRAB_ACTIVE;
    colors[ImGuiCol_Button] = BUTTON_COLOR;
    colors[ImGuiCol_ButtonHovered] = BUTTON_HOVERED;
    colors[ImGuiCol_ButtonActive] = BUTTON_ACTIVE;
    colors[ImGuiCol_Header] = HEADER_COLOR;
    colors[ImGuiCol_HeaderHovered] = HEADER_HOVERED;
    colors[ImGuiCol_HeaderActive] = HEADER_ACTIVE;
    colors[ImGuiCol_Separator] = SEPARATOR_COLOR;
    colors[ImGuiCol_SeparatorHovered] = SEPARATOR_HOVERED;
    colors[ImGuiCol_SeparatorActive] = SEPARATOR_ACTIVE;
    colors[ImGuiCol_ResizeGrip] = RESIZE_GRIP;
    colors[ImGuiCol_ResizeGripHovered] = RESIZE_GRIP_HOVERED;
    colors[ImGuiCol_ResizeGripActive] = RESIZE_GRIP_ACTIVE;
    colors[ImGuiCol_Tab] = TAB_COLOR;
    colors[ImGuiCol_TabHovered] = TAB_HOVERED;
    colors[ImGuiCol_TabSelected] = TAB_ACTIVE;
    colors[ImGuiCol_TabDimmed] = TAB_UNFOCUSED;
    colors[ImGuiCol_TabDimmedSelected] = TAB_UNFOCUSED_ACTIVE;
    colors[ImGuiCol_DockingPreview] = DOCKING_PREVIEW;
    colors[ImGuiCol_DockingEmptyBg] = DOCKING_EMPTY_BG;
    colors[ImGuiCol_PlotLines] = ImVec4(0.610f, 0.610f, 0.610f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.430f, 0.350f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.900f, 0.700f, 0.0f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.600f, 0.0f, 1.0f);
    colors[ImGuiCol_TableHeaderBg] = TABLE_HEADER_BG;
    colors[ImGuiCol_TableBorderStrong] = TABLE_BORDER_STRONG;
    colors[ImGuiCol_TableBorderLight] = TABLE_BORDER_LIGHT;
    colors[ImGuiCol_TableRowBg] = TABLE_ROW_BG;
    colors[ImGuiCol_TableRowBgAlt] = TABLE_ROW_BG_ALT;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.260f, 0.590f, 0.980f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.90f);
    colors[ImGuiCol_NavHighlight] = NAV_HIGHLIGHT;
    colors[ImGuiCol_NavWindowingHighlight] = NAV_WINDOWING_HIGHLIGHT;
    colors[ImGuiCol_NavWindowingDimBg] = NAV_WINDOWING_DIM_BG;
    colors[ImGuiCol_ModalWindowDimBg] = MODAL_DIM;

    // Style settings
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;

    // Border sizes
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.TabBarBorderSize = 1.0f;

    // Rounding
    // Note: WindowRounding is set to 0 because docked windows should have sharp corners.
    // Undocked/floating windows will automatically get rounding from the OS or can be styled separately.
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;

    // Alignment
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    // Safe area
    style.DisplaySafeAreaPadding = ImVec2(3.0f, 3.0f);
}

}  // namespace tmt::theme
