#include "walking.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"

void Walking::start() {
    tmt::GoapAgentFactory::spawn_agent_from_type("dragon", entity);
}

void Walking::update(const tmt::FrameData& time) {}

void Walking::end() {}
