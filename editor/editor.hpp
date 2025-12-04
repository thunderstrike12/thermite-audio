#pragma once
#include <graphite/imgui.hh>

#include "engine/events/engine.hpp"

namespace tmt {
class Editor : public OnEngineInit, public OnEngineUpdate, public OnEngineFixedUpdate, public OnEngineEnd {
   public:
    Editor() = default;

    void init();

   private:
    void on_engine_init(const ApplicationSpecs& specs) override;
    void on_engine_update(const FrameData& time) override;
    void on_engine_fixed_update(const FrameData& time) override;
    void on_engine_end() override;

    ImGUI imgui {};
};

/* Singleton */
extern Editor editor;

}  // namespace tmt