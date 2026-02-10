#include "editor_settings.hpp"
#include "editor/shared/save_data.hpp"
#include "editor.hpp"

#include <ImReflect.hpp>

namespace tmt {

void EditorSettingsWindow::display() {
    //
    ImReflect::Input("Editor Settings", editor.save_data);
}

}  // namespace tmt