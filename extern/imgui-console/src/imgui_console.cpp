// Copyright (c) 2020 - present, Roland Munguia
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

// #pragma once

#include <string>
#include "imgui_console.h"
#include "imgui_internal.h"
#include <cstring>
#include <misc/cpp/imgui_stdlib.h>
// fonts icons

#define ICON_FA_TRIANGLE_EXCLAMATION "\xef\x81\xb1"
#define ICON_FA_CIRCLE_XMARK "\xef\x81\x97"
#define ICON_FA_CIRCLE_INFO "\xef\x81\x9a"

ImGuiConsole::ImGuiConsole(std::string c_name, size_t inputBufferSize) : m_ConsoleName(std::move(c_name)) {
    // Set input buffer size.
    m_Buffer.resize(inputBufferSize);
    m_HistoryIndex = std::numeric_limits<size_t>::min();

    // Specify custom data to be store/loaded from imgui.ini
    InitIniSettings();

    // Set Console ImGui default settings
    if (!m_LoadedFromIni) {
        DefaultSettings();
    }

    // Custom functions.
    RegisterConsoleCommands();
}

void ImGuiConsole::Draw() {
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_WindowAlpha);
    if (!ImGui::Begin(m_ConsoleName.data(), nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::PopStyleVar();
        ImGui::End();
        return;
    }
    ImGui::PopStyleVar();

    DrawContent();

    ImGui::End();
}

void ImGuiConsole::DrawContent() {
    MenuBar();

    if (m_FilterBar) {
        FilterBar();
    }

    LogWindow();
    ImGui::Separator();
    InputBar();
}

csys::System& ImGuiConsole::System() { return m_ConsoleSystem; }

void ImGuiConsole::InitIniSettings() {
    ImGuiContext& g = *ImGui::GetCurrentContext();

    // Load from .ini
    if (g.Initialized && !g.SettingsLoaded && !m_LoadedFromIni) {
        ImGuiSettingsHandler console_ini_handler;
        console_ini_handler.TypeName = "imgui-console";
        console_ini_handler.TypeHash = ImHashStr("imgui-console");
        console_ini_handler.ClearAllFn = SettingsHandler_ClearALl;
        console_ini_handler.ApplyAllFn = SettingsHandler_ApplyAll;
        console_ini_handler.ReadInitFn = SettingsHandler_ReadInit;
        console_ini_handler.ReadOpenFn = SettingsHandler_ReadOpen;
        console_ini_handler.ReadLineFn = SettingsHandler_ReadLine;
        console_ini_handler.WriteAllFn = SettingsHandler_WriteAll;
        console_ini_handler.UserData = this;
        g.SettingsHandlers.push_back(console_ini_handler);
    }
    // else Ini settings already loaded!
}

void ImGuiConsole::DefaultSettings() {
    // Settings
    m_AutoScroll = true;
    m_ScrollToBottom = false;
    m_ColoredOutput = true;
    m_FilterBar = true;
    m_TimeStamps = true;

    m_ShowCommand = true;
    m_ShowLog = true;
    m_ShowWarning = true;
    m_ShowError = true;
    m_ShowInfo = true;

    // Style
    m_WindowAlpha = 1;
    m_ColorPalette[COL_COMMAND] = ImVec4(1.f, 1.f, 1.f, 1.f);
    m_ColorPalette[COL_LOG] = ImVec4(1.f, 1.f, 1.f, 0.5f);
    m_ColorPalette[COL_WARNING] = ImVec4(1.0f, 0.87f, 0.37f, 1.f);
    m_ColorPalette[COL_ERROR] = ImVec4(1.f, 0.365f, 0.365f, 1.f);
    m_ColorPalette[COL_INFO] = ImVec4(0.46f, 0.96f, 0.46f, 1.f);
    m_ColorPalette[COL_TIMESTAMP] = ImVec4(1.f, 1.f, 1.f, 0.5f);
}

void ImGuiConsole::RegisterConsoleCommands() {
    m_ConsoleSystem.RegisterCommand("clear", "Clear console log", [this]() { m_ConsoleSystem.Items().clear(); });

    m_ConsoleSystem.RegisterCommand(
        "filter", "Set screen filter",
        [this](const csys::String& filter) {
            // Reset filter buffer.
            std::memset(m_TextFilter.InputBuf, '\0', 256);

            // Copy filter input buffer from client.
            std::copy(filter.m_String.c_str(), filter.m_String.c_str() + std::min(static_cast<int>(filter.m_String.length()), 255), m_TextFilter.InputBuf);

            // Build text filter.
            m_TextFilter.Build();
        },
        csys::Arg<csys::String>("filter_str")
    );

    m_ConsoleSystem.RegisterCommand(
        "run", "Run given script",
        [this](const csys::String& filter) {
            // Logs command.
            m_ConsoleSystem.RunScript(filter.m_String);
        },
        csys::Arg<csys::String>("script_name")
    );
}

void ImGuiConsole::FilterBar() {
    DrawLogTypeButtons();

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    m_TextFilter.Draw("Filter", ImGui::GetWindowWidth() * 0.25f);
    ImGui::Separator();
}

void ImGuiConsole::DrawLogTypeButtons() {
    auto ToggleButton = [this](const char* icon, const char* tooltip, bool* enabled, ImVec4 color, int count) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

        if (*enabled) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x, color.y, color.z, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        char buf[64];
        snprintf(buf, sizeof(buf), "%s %d", icon, count);
        if (ImGui::Button(buf)) {
            *enabled = !*enabled;
        }

        // Tooltip on hover
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s (%d)", tooltip, count);
            ImGui::EndTooltip();
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
    };

    // Count each type
    int cmdCount = 0, logCount = 0, warnCount = 0, errCount = 0, infoCount = 0;
    for (const auto& item : m_ConsoleSystem.Items()) {
        switch (item.m_Type) {
            case csys::COMMAND:
                cmdCount++;
                break;
            case csys::LOG:
                logCount++;
                break;
            case csys::WARNING:
                warnCount++;
                break;
            case csys::ERROR:
                errCount++;
                break;
            case csys::INFO:
                infoCount++;
                break;
        }
    }

    ToggleButton(ICON_FA_CIRCLE_INFO, "Info", &m_ShowInfo, m_ColorPalette[COL_INFO], infoCount);
    ImGui::SameLine();
    ToggleButton(ICON_FA_TRIANGLE_EXCLAMATION, "Warnings", &m_ShowWarning, m_ColorPalette[COL_WARNING], warnCount);
    ImGui::SameLine();
    ToggleButton(ICON_FA_CIRCLE_XMARK, "Errors", &m_ShowError, m_ColorPalette[COL_ERROR], errCount);
}
void ImGuiConsole::LogWindow() {
    const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollRegion##", ImVec2(0, -footerHeightToReserve), false, 0)) {
        // Display colored command output.
        static const float timestamp_width = ImGui::CalcTextSize("00:00:00:0000").x;  // Timestamp.
        int count = 0;                                                                // Item count.
        int itemIndex = 0;                                                            // Selection index.

        ImVec2 mousePos = ImGui::GetMousePos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Track content bounds.
        float contentStartY = ImGui::GetCursorScreenPos().y;
        float contentEndY = contentStartY;

        // Wrap items.
        ImGui::PushTextWrapPos();

        std::string selectedText;

        auto PassesTypeFilter = [this](csys::ItemType type) {
            switch (type) {
                case csys::INFO:
                    return m_ShowInfo;
                case csys::WARNING:
                    return m_ShowWarning;
                case csys::ERROR:
                    return m_ShowError;
                default:
                    return true;
            }
        };
        // First pass: calculate content bounds.
        for (const auto& item : m_ConsoleSystem.Items()) {
            if (!PassesTypeFilter(item.m_Type) || !m_TextFilter.PassFilter(item.Get().c_str())) continue;
            ImVec2 textSize = ImGui::CalcTextSize(item.Get().data());
            contentEndY += textSize.y + ImGui::GetStyle().ItemSpacing.y;
            if (item.m_Type == csys::COMMAND && count++ != 0) {
                contentEndY += ImGui::GetFontSize();  // Account for spacing between commands.
            }
        }
        count = 0;  // Reset count for actual rendering.

        // Handle selection input.
        if (ImGui::IsWindowHovered()) {
            if (ImGui::IsMouseClicked(0)) {
                // Only start selection if clicking within content bounds.
                if (mousePos.y >= contentStartY && mousePos.y <= contentEndY) {
                    m_IsSelecting = true;
                    m_SelectionStart = mousePos;
                    m_SelectionEnd = mousePos;
                    m_SelectionStartIndex = -1;
                    m_SelectionEndIndex = -1;
                } else {
                    // Clear selection if clicking outside content.
                    m_SelectionStartIndex = -1;
                    m_SelectionEndIndex = -1;
                }
            }
        }

        if (m_IsSelecting) {
            if (ImGui::IsMouseDown(0)) {
                m_SelectionEnd = mousePos;
                // Clamp selection end to content bounds.
                m_SelectionEnd.y = std::max(contentStartY, std::min(m_SelectionEnd.y, contentEndY));
            } else {
                m_IsSelecting = false;
            }
        }

        // Calculate selection bounds (ensure start < end).
        ImVec2 selMin(std::min(m_SelectionStart.x, m_SelectionEnd.x), std::min(m_SelectionStart.y, m_SelectionEnd.y));
        ImVec2 selMax(std::max(m_SelectionStart.x, m_SelectionEnd.x), std::max(m_SelectionStart.y, m_SelectionEnd.y));

        // Display items.
        for (const auto& item : m_ConsoleSystem.Items()) {
            // Exit if word is filtered.
            if (!PassesTypeFilter(item.m_Type) || !m_TextFilter.PassFilter(item.Get().c_str())) {
                itemIndex++;
                continue;
            }

            // Spacing between commands.
            if (item.m_Type == csys::COMMAND) {
                if (m_TimeStamps) ImGui::PushTextWrapPos(ImGui::GetColumnWidth() - timestamp_width);  // Wrap before timestamps start.
                if (count++ != 0) ImGui::Dummy(ImVec2(-1, ImGui::GetFontSize()));                     // No space for the first command.
            }

            // Get item position before rendering.
            ImVec2 textPos = ImGui::GetCursorScreenPos();
            ImVec2 textSize = ImGui::CalcTextSize(item.Get().data());
            ImVec2 textEnd(textPos.x + textSize.x, textPos.y + textSize.y);

            // Check if this line intersects with selection.
            bool lineSelected =
                (textPos.y <= selMax.y && textEnd.y >= selMin.y) && (m_SelectionStartIndex != -1 || m_SelectionStart.x != m_SelectionEnd.x || m_SelectionStart.y != m_SelectionEnd.y);

            // Track selection indices.
            if (mousePos.y >= textPos.y && mousePos.y <= textEnd.y) {
                if (ImGui::IsMouseClicked(0)) {
                    m_SelectionStartIndex = itemIndex;
                }
                if (m_IsSelecting) {
                    m_SelectionEndIndex = itemIndex;
                }
            }

            // Draw selection highlight.
            if (lineSelected) {
                float highlightStartX = textPos.x;
                float highlightEndX = textEnd.x;

                // Clamp to selection bounds for first/last lines.
                if (textPos.y <= selMin.y && textEnd.y >= selMin.y) {
                    highlightStartX = std::max(textPos.x, selMin.x);
                }
                if (textPos.y <= selMax.y && textEnd.y >= selMax.y) {
                    highlightEndX = std::min(textEnd.x, selMax.x);
                }

                drawList->AddRectFilled(ImVec2(highlightStartX, textPos.y), ImVec2(highlightEndX, textEnd.y), IM_COL32(80, 120, 200, 100));

                selectedText += item.Get() + "\n";
            }

            // Items.
            if (m_ColoredOutput) {
                ImGui::PushStyleColor(ImGuiCol_Text, m_ColorPalette[item.m_Type]);
                ImGui::TextUnformatted(item.Get().data());
                ImGui::PopStyleColor();
            } else {
                ImGui::TextUnformatted(item.Get().data());
            }

            // Time stamp.
            if (item.m_Type == csys::COMMAND && m_TimeStamps) {
                // No wrap for timestamps
                ImGui::PopTextWrapPos();

                // Right align.
                ImGui::SameLine(ImGui::GetColumnWidth(-1) - timestamp_width);

                // Draw time stamp.
                ImGui::PushStyleColor(ImGuiCol_Text, m_ColorPalette[COL_TIMESTAMP]);
                ImGui::Text("%02d:%02d:%02d:%04d", ((item.m_TimeStamp / 1000 / 3600) % 24), ((item.m_TimeStamp / 1000 / 60) % 60), ((item.m_TimeStamp / 1000) % 60), item.m_TimeStamp % 1000);
                ImGui::PopStyleColor();
            }

            itemIndex++;
        }

        // Stop wrapping since we are done displaying console items.
        ImGui::PopTextWrapPos();

        // Handle Ctrl+C to copy selected text.
        if (ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_C) && !selectedText.empty()) {
                ImGui::SetClipboardText(selectedText.c_str());
            }
            if (ImGui::IsKeyPressed(ImGuiKey_A)) {
                m_SelectionStartIndex = 0;
                m_SelectionEndIndex = m_ConsoleSystem.Items().size() - 1;
                m_SelectionStart = ImVec2(ImGui::GetCursorScreenPos().x, contentStartY);
                m_SelectionEnd = ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetColumnWidth(), contentEndY);
            }
        }

        // Auto-scroll logs.
        if (m_ScrollToBottom || (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;
    }
    // Loop through command string vector.
    ImGui::EndChild();
}

void ImGuiConsole::InputBar() {
    // Variables.
    ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackCompletion |
                                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways;

    // Only reclaim after enter key is pressed!
    bool reclaimFocus = false;

    // Input widget. (Width an always fixed width)
    ImGui::PushItemWidth(-ImGui::GetStyle().ItemSpacing.x * 7);
    if (ImGui::InputText("Input", &m_Buffer, inputTextFlags, InputCallback, this)) {
        // Validate.
        if (!m_Buffer.empty()) {
            // Run command line input.
            m_ConsoleSystem.RunCommand(m_Buffer);

            // Scroll to bottom after its ran.
            m_ScrollToBottom = true;
        }

        // Keep focus.
        reclaimFocus = true;

        // Clear command line.
        m_Buffer.clear();
    }
    ImGui::PopItemWidth();

    // Reset suggestions when client provides char input.
    if (ImGui::IsItemEdited() && !m_WasPrevFrameTabCompletion) {
        m_CmdSuggestions.clear();
    }
    m_WasPrevFrameTabCompletion = false;

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaimFocus) ImGui::SetKeyboardFocusHere(-1);  // Focus on command line after clearing.
}

void ImGuiConsole::MenuBar() {
    if (ImGui::BeginMenuBar()) {
        // Settings menu.
        if (ImGui::BeginMenu("Settings")) {
            // Colored output
            ImGui::Checkbox("Colored Output", &m_ColoredOutput);
            ImGui::SameLine();
            HelpMaker("Enable colored command output");

            // Auto Scroll
            ImGui::Checkbox("Auto Scroll", &m_AutoScroll);
            ImGui::SameLine();
            HelpMaker("Automatically scroll to bottom of console log");

            // Filter bar
            ImGui::Checkbox("Filter Bar", &m_FilterBar);
            ImGui::SameLine();
            HelpMaker("Enable console filter bar");

            // Time stamp
            ImGui::Checkbox("Time Stamps", &m_TimeStamps);
            ImGui::SameLine();
            HelpMaker("Display command execution timestamps");

            // Reset to default settings
            if (ImGui::Button("Reset settings", ImVec2(ImGui::GetColumnWidth(), 0))) ImGui::OpenPopup("Reset Settings?");

            // Confirmation
            if (ImGui::BeginPopupModal("Reset Settings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("All settings will be reset to default.\nThis operation cannot be undone!\n\n");
                ImGui::Separator();

                if (ImGui::Button("Reset", ImVec2(120, 0))) {
                    DefaultSettings();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::EndMenu();
        }

        // View settings.
        if (ImGui::BeginMenu("Appearance")) {
            // Logging Colors
            ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar;

            ImGui::TextUnformatted("Color Palette");
            ImGui::Indent();
            ImGui::ColorEdit4("Command##", (float*)&m_ColorPalette[COL_COMMAND], flags);
            ImGui::ColorEdit4("Log##", (float*)&m_ColorPalette[COL_LOG], flags);
            ImGui::ColorEdit4("Warning##", (float*)&m_ColorPalette[COL_WARNING], flags);
            ImGui::ColorEdit4("Error##", (float*)&m_ColorPalette[COL_ERROR], flags);
            ImGui::ColorEdit4("Info##", (float*)&m_ColorPalette[COL_INFO], flags);
            ImGui::ColorEdit4("Time Stamp##", (float*)&m_ColorPalette[COL_TIMESTAMP], flags);
            ImGui::Unindent();

            ImGui::Separator();

            // Window transparency.
            ImGui::TextUnformatted("Background");
            ImGui::SliderFloat("Transparency##", &m_WindowAlpha, 0.1f, 1.f);

            ImGui::EndMenu();
        }

        // All scripts.
        if (ImGui::BeginMenu("Scripts")) {
            // Show registered scripts.
            for (const auto& scr_pair : m_ConsoleSystem.Scripts()) {
                if (ImGui::MenuItem(scr_pair.first.c_str())) {
                    m_ConsoleSystem.RunScript(scr_pair.first);
                    m_ScrollToBottom = true;
                }
            }

            // Reload scripts.
            ImGui::Separator();
            if (ImGui::Button("Reload Scripts", ImVec2(ImGui::GetColumnWidth(), 0))) {
                for (const auto& scr_pair : m_ConsoleSystem.Scripts()) {
                    scr_pair.second->Reload();
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

// From imgui_demo.cpp
void ImGuiConsole::HelpMaker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

int ImGuiConsole::InputCallback(ImGuiInputTextCallbackData* data) {
    // Exit if no buffer.
    if (data->BufTextLen == 0 && (data->EventFlag != ImGuiInputTextFlags_CallbackHistory)) return 0;

    // Get input string and console.
    std::string input_str = data->Buf;
    std::string trim_str;
    auto console = static_cast<ImGuiConsole*>(data->UserData);

    // Optimize by only using positions.
    // Trim start and end spaces.
    size_t startPos = console->m_Buffer.find_first_not_of(' ');
    size_t endPos = console->m_Buffer.find_last_not_of(' ');

    // Get trimmed string.
    if (startPos != std::string::npos && endPos != std::string::npos)
        trim_str = console->m_Buffer.substr(startPos, endPos + 1);
    else
        trim_str = console->m_Buffer;

    switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackCompletion: {
            // Find last word.
            size_t startSubtrPos = trim_str.find_last_of(' ');
            csys::AutoComplete* console_autocomplete;

            // Command line is an entire word/string (No whitespace)
            // Determine which autocomplete tree to use.
            if (startSubtrPos == std::string::npos) {
                startSubtrPos = 0;
                console_autocomplete = &console->m_ConsoleSystem.CmdAutocomplete();
            } else {
                startSubtrPos += 1;
                console_autocomplete = &console->m_ConsoleSystem.VarAutocomplete();
            }

            // Validate str
            if (!trim_str.empty()) {
                // Display suggestions on console.
                if (!console->m_CmdSuggestions.empty()) {
                    console->m_ConsoleSystem.Log(csys::COMMAND) << "Suggestions: " << csys::endl;

                    for (const auto& suggestion : console->m_CmdSuggestions) console->m_ConsoleSystem.Log(csys::LOG) << suggestion << csys::endl;

                    console->m_CmdSuggestions.clear();
                }

                // Get partial completion and suggestions.
                std::string partial = console_autocomplete->Suggestions(trim_str.substr(startSubtrPos, endPos + 1), console->m_CmdSuggestions);

                // Autocomplete only when one work is available.
                if (!console->m_CmdSuggestions.empty() && console->m_CmdSuggestions.size() == 1) {
                    data->DeleteChars(static_cast<int>(startSubtrPos), static_cast<int>(data->BufTextLen - startSubtrPos));
                    data->InsertChars(static_cast<int>(startSubtrPos), console->m_CmdSuggestions[0].data());
                    console->m_CmdSuggestions.clear();
                } else {
                    // Partially complete word.
                    if (!partial.empty()) {
                        data->DeleteChars(static_cast<int>(startSubtrPos), static_cast<int>(data->BufTextLen - startSubtrPos));
                        data->InsertChars(static_cast<int>(startSubtrPos), partial.data());
                    }
                }
            }

            // We have performed the completion event.
            console->m_WasPrevFrameTabCompletion = true;
        } break;

        case ImGuiInputTextFlags_CallbackHistory: {
            // Clear buffer.
            data->DeleteChars(0, data->BufTextLen);

            // Init history index
            if (console->m_HistoryIndex == std::numeric_limits<size_t>::min()) console->m_HistoryIndex = console->m_ConsoleSystem.History().GetNewIndex();

            // Traverse history.
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (console->m_HistoryIndex) --(console->m_HistoryIndex);
            } else {
                if (console->m_HistoryIndex < console->m_ConsoleSystem.History().Size()) ++(console->m_HistoryIndex);
            }

            // Get history.
            std::string prevCommand = console->m_ConsoleSystem.History()[console->m_HistoryIndex];

            // Insert commands.
            data->InsertChars(data->CursorPos, prevCommand.data());
        } break;

        case ImGuiInputTextFlags_CallbackCharFilter:
        case ImGuiInputTextFlags_CallbackAlways:
        default:
            break;
    }
    return 0;
}

void ImGuiConsole::SettingsHandler_ClearALl(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {}

void ImGuiConsole::SettingsHandler_ReadInit(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {}

void* ImGuiConsole::SettingsHandler_ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) {
    if (!handler->UserData) return nullptr;

    auto console = static_cast<ImGuiConsole*>(handler->UserData);

    if (strcmp(name, console->m_ConsoleName.c_str()) != 0) return nullptr;
    return (void*)1;
}

void ImGuiConsole::SettingsHandler_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
    if (!handler->UserData) return;

    // Get console.
    auto console = static_cast<ImGuiConsole*>(handler->UserData);

    // Ensure console doesn't reset variables.
    console->m_LoadedFromIni = true;

// Disable warning regarding sscanf when using MVSC
#pragma warning(push)
#pragma warning(disable : 4996)

#define INI_CONSOLE_LOAD_COLOR(type) \
    (std::sscanf(line, #type "=%i,%i,%i,%i", &r, &g, &b, &a) == 4) { console->m_ColorPalette[type] = ImColor(r, g, b, a); }
#define INI_CONSOLE_LOAD_FLOAT(var) \
    (std::sscanf(line, #var "=%f", &f) == 1) { console->var = f; }
#define INI_CONSOLE_LOAD_BOOL(var) \
    (std::sscanf(line, #var "=%i", &b) == 1) { console->var = b == 1; }

    float f;
    int r, g, b, a;

    // Window style/visuals
    if INI_CONSOLE_LOAD_COLOR (COL_COMMAND)
        else if INI_CONSOLE_LOAD_COLOR (COL_LOG) else if INI_CONSOLE_LOAD_COLOR (COL_WARNING) else if INI_CONSOLE_LOAD_COLOR (COL_ERROR) else if INI_CONSOLE_LOAD_COLOR (COL_INFO) else if INI_CONSOLE_LOAD_COLOR (COL_TIMESTAMP) else if INI_CONSOLE_LOAD_FLOAT (m_WindowAlpha)

            // Window settings
            else if INI_CONSOLE_LOAD_BOOL (m_AutoScroll) else if INI_CONSOLE_LOAD_BOOL (m_ScrollToBottom) else if INI_CONSOLE_LOAD_BOOL (m_ColoredOutput) else if INI_CONSOLE_LOAD_BOOL (m_FilterBar) else if INI_CONSOLE_LOAD_BOOL (m_TimeStamps)

            // Log type filters
            else if INI_CONSOLE_LOAD_BOOL (m_ShowCommand) else if INI_CONSOLE_LOAD_BOOL (m_ShowLog) else if INI_CONSOLE_LOAD_BOOL (m_ShowWarning) else if INI_CONSOLE_LOAD_BOOL (m_ShowError) else if INI_CONSOLE_LOAD_BOOL (m_ShowInfo)
#pragma warning(pop)
}

void ImGuiConsole::SettingsHandler_ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {
    if (!handler->UserData) return;
}

void ImGuiConsole::SettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
    if (!handler->UserData) return;

    // Get console.
    auto console = static_cast<ImGuiConsole*>(handler->UserData);

#define INI_CONSOLE_SAVE_COLOR(type)                                                                                                                                      \
    buf->appendf(                                                                                                                                                         \
        #type "=%i,%i,%i,%i\n", (int)(console->m_ColorPalette[type].x * 255), (int)(console->m_ColorPalette[type].y * 255), (int)(console->m_ColorPalette[type].z * 255), \
        (int)(console->m_ColorPalette[type].w * 255)                                                                                                                      \
    )

#define INI_CONSOLE_SAVE_FLOAT(var) buf->appendf(#var "=%.3f\n", console->var)
#define INI_CONSOLE_SAVE_BOOL(var) buf->appendf(#var "=%i\n", console->var)

    // Set header for CONSOLE Console.
    buf->appendf("[%s][%s]\n", handler->TypeName, console->m_ConsoleName.data());

    // Window settings.
    INI_CONSOLE_SAVE_BOOL(m_AutoScroll);
    INI_CONSOLE_SAVE_BOOL(m_ScrollToBottom);
    INI_CONSOLE_SAVE_BOOL(m_ColoredOutput);
    INI_CONSOLE_SAVE_BOOL(m_FilterBar);
    INI_CONSOLE_SAVE_BOOL(m_TimeStamps);
    // Log type filters.
    INI_CONSOLE_SAVE_BOOL(m_ShowCommand);
    INI_CONSOLE_SAVE_BOOL(m_ShowLog);
    INI_CONSOLE_SAVE_BOOL(m_ShowWarning);
    INI_CONSOLE_SAVE_BOOL(m_ShowError);
    INI_CONSOLE_SAVE_BOOL(m_ShowInfo);
    // Window style/visuals
    INI_CONSOLE_SAVE_FLOAT(m_WindowAlpha);
    INI_CONSOLE_SAVE_COLOR(COL_COMMAND);
    INI_CONSOLE_SAVE_COLOR(COL_LOG);
    INI_CONSOLE_SAVE_COLOR(COL_WARNING);
    INI_CONSOLE_SAVE_COLOR(COL_ERROR);
    INI_CONSOLE_SAVE_COLOR(COL_INFO);
    INI_CONSOLE_SAVE_COLOR(COL_TIMESTAMP);

    // End saving.
    buf->append("\n");
}
