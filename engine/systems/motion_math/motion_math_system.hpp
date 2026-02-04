#pragma once
#include "engine/core/system.hpp"
#include "engine/tools/second_order_solver.hpp"
#include <any>
#include <typeindex>
#include "engine/core/logger.hpp"
#include "engine/core/ecs.hpp"
#include "engine/engine.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp"
#include "glm/gtx/quaternion.hpp"

namespace tmt {
class MotionMathSystem : public ISystem {
   public:
    struct MotionSettings {
        // default: critical damping (same as unity's SmoothDamp. initial high velocity, slows down towards end)
        float frequency = 1.f;
        float damping = 1.f;
        float initial_response = 0.f;
    };

    // dispatch single-time motion that drives a value towards a target
    template <typename T>
    void do_motion(T* value, const T& start, const T& end, const MotionSettings& settings = {}, std::optional<tmt::Entity> safeguard = std::nullopt) {
        if (!value) {
            Log::warn(Log::Scope::ENGINE, "[MotionMath] nullptr passed, won't do motion");
            return;
        }

        auto& bucket = get_or_create_bucket<T>();
        auto& motions = std::any_cast<std::vector<Motion<T>>&>(bucket.motion_collection);

        auto it = std::find_if(motions.begin(), motions.end(), [&value](const Motion<T>& motion) { return motion.value == value; });
        if (it != motions.end()) {
            // duplicate pointer driven motion found
            // Log::warn(Log::Scope::ENGINE, "[MotionMath] overlapping pointer target value found, cancelling old motion..");

            // we decide to just erase the previous motion

            motions.erase(it);
        }

        if constexpr (std::is_same_v<T, glm::quat>) {
            auto rotvec = [](const glm::quat& q_in) -> glm::vec3 {
                glm::quat q = glm::normalize(q_in);
                glm::quat l = glm::log(q);               // xyz = axis*(angle/2)
                return 2.0f * glm::vec3(l.x, l.y, l.z);  // axis*angle
            };

            const glm::quat start_q = glm::normalize(start);

            glm::quat end_q = glm::normalize(end);
            end_q = hemi(end_q, start_q);  // shortest path relative to start

            SecondOrderSolver::State<glm::vec3> state {};
            state.current_state = rotvec(start_q);
            state.current_velocity = glm::vec3(0.0f);
            state.current_target = rotvec(end_q);

            motions.emplace_back(state, settings, end_q, value, start_q, safeguard);
        } else {
            SecondOrderSolver::State<T> state {};
            state.current_state = start;

            motions.emplace_back(state, end, value, settings, safeguard);
        }
    }

   private:
    // Inherited via ISystem
    void on_start() override;
    void on_end() override;
    void on_update(const tmt::FrameData&) override {};
    void on_fixed_update(const tmt::FrameData& time) override;

    constexpr std::string get_name() { return "Motion Math System"; };
    // completion epsilons
    static inline const float EPS_POS_COMPLETE = 0.001f;
    static inline const float EPS_VEL_COMPLETE = 0.001f;

    // bucket of same T, type-erased
    struct Bucket {
        std::any motion_collection;  // type erased std::vector<Motion<T>>

        void (*update_method)(std::any& motion_collection, float dt) = nullptr;
    };

    // single-time call motion information
    template <typename T>
    struct Motion {
        SecondOrderSolver::State<T> state;
        T target;
        T* value;
        MotionSettings settings;
        std::optional<tmt::Entity> safeguard;
    };

    // Motion quaternion template specialization
    template <>
    struct tmt::MotionMathSystem::Motion<glm::quat> {
        SecondOrderSolver::State<glm::vec3> state;  // rotvec state + angular velocity
        MotionSettings settings;
        glm::quat target;
        glm::quat* value = nullptr;
        glm::quat prev_target;  // for hemisphere continuity
        std::optional<tmt::Entity> safeguard;
    };

    // squared length of vector and arithmetic
    template <typename T>
    static float norm2_type(const T& value) {
        if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
            float v = static_cast<float>(value);
            return v * v;
        } else {
            // probably glm
            return static_cast<float>(glm::length2(value));
        }
    }

    // angle error squared from a quaternion motion
    static float angle_err2(const tmt::MotionMathSystem::Motion<glm::quat>& motion) {
        glm::quat cur = glm::normalize(*motion.value);
        glm::quat tgt = glm::normalize(motion.target);
        tgt = hemi(tgt, cur);

        glm::quat qerr = tgt * glm::inverse(cur);
        float angle = glm::angle(glm::normalize(qerr));
        return angle * angle;
    }

    // log/acos/dot space fix
    static inline glm::quat hemi(glm::quat q, const glm::quat& ref) { return (glm::dot(q, ref) < 0.0f) ? -q : q; }

    template <typename T>
    static void update_bucket(std::any& motion_collection, float dt) {
        auto& motions = std::any_cast<std::vector<Motion<T>>&>(motion_collection);

        for (auto it = motions.begin(); it != motions.end();) {
            auto& motion = *it;

            if (!motion.value || motion.safeguard && !tmt::engine.ecs.get_registry().valid(*motion.safeguard)) {
                it = motions.erase(it);
                continue;
            }

            if constexpr (std::is_same_v<T, glm::quat>) {
                const bool is_complete = angle_err2(motion) < (EPS_POS_COMPLETE * EPS_POS_COMPLETE) && norm2_type(motion.state.current_velocity) < (EPS_VEL_COMPLETE * EPS_VEL_COMPLETE);

                if (is_complete) {
                    it = motions.erase(it);
                    continue;
                }

                glm::quat tgt = glm::normalize(motion.target);
                motion.prev_target = hemi(tgt, motion.prev_target);

                const glm::quat lt = glm::log(tgt);
                const glm::vec3 tgt_rv = 2.0f * glm::vec3(lt.x, lt.y, lt.z);

                // drive target, quats need transforming from rotation vector
                SecondOrderSolver::solve(motion.state, tgt_rv, motion.settings.frequency, motion.settings.damping, motion.settings.initial_response, dt);
                const glm::vec3 rv = motion.state.current_state;
                const glm::quat out = glm::normalize(glm::exp(glm::quat(0.0f, 0.5f * rv.x, 0.5f * rv.y, 0.5f * rv.z)));

                *motion.value = out;
            } else {
                bool is_complete = norm2_type(motion.state.current_state - motion.target) < (EPS_POS_COMPLETE * EPS_POS_COMPLETE) &&
                                   norm2_type(motion.state.current_velocity) < (EPS_VEL_COMPLETE * EPS_VEL_COMPLETE);
                if (is_complete) {
                    it = motions.erase(it);
                    continue;
                }

                // drive the target value
                SecondOrderSolver::solve(motion.state, motion.target, motion.settings.frequency, motion.settings.damping, motion.settings.initial_response, dt);
                *motion.value = motion.state.current_state;
            }
            ++it;
        }
    }

    // get or create a bucket if bucket for type T doesn't exist
    template <typename T>
    Bucket& get_or_create_bucket() {
        using LogicalType = std::decay_t<T>;

        auto& type_info = typeid(LogicalType);
        if (auto search = motion_buckets.find(type_info); search != motion_buckets.end()) {
            return search->second;
        }

        Bucket bucket;
        bucket.motion_collection = std::vector<Motion<T>> {};
        bucket.update_method = &update_bucket<T>;
        return motion_buckets.emplace(type_info, bucket).first->second;
    }

    std::unordered_map<std::type_index, Bucket> motion_buckets;
};
}  // namespace tmt