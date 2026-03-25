#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#ifndef IM_CUSTOM_COLORSPACE_MATRIX
    // Matrix to convert from a color in the default (srgb rec709) colorspace to the user's custom colorspace.
    #define IM_CUSTOM_COLORSPACE_MATRIX { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f }
#endif

#define IM_TAU (IM_PI * 2.0f)

struct ImMat3 {
    float rows[3][3];
};

constexpr ImMat3 inverse(const ImMat3& m) {
    /* Cofactors */
    const float c00 = (m.rows[1][1] * m.rows[2][2] - m.rows[1][2] * m.rows[2][1]);
    const float c10 = -(m.rows[1][0] * m.rows[2][2] - m.rows[1][2] * m.rows[2][0]);
    const float c20 = (m.rows[1][0] * m.rows[2][1] - m.rows[1][1] * m.rows[2][0]);

    const float c01 = -(m.rows[0][1] * m.rows[2][2] - m.rows[0][2] * m.rows[2][1]);
    const float c11 = (m.rows[0][0] * m.rows[2][2] - m.rows[0][2] * m.rows[2][0]);
    const float c21 = -(m.rows[0][0] * m.rows[2][1] - m.rows[0][1] * m.rows[2][0]);

    const float c02 = (m.rows[0][1] * m.rows[1][2] - m.rows[0][2] * m.rows[1][1]);
    const float c12 = -(m.rows[0][0] * m.rows[1][2] - m.rows[0][2] * m.rows[1][0]);
    const float c22 = (m.rows[0][0] * m.rows[1][1] - m.rows[0][1] * m.rows[1][0]);

    /* Determinant */
    const float det = m.rows[0][0] * c00 + m.rows[0][1] * c10 + m.rows[0][2] * c20;
    const float inv_det = 1.0f / det;

    /* Adjugate / determinant */
    return ImMat3 { c00 * inv_det, c01 * inv_det, c02 * inv_det, c10 * inv_det, c11 * inv_det, c12 * inv_det, c20 * inv_det, c21 * inv_det, c22 * inv_det };
}

inline void ConvertColorSpace(const ImMat3& m, float color[3]) {
    float out_color[3];

    out_color[0] = m.rows[0][0] * color[0] + m.rows[1][0] * color[1] + m.rows[2][0] * color[2];
    out_color[1] = m.rows[0][1] * color[0] + m.rows[1][1] * color[1] + m.rows[2][1] * color[2];
    out_color[2] = m.rows[0][2] * color[0] + m.rows[1][2] * color[1] + m.rows[2][2] * color[2];

    color[0] = out_color[0];
    color[1] = out_color[1];
    color[2] = out_color[2];
}

enum ImGuiColorWheelFlags : uint8_t {
    ImGuiColorWheelFlags_None = 0,
    ImGuiColorWheelFlags_CustomColorSpace = (1 << 1),
    ImGuiColorWheelFlags_NoInputs = (1 << 2),
};

using ImGuiColorWheelFlags_ = uint8_t;

namespace ImGui {

constexpr ImMat3 CUSTOM_COLOR_MATRIX = IM_CUSTOM_COLORSPACE_MATRIX;
constexpr ImMat3 INV_CUSTOM_COLOR_MATRIX = inverse(CUSTOM_COLOR_MATRIX);

inline float ImLength(const ImVec2& lhs) {
    return ImSqrt((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

/*
Hue and Saturation wheel with value slider.
Default color format is linear srgb rec709, format can be changed by using flag ImGuiColorWheelFlags_CustomColorSpace defining the macro IM_CUSTOM_COLORSPACE_MATRIX.
*/
inline bool ColorWheel3(const char* label, float color[3], ImGuiColorWheelFlags flags = ImGuiColorWheelFlags_None) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiIO& io = g.IO;

    const float width = CalcItemWidth();
    const bool is_readonly = ((g.NextItemData.ItemFlags | g.CurrentItemFlags) & ImGuiItemFlags_ReadOnly) != 0;
    g.NextItemData.ClearFlags();

    PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id) g.ColorEditCurrentID = window->IDStack.back();

    // Setup.
    const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
    const ImU32 black = IM_COL32(0, 0, 0, style_alpha8);
    const ImU32 white = IM_COL32(255, 255, 255, style_alpha8);

    bool hs_cursor_active = false;
    bool v_cursor_active = false;

    // Convert the custom color space to rec709 to work with in ImGui.
    float srgb_color[3];  // Store the color separately to use for the color selection circle later.
    std::memcpy(srgb_color, color, sizeof(srgb_color));
    if (flags & ImGuiColorWheelFlags_CustomColorSpace) ConvertColorSpace(INV_CUSTOM_COLOR_MATRIX, srgb_color);

    // Extract HSV from RGB color.
    float hue, saturation, value;
    ColorConvertRGBtoHSV(srgb_color[0], srgb_color[1], srgb_color[2], hue, saturation, value);
    hue = ImClamp(hue, 0.0f, 1.0f);
    saturation = ImClamp(saturation, 0.0f, 1.0f);
    value = ImClamp(value, 0.0f, 1.0f);

    const float frame_height = GetFrameHeight();
    const ImU32 square_color = ColorConvertFloat4ToU32(ImVec4(srgb_color[0], srgb_color[1], srgb_color[2], 1.0f));
    const ImVec2 size(frame_height, frame_height);
    window->DrawList->AddRectFilled(window->DC.CursorPos, window->DC.CursorPos + size, square_color);
    if (InvisibleButton(label, size) && !IsPopupOpen("WheelPopup")) OpenPopup("WheelPopup");
    SameLine();
    Text("%s", label);

    BeginGroup();

    bool changed = false;
    if (BeginPopup("WheelPopup", ImGuiWindowFlags_NoMove)) {
        const ImGuiWindow* popup_window = GetCurrentWindow();
        const ImVec2 picker_pos = popup_window->DC.CursorPos;

        const float bars_width = GetFrameHeight();                                                                      // Arbitrary smallish width of Hue/Alpha picking bars.
        const float hs_picker_size = ImMax(bars_width * 1.0f, width - 1.0f * (bars_width + style.ItemInnerSpacing.x));  // Hue/Saturation/ picking circle.
        const float bar_pos_x = picker_pos.x + hs_picker_size + style.ItemInnerSpacing.x;
        const float handle_half_height = bars_width * 0.25f;

        const float wheel_thickness = hs_picker_size * 0.08f;
        const float wheel_radius = hs_picker_size * 0.5f;
        const ImVec2 wheel_center(picker_pos.x + hs_picker_size * 0.5f, picker_pos.y + hs_picker_size * 0.5f);

        // Handle hue & saturation circle user input.
        InvisibleButton("HueSaturation", ImVec2(hs_picker_size, hs_picker_size));
        if (IsItemActive() && !is_readonly) {
            // Mouse position inside the circle as normalized device coordinates [-1,1]
            const ImVec2 circle_ndc = ImClamp((io.MousePos - wheel_center) / wheel_radius, ImVec2(-1.0f, -1.0f), ImVec2(1.0f, 1.0f));
            // Calculate the saturation of the clicked point on the circle.
            saturation = ImMin(1.0f, ImLength(circle_ndc));
            // Calculate the hue of the clicked point on the circle.
            hue = ImFmod(ImAtan2(-circle_ndc.y, -circle_ndc.x) / IM_TAU + 0.5f + 0.75f, 1.0f);
            hs_cursor_active = true;
        }

        // Handle value bar user input.
        SetCursorScreenPos(ImVec2(bar_pos_x, picker_pos.y));
        InvisibleButton("Value", ImVec2(bars_width, hs_picker_size));
        if (IsItemActive() && !is_readonly) {
            // Calculate the value of the value bar of the clicked point on the bar.
            value = 1.0f - glm::clamp((io.MousePos.y - (picker_pos.y + handle_half_height)) / (hs_picker_size - (handle_half_height * 2.0f) - 1.0f), 0.0f, 1.0f);
            v_cursor_active = true;
        }

        // Only update the color with the new HSV values if the values changed.
        changed = hs_cursor_active || v_cursor_active;
        if (changed) {
            // Update the input color.
            ColorConvertHSVtoRGB(hue, saturation, value, srgb_color[0], srgb_color[1], srgb_color[2]);

            std::memcpy(color, srgb_color, sizeof(srgb_color));

            // If the value changed, and we are using a custom color space, we have to convert the rec709 back to the custom colorspace.
            if (flags & ImGuiColorWheelFlags_CustomColorSpace) ConvertColorSpace(CUSTOM_COLOR_MATRIX, color);
        }

        // If we do want the manual input colors we add the drag float after converting back, this ensures that the colorspace is always right.
        if (!(flags & ImGuiColorWheelFlags_NoInputs)) changed |= DragFloat3("RGB", color, 0.1f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_ColorMarkers);

        ImDrawList* draw_list = popup_window->DrawList;

        // Draw the color wheel in segments.
        const int wheel_segment_count = draw_list->_CalcCircleAutoSegmentCount(wheel_radius);
        for (int i = 0; i < wheel_segment_count; i++) {
            const float segment1 = static_cast<float>(i) / static_cast<float>(wheel_segment_count - 1);
            const float x1 = ImCos(IM_TAU * segment1) * wheel_radius;
            const float y1 = ImSin(IM_TAU * segment1) * wheel_radius;

            const float segment2 = static_cast<float>((i + 1) % wheel_segment_count) / static_cast<float>(wheel_segment_count - 1);
            const float x2 = ImCos(IM_TAU * segment2) * wheel_radius;
            const float y2 = ImSin(IM_TAU * segment2) * wheel_radius;

            ImVec4 rgb1(0.0f, 0.0f, 0.0f, 1.0f);
            ColorConvertHSVtoRGB(ImFmod(segment1 + 0.75f, 1.0f), 1.0f, value, rgb1.x, rgb1.y, rgb1.z);
            const ImU32 color1 = ColorConvertFloat4ToU32(rgb1);

            ImVec4 rgb2(0.0f, 0.0f, 0.0f, 1.0f);
            ColorConvertHSVtoRGB(ImFmod(segment2 + 0.75f, 1.0f), 1.0f, value, rgb2.x, rgb2.y, rgb2.z);
            const ImU32 color2 = ColorConvertFloat4ToU32(rgb2);

            const ImU32 no_saturation_color = ColorConvertFloat4ToU32(ImVec4(value, value, value, 1.0f));

            const ImVec2 uv_white = GetFontTexUvWhitePixel();
            draw_list->PrimReserve(3, 3);
            draw_list->PrimVtx(wheel_center + ImVec2(x2, y2), uv_white, color2);
            draw_list->PrimVtx(wheel_center, uv_white, no_saturation_color);
            draw_list->PrimVtx(wheel_center + ImVec2(x1, y1), uv_white, color1);
        }

        // Calculate the hue & saturation wheel cursor position.
        const float hs_cursor_angle = hue * IM_TAU + (IM_PI * 0.5f);
        const float hs_cursor_dist = saturation * wheel_radius;
        const ImVec2 hs_cursor_pos(wheel_center.x + ImCos(hs_cursor_angle) * hs_cursor_dist, wheel_center.y + ImSin(hs_cursor_angle) * hs_cursor_dist);

        // Draw hue & saturation wheel cursor/preview circle.
        const float hs_cursor_radius = hs_cursor_active ? wheel_thickness * 0.55f : wheel_thickness * 0.40f;
        const int hs_cursor_segments = draw_list->_CalcCircleAutoSegmentCount(hs_cursor_radius);  // Lock segment count so the +1 one matches others.
        draw_list->AddCircleFilled(hs_cursor_pos, hs_cursor_radius, ColorConvertFloat4ToU32(ImVec4(srgb_color[0], srgb_color[1], srgb_color[2], 1.0f)), hs_cursor_segments);
        draw_list->AddCircle(hs_cursor_pos, hs_cursor_radius + 1, black, hs_cursor_segments);
        draw_list->AddCircle(hs_cursor_pos, hs_cursor_radius, white, hs_cursor_segments);

        // Draw value bar.
        draw_list->AddRectFilledMultiColor(ImVec2(bar_pos_x, picker_pos.y), ImVec2(bar_pos_x + bars_width, picker_pos.y + hs_picker_size), white, white, black, black);

        // Draw value bar handle.
        const float value_selection_y = picker_pos.y + handle_half_height + (hs_picker_size - (handle_half_height * 2.0f) - 1.0f) * (1 - value);
        const ImVec2 selection_min(bar_pos_x, value_selection_y - handle_half_height);
        const ImVec2 selection_max(bar_pos_x + bars_width, value_selection_y + handle_half_height);
        draw_list->AddRectFilled(selection_min, selection_max, white);
        draw_list->AddRect(selection_min, selection_max, black);

        EndPopup();
    }

    EndGroup();

    if (changed && g.LastItemData.ID != 0)  // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        MarkItemEdited(g.LastItemData.ID);

    if (set_current_color_edit_id) g.ColorEditCurrentID = 0;
    PopID();

    return changed;
}

}  // namespace ImGui
