#include "tutorial_flipbook.hpp"
#include "engine/core/components/text_renderer.hpp"

void game::TutorialFlipbook::start() {
    // Add callback for next and previous buttons
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(previous_button)) {
        button_component->on_click.add(this, &TutorialFlipbook::previous_page);
    } else {
        tmt::Log::warn("No button component found for previous button in tutorial flipbook.");
    }

    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(next_button)) {
        button_component->on_click.add(this, &TutorialFlipbook::next_page);
    } else {
        tmt::Log::warn("No button component found for next button in tutorial flipbook.");
    }

    // Add callback for open and close buttons
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(open_button)) {
        button_component->on_click.add(this, &TutorialFlipbook::open_flipbook);
    } else {
        tmt::Log::warn("No button component found for open button in tutorial flipbook.");
    }

    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(close_button)) {
        button_component->on_click.add(this, &TutorialFlipbook::close_flipbook);
    } else {
        tmt::Log::warn("No button component found for close button in tutorial flipbook.");
    }

    // Unlock the first 3 pages
    for (size_t i = 0; i < 3; i++) {
        unlocked_pages.insert(i);
    }

    if (tmt::TextRenderer* text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(page_text)) {
        text_component->text = std::format("1/{}", unlocked_pages.size());
    }
}

void game::TutorialFlipbook::update(const tmt::FrameData& time) {}

void game::TutorialFlipbook::end() {}

void game::TutorialFlipbook::next_page(tmt::Button::Context context) {
    if (context.disabled) return;

    // Disable current page's UI entities
    tmt::engine.ecs.disable(ui_entities[current_page], true);

    auto page_it_pos = unlocked_pages.find(current_page);

    // Go to the next unlocked page, if we can't, loop back to the first one
    if (page_it_pos != unlocked_pages.end()) {
        std::advance(page_it_pos, 1);
        current_page = *page_it_pos;
    } else {
        current_page = *unlocked_pages.begin();
    }

    if (tmt::TextRenderer* text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(page_text)) {
        text_component->text = std::format("{}/{}", (std::distance(unlocked_pages.begin(), unlocked_pages.find(current_page)) + 1), unlocked_pages.size());
    }

    // Enable new page's UI entities
    tmt::engine.ecs.enable(ui_entities[current_page], true);
}

void game::TutorialFlipbook::previous_page(tmt::Button::Context context) {
    if (context.disabled) return;

    // Disable current page's UI entities
    tmt::engine.ecs.disable(ui_entities[current_page], true);

    auto page_it_pos = unlocked_pages.find(current_page);

    // Go to the previous unlocked page, if we can't, loop back to the last one
    if (page_it_pos != unlocked_pages.begin()) {
        std::advance(page_it_pos, -1);
        current_page = *page_it_pos;
    } else {
        current_page = *unlocked_pages.rbegin();
    }

    if (tmt::TextRenderer* text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(page_text)) {
        text_component->text = std::format("{}/{}", (std::distance(unlocked_pages.begin(), unlocked_pages.find(current_page)) + 1), unlocked_pages.size());
    }

    // Enable new page's UI entities
    tmt::engine.ecs.enable(ui_entities[current_page], true);
}

void game::TutorialFlipbook::open_flipbook(tmt::Button::Context context) {
    // Set the page to the current page
    current_page = *unlocked_pages.begin();

    // Enable buttons and page text
    tmt::engine.ecs.enable(previous_button, true);
    tmt::engine.ecs.enable(next_button, true);
    tmt::engine.ecs.enable(page_text, true);
    tmt::engine.ecs.enable(close_button, true);

    // Update page text
    if (tmt::TextRenderer* text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(page_text)) {
        text_component->text = std::format("1/{}", unlocked_pages.size());
    }

    // Enable current page's UI entities
    tmt::engine.ecs.enable(ui_entities[current_page], true);
}

void game::TutorialFlipbook::close_flipbook(tmt::Button::Context context) {
    // Disable buttons and page text
    tmt::engine.ecs.disable(previous_button, true);
    tmt::engine.ecs.disable(next_button, true);
    tmt::engine.ecs.disable(page_text, true);
    tmt::engine.ecs.disable(close_button, true);

    // Disable current page's UI entities
    tmt::engine.ecs.disable(ui_entities[current_page], true);
}

void game::TutorialFlipbook::unlock_page(uint8_t page) {
    // Insert the page if it isn't added yet
    if (unlocked_pages.find(page) == unlocked_pages.end()) {
        unlocked_pages.insert(page);
    }
}
