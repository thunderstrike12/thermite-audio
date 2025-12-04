#pragma once

#define EDITOR_SAFE_GUARD static_assert(THERMITE_EDITOR, "Include Error: Editor header '" __FILE__ "' is being included in non-editor build");