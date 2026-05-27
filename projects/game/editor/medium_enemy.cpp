#if THERMITE_EDITOR

    #include "medium_enemy.hpp"
    #include "../components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* name, game::MediumEnemy& value, ImSettings& settings, ImResponse& response) {
    using namespace game;
    // ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);
    auto& type_settings = settings.get<MediumEnemy>();
    auto& type_response = response.get<MediumEnemy>();

    auto help = [](const char* desc) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("%s", desc);
    };

    /* ── Entity References ───────────────────────────── */
    ImReflect::Input("Walkable Asteroid", value.walkable_asteroid, type_settings, type_response);
    ImReflect::Input("Laser Origin", value.laser_origin, type_settings, type_response);
    ImReflect::Input("Missile Origin", value.missile_origin, type_settings, type_response);
    ImReflect::Input("Core", value.core, type_settings, type_response);
    ImReflect::Input("Rig Controller", value.rig_controller, type_settings, type_response);
    if (ImGui::TreeNodeEx("Available Positions", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImReflect::Input("", value.available_positions, type_settings, type_response);
        ImGui::TreePop();
    }

    /* ── Layers ────────────────────────── */
    if (ImGui::TreeNodeEx("Masks & Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImReflect::Input("Enemy Mask", value.enemy_mask, type_settings, type_response);
        ImReflect::Input("Projectile Layer Mask", value.projectile_mask, type_settings, type_response);
        help("Mask that only excludes enemy projectiles, so they don't collide with themselves");
        ImReflect::Input("Projectile Layer", value.projectile_layer, type_settings, type_response);
        help("Layer all projectiles are put on");
        ImReflect::Input("Enemy Layer", value.enemy_layer, type_settings, type_response);
        help("Layer all enemies are put on");
        ImGui::TreePop();
    }

    ImGui::Spacing();

    /* ── Movement & Detection ────────────────────────── */
    if (ImGui::TreeNodeEx("Movement & Detection", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImReflect::Input("Walk Speed", value.walk_speed, type_settings, type_response);
        ImReflect::Input("Rotation Speed", value.rotation_speed, type_settings, type_response);
        help("Speed at which the enemy adjusts its rotation relative to the ground and its velocity");
        ImReflect::Input("Height Above Ground", value.height_above_ground, type_settings, type_response);
        ImReflect::Input("Aggro Range", value.aggro_range, type_settings, type_response);
        help("When the player enters aggro range, the enemy will start chasing and firing missiles");
        ImReflect::Input("Back Off Distance", value.back_off_distance, type_settings, type_response);
        help("Back Off Distance is the distance at which the enemy will change from trying to get closer to the player to backing away");
        ImReflect::Input("Velocity of Objects on Death", value.velocity_of_objects_on_death, type_settings, type_response);
        help("When the enemy dies, it applies an outward force to all objects that the enemy is made up of. This is the strength of that force.");
        ImGui::TreePop();
    }

    ImGui::Spacing();

    /* ── Attacks ─────────────────────────────────────── */
    if (ImGui::TreeNodeEx("Attacks", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        /* Stomp */
        if (ImGui::TreeNode("Stomp")) {
            ImReflect::Input("Stomp Range", value.stomp_range, type_settings, type_response);
            ImReflect::Input("Stomp Cooldown", value.stomp_cooldown, type_settings, type_response);
            ImReflect::Input("Stomp Windup", value.stomp_windup, type_settings, type_response);
            ImReflect::Input("Stomp Radius", value.stomp_radius, type_settings, type_response);
            ImReflect::Input("Stomp Damage", value.stomp_damage, type_settings, type_response);
            ImGui::TreePop();
        }

        /* Missiles */
        if (ImGui::TreeNode("Missiles")) {
            ImReflect::Input("Missile Voxel Body", value.missile_voxel_object, type_settings, type_response);
            ImReflect::Input("Missile Cooldown", value.missile_cooldown, type_settings, type_response);
            ImReflect::Input("Missile Burst", value.missile_burst, type_settings, type_response);
            help("Number of missiles launched in one burst");
            ImReflect::Input("Burst Interval", value.burst_interval, type_settings, type_response);
            help("Time between missiles in a burst");
            ImReflect::Input("Max Randomness", value.missile_max_randomness, type_settings, type_response);
            help("Maximum random offset added to each missile's trajectory during launching");
            ImReflect::Input("Target Offset", value.target_offset, type_settings, type_response);
            help("Offset from player");
            ImReflect::Input("Rotation Speed", value.missile_rotation_speed, type_settings, type_response);
            help("Speed at which missiles turn towards the direction they're going in");
            ImReflect::Input("Explosion Radius", value.missile_explosion_radius, type_settings, type_response);

            ImGui::Spacing();
            ImGui::TextDisabled("Per-Missile Settings");
            ImGui::Spacing();

            ImReflect::Input("Launch Speed", value.launch_speed, type_settings, type_response);
            help("Initial speed of the missile when launched, decaying towards 0 (launching direction will be the up of the enemy)");
            ImReflect::Input("Stop Launching After", value.stop_launching_after, type_settings, type_response);
            help("Amount of time the missile will take to go from launch speed to 0 (Note: the velocity won't be zero, but no more force in the enemies' up direction will be added)");
            ImReflect::Input("Home Speed", value.home_speed, type_settings, type_response);
            help("Maximum speed the missile will reach when homing. home speed will start at 0 and reach it's maximum at the end of the missiles lifetime");
            ImReflect::Input("Start Homing After", value.start_homing_after, type_settings, type_response);
            help("Amount of time after the missile was created before the missile starts homing towards the player");
            ImReflect::Input("Slow Homing Accuracy After", value.slow_homing_accuracy_after, type_settings, type_response);
            help("Time after which the homing will start becoming less effective");
            ImReflect::Input("Lifetime", value.life_time, type_settings, type_response);

            if (ImGui::TreeNode("Missile Destruction Entities")) {
                ImReflect::Input("Missile Voxel Entities", value.missile_voxel_entities, type_settings, type_response);
                help("All the entities that will be taken into account for checking wether the missiles have been destroyed.");
                ImReflect::Input("Amount of voxels to lose", value.missile_voxel_to_lose, type_settings, type_response);
                help("Amount of voxels that need to be destroyed for the missiles to be destroyed.");
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        /* Laser */
        if (ImGui::TreeNode("Laser")) {
            ImReflect::Input("Laser Charge Voxel Body", value.laser_charge_voxel_object, type_settings, type_response);
            ImReflect::Input("Laser Voxel Body", value.laser_voxel_object, type_settings, type_response);
            ImReflect::Input("Laser Range", value.laser_range, type_settings, type_response);
            ImReflect::Input("Laser Cooldown", value.laser_cooldown, type_settings, type_response);
            ImReflect::Input("Laser Firing Time", value.laser_firing_time, type_settings, type_response);
            ImReflect::Input("Laser Sitting Down Time", value.laser_sitting_down_time, type_settings, type_response);
            ImReflect::Input("Rotation speed during firing laser", value.rotation_speed_during_laser, type_settings, type_response);
            ImReflect::Input("Laser Winding Up Time", value.laser_winding_up_time, type_settings, type_response);
            ImReflect::Input("Laser Damage Per Second", value.laser_damage, type_settings, type_response);
            ImReflect::Input("Laser Damage Radius", value.laser_damage_radius, type_settings, type_response);
            ImReflect::Input("Laser Target Offset", value.laser_target_offset, type_settings, type_response);
            help("Offset from player, purely visual. The point that is used to check if damage should be done to the player, is also moved using the same offset.");

            ImGui::Spacing();
            ImGui::TextDisabled("Laser Advanced Settings");
            ImGui::Spacing();

            ImReflect::Input("Exponential Speed", value.laser_exponential_speed, type_settings, type_response);
            help("The laser starts by following the player exponentially, where the greater the distance is between the laser's target position and the player, the faster it will move");
            ImReflect::Input("Linear Speed", value.laser_linear_speed, type_settings, type_response);
            help("The laser ends by following the player linearly, where the laser moves towards the player's position at a constant speed");
            ImReflect::Input("Linear Threshold", value.laser_linear_threshold, type_settings, type_response);
            help("The higher this value, the longer it takes for linear following to become stronger than the exponential following");
            ImReflect::Input("Max Randomness", value.laser_max_randomness, type_settings, type_response);
            help("Max randomness is the maximum random offset added to the laser's initial direction when firing");
            ImReflect::Input("Prediction Length", value.laser_prediction_length, type_settings, type_response);
            help("Length of the offset added to the laser's target position, in the direction the player is moving");
            ImReflect::Input("Laser Velocity Smoothing", value.laser_vel_smoothing, type_settings, type_response);

            if (ImGui::TreeNode("Laser Destruction Entities")) {
                ImReflect::Input("Laser Voxel Entities", value.laser_voxel_entities, type_settings, type_response);
                help("All the entities that will be taken into account for checking wether the laser has been destroyed.");
                ImReflect::Input("Amount of voxels to lose", value.laser_voxel_to_lose, type_settings, type_response);
                help("Amount of voxels that need to be destroyed for the laser to be destroyed.");
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        /* Sounds */
        if (ImGui::TreeNode("Sounds")) {
            ImReflect::Input("Aggroed Audio", value.sounds.sound_aggroed_audio, type_settings, type_response);
            ImReflect::Input("Laser", value.sounds.sound_laser, type_settings, type_response);
            ImReflect::Input("Missile Explosion", value.sounds.sound_missile_explosion, type_settings, type_response);
            ImReflect::Input("Missile Fire", value.sounds.sound_missile_fire, type_settings, type_response);
            ImReflect::Input("Random Chatter", value.sounds.sound_random_chatter, type_settings, type_response);
            ImReflect::Input("Shield Slam", value.sounds.sound_shield_slam, type_settings, type_response);
            ImReflect::Input("Taunt", value.sounds.sound_taunt, type_settings, type_response);
            ImReflect::Input("Walk", value.sounds.sound_walk, type_settings, type_response);
            ImReflect::Input("Core Destroyed", value.sounds.sound_core_destroyed, type_settings, type_response);

            ImGui::TreePop();
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}
#endif
