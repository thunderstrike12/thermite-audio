#pragma once

#ifdef THERMITE_EDITOR
#include <tracy/Tracy.hpp>

#define TMT_ZONE_SCOPED ZoneScoped
#define TMT_ZONE_SCOPED_N(x) ZoneScopedN(x)
#define TMT_ZONE_TEXT(x, len) ZoneText(x, len)
#define TMT_ZONE_VALUE(x) ZoneValue(x)
#define TMT_FRAME_MARK FrameMark
#define TMT_FRAME_MARK_N(x) FrameMarkNamed(x)
#else
#define TMT_ZONE_SCOPED
#define TMT_ZONE_SCOPED_N(x)
#define TMT_ZONE_TEXT(x, len)
#define TMT_ZONE_VALUE(x)
#define TMT_FRAME_MARK
#define TMT_FRAME_MARK_N(x)
#endif
