#pragma once
#include <variant>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/tools/tweening.hpp"
#include "engine/core/reflection.hpp"

namespace tmt::transform_tweening {

enum class Space {
    LOCAL,
    WORLD,
};

/* Sub mixins */
template <typename Derived>
struct WorldOrLocalMixin {
    Derived& local_space() {
        space = Space::LOCAL;
        return static_cast<Derived&>(*this);
    }
    Derived& world_space() {
        space = Space::WORLD;
        return static_cast<Derived&>(*this);
    }

    Space get_space() const { return space; }

   protected:
    BEFRIEND_MIXINS
    Space space = Space::LOCAL; /* Default to local space */
};

template <typename Derived, typename T>
struct OffsetMixin {
    Derived& offset(const T& offset_val) {
        this->offset_value = offset_val;
        return static_cast<Derived&>(*this);
    }

    bool has_offset() const { return offset_value.has_value(); }

    const T& get_offset() const { return offset_value.value(); }

   protected:
    BEFRIEND_MIXINS
    std::optional<T> offset_value {};
};

/* Sub tweens */
template <typename ITween>
struct Move : public Tweening::Detail::FromToMixin<Move<ITween>, glm::vec3>,  //
              public Tweening::Detail::TypedCallbackMixin<Move<ITween>, tmt::Transform&>,
              public Tweening::Detail::SubTweenMixin<ITween>,
              public WorldOrLocalMixin<Move<ITween>>,
              public OffsetMixin<Move<ITween>, glm::vec3> {
    using value_type = glm::vec3;
    Move() = default;
    Move(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
};

template <typename ITween>
struct Rotate : public Tweening::Detail::FromToMixin<Rotate<ITween>, glm::vec3>,  //
                public Tweening::Detail::TypedCallbackMixin<Rotate<ITween>, tmt::Transform&>,
                public Tweening::Detail::SubTweenMixin<ITween>,
                public WorldOrLocalMixin<Rotate<ITween>>,
                public OffsetMixin<Rotate<ITween>, glm::vec3> {
    using value_type = glm::vec3;
    Rotate() = default;
    Rotate(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
};

template <typename ITween>
struct Scale : public Tweening::Detail::FromToMixin<Scale<ITween>, glm::vec3>,  //
               public Tweening::Detail::TypedCallbackMixin<Scale<ITween>, tmt::Transform&>,
               public Tweening::Detail::SubTweenMixin<ITween>,
               public WorldOrLocalMixin<Scale<ITween>>,
               public OffsetMixin<Scale<ITween>, glm::vec3> {
    using value_type = glm::vec3;
    Scale() = default;
    Scale(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
};

}  // namespace tmt::transform_tweening

TMT_OBJECT_TEMPLATE((typename Derived), tmt::transform_tweening::WorldOrLocalMixin, (Derived), space);
TMT_OBJECT_TEMPLATE((typename Derived, typename T), tmt::transform_tweening::OffsetMixin, (Derived, T), offset_value);

template <>
struct Tweening::ITween<tmt::Entity, tmt::Transform> : public Tweening::Detail::TweenBase<ITween<tmt::Entity, tmt::Transform>> {
    using Move = tmt::transform_tweening::Move<ITween>;
    using Rotate = tmt::transform_tweening::Rotate<ITween>;
    using Scale = tmt::transform_tweening::Scale<ITween>;

   public:
    ITween() = default;
    ITween(const tmt::Entity entity) : owner_entity(entity) {};
    static ITween create(const tmt::Entity& entity) { return ITween(entity); }
    static std::shared_ptr<ITween> make_shared_default() { return std::make_shared<ITween>(static_cast<tmt::Entity>(entt::null)); }

    void interpolate(const Tweening::Detail::TweenFrame& frame) {
        if (tmt::engine.ecs.valid(owner_entity) == false) {
            return;
        }
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(owner_entity);
        const float t = frame.alpha;

        if (frame.frame_count == 0) {
            /* On first frame, if start or end are not set, initialize them to current value */
            /* If we have an offset, use start + offset as end value */
            std::visit(
                [&](auto& sub) {
                    using SubType = std::decay_t<decltype(sub)>;
                    const bool has_offset = sub.has_offset();
                    const bool is_local = (sub.get_space() == tmt::transform_tweening::Space::LOCAL);

                    if constexpr (std::is_same_v<SubType, Move>) {
                        if (!sub.has_start()) sub.from(is_local ? transform.get_local_position() : transform.get_world_position());
                        if (has_offset)
                            sub.to(sub.get_start() + sub.get_offset());
                        else if (!sub.has_end())
                            sub.to(is_local ? transform.get_local_position() : transform.get_world_position());
                    } else if constexpr (std::is_same_v<SubType, Rotate>) {
                        if (!sub.has_start()) sub.from(glm::eulerAngles(is_local ? transform.get_local_rotation() : transform.get_world_rotation()));
                        if (has_offset)
                            sub.to(sub.get_start() + sub.get_offset());
                        else if (!sub.has_end())
                            sub.to(glm::eulerAngles(is_local ? transform.get_local_rotation() : transform.get_world_rotation()));
                    } else if constexpr (std::is_same_v<SubType, Scale>) {
                        if (!sub.has_start()) sub.from(is_local ? transform.get_local_scale() : transform.get_world_scale());
                        if (has_offset)
                            sub.to(sub.get_start() * sub.get_offset());
                        else if (!sub.has_end())
                            sub.to(is_local ? transform.get_local_scale() : transform.get_world_scale());
                    }
                },
                tween_variant
            );
        }

        std::visit(
            [&](auto& sub) {
                using SubType = std::decay_t<decltype(sub)>;
                const bool is_local = (sub.get_space() == tmt::transform_tweening::Space::LOCAL);
                if constexpr (std::is_same_v<SubType, Move>) {
                    if (is_local) {
                        transform.set_local_position(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    } else {
                        transform.set_world_position(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    }
                } else if constexpr (std::is_same_v<SubType, Rotate>) {
                    if (is_local) {
                        transform.set_local_rotation(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    } else {
                        transform.set_world_rotation(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    }
                } else if constexpr (std::is_same_v<SubType, Scale>) {
                    if (is_local) {
                        transform.set_local_scale(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    } else {
                        transform.set_world_scale(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    }
                }

                sub.call_update(t, transform);
            },
            tween_variant
        );
    }

    ITween& owner(const tmt::Entity entity) {
        owner_entity = entity;
        return *this;
    }

    Move& move() {
        tween_variant.emplace<Move>(this);
        return std::get<Move>(tween_variant);
    }

    Rotate& rotate() {
        tween_variant.emplace<Rotate>(this);
        return std::get<Rotate>(tween_variant);
    }

    Scale& scale() {
        tween_variant.emplace<Scale>(this);
        return std::get<Scale>(tween_variant);
    }

   private:
    BEFRIEND_VISITABLE()
    tmt::Entity owner_entity = entt::null;

    std::variant<Move, Rotate, Scale> tween_variant;
};

TMT_OBJECT_TEMPLATE((), Tweening::ITween, (tmt::Entity, tmt::Transform), owner_entity);