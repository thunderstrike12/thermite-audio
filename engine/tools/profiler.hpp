#pragma once

#ifdef THERMITE_EDITOR
#include <tracy/Tracy.hpp>
// Zone scoping
#define TMT_ZONE_SCOPED ZoneScoped
#define TMT_ZONE_SCOPED_N(name) ZoneScopedN(name)
#define TMT_ZONE_SCOPED_C(color) ZoneScopedC(color)
#define TMT_ZONE_SCOPED_NC(name, color) ZoneScopedNC(name, color)
#define TMT_ZONE_TEXT(text, len) ZoneText(text, len)
#define TMT_ZONE_VALUE(value) ZoneValue(value)

#ifndef TMT_CALLSTACK_DEPTH
#define TMT_CALLSTACK_DEPTH 16
#endif
#define TMT_ZONE_SCOPED_S ZoneScopedS(TMT_CALLSTACK_DEPTH)
#define TMT_ZONE_SCOPED_NS(name) ZoneScopedNS(name, TMT_CALLSTACK_DEPTH)
#define TMT_ZONE_SCOPED_CS(color) ZoneScopedCS(color, TMT_CALLSTACK_DEPTH)
#define TMT_ZONE_SCOPED_NCS(name, color) ZoneScopedNCS(name, color, TMT_CALLSTACK_DEPTH)

// Frame marks
#define TMT_FRAME_MARK FrameMark
#define TMT_FRAME_MARK_N(name) FrameMarkNamed(name)
#define TMT_FRAME_MARK_START(name) FrameMarkStart(name)
#define TMT_FRAME_MARK_END(name) FrameMarkEnd(name)

// Plotting values over time
#define TMT_PLOT(name, val) TracyPlot(name, val)
#define TMT_PLOT_CONFIG(name, type, step, fill, color) TracyPlotConfig(name, type, step, fill, color)

// Messages/logging
#define TMT_MESSAGE(text, len) TracyMessage(text, len)
#define TMT_MESSAGE_L(text) TracyMessageL(text)
#define TMT_MESSAGE_C(text, len, color) TracyMessageC(text, len, color)

// Memory tracking
#define TMT_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define TMT_FREE(ptr) TracyFree(ptr)

// Lock profiling
#define TMT_LOCKABLE(type, var) TracyLockable(type, var)
#define TMT_LOCKABLE_N(type, var, name) TracyLockableN(type, var, name)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4595)  // non-member operator new/delete may not be declared inline
#endif

inline void* operator new(std::size_t count) {
    auto ptr = malloc(count);
    TracyAllocS(ptr, count, TMT_CALLSTACK_DEPTH);
    return ptr;
}
inline void operator delete(void* ptr) noexcept {
    TracyFreeS(ptr, TMT_CALLSTACK_DEPTH);
    free(ptr);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#else

// Zone scoping
#define TMT_ZONE_SCOPED
#define TMT_ZONE_SCOPED_N(name)
#define TMT_ZONE_SCOPED_C(color)
#define TMT_ZONE_SCOPED_NC(name, color)
#define TMT_ZONE_TEXT(text, len)
#define TMT_ZONE_VALUE(value)

// Zone scoping with callstacks (no-op)
#define TMT_ZONE_SCOPED_S
#define TMT_ZONE_SCOPED_NS(name)
#define TMT_ZONE_SCOPED_CS(color)
#define TMT_ZONE_SCOPED_NCS(name, color)

// Frame marks
#define TMT_FRAME_MARK
#define TMT_FRAME_MARK_N(name)
#define TMT_FRAME_MARK_START(name)
#define TMT_FRAME_MARK_END(name)

// Plotting
#define TMT_PLOT(name, val)
#define TMT_PLOT_CONFIG(name, type, step, fill, color)

// Messages
#define TMT_MESSAGE(text, len)
#define TMT_MESSAGE_L(text)
#define TMT_MESSAGE_C(text, len, color)

// Memory
#define TMT_ALLOC(ptr, size)
#define TMT_FREE(ptr)

// Locks
#define TMT_LOCKABLE(type, var) type var
#define TMT_LOCKABLE_N(type, var, name) type var

#endif
