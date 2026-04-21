#pragma once
#include <stdint.h>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <vector>
#include <numbers>
#include <optional>

#ifndef BEFRIEND_MIXINS
    #define BEFRIEND_MIXINS /* Empty, defined by user */
#endif

namespace Tweening {

enum class Ease : uint8_t {
    LINEAR,
    IN_SINE,
    OUT_SINE,
    IN_OUT_SINE,
    IN_QUAD,
    OUT_QUAD,
    IN_OUT_QUAD,
    IN_CUBIC,
    OUT_CUBIC,
    IN_OUT_CUBIC,
    IN_QUART,
    OUT_QUART,
    IN_OUT_QUART,
    IN_QUINT,
    OUT_QUINT,
    IN_OUT_QUINT,
    IN_EXPO,
    OUT_EXPO,
    IN_OUT_EXPO,
    IN_CIRC,
    OUT_CIRC,
    IN_OUT_CIRC,
    IN_BACK,
    OUT_BACK,
    IN_OUT_BACK,
    IN_ELASTIC,
    OUT_ELASTIC,
    IN_OUT_ELASTIC,
    IN_BOUNCE,
    OUT_BOUNCE,
    IN_OUT_BOUNCE,
};

enum class LoopMode : uint8_t {
    RESTART,
    PING_PONG,
};

/* Forward declarations */

/* Primary template, should be specialized for each type we want to tween */
/* Requiresments:
 * - Must inherit from Detail::TweenBase<ITween<T, Args...>>
 * - Must provide a static create function that returns an instance of the tween
 * - Must provide a static create_default function that returns an instance of the tween with default values for all parameters
 * - Must implement the interpolation logic in the interpolate function, which takes a TweenFrame as input
 * - [Optional] Should implement FromToMixin, for providing from/to functions and storing start/end values
 * - [Optional] Should implement TypedCallbackMixin, for providing on_update callbacks with the correct type
 *
 * See ``ITween<T>`` Default specialization for an example of how to implement a tween specialization
 */
template <typename T, typename... Args>
struct ITween;

namespace Detail {

/* Mixin to inject control functions into ITween specializations and convert to Tween handle */
template <typename D>
class ControllableMixin;

}  // namespace Detail

/* Handle to control a tween after creation, can be used to stop, start, restart, or finish the tween. */
class Tween {
   public:
    Tween() = default;

    Tween(std::shared_ptr<void> life, std::function<void()> stop, std::function<void()> start, std::function<void()> restart, std::function<void()> finish, std::function<bool()> is_paused) :
        lifetime(std::move(life)),
        ctrl_stop(std::move(stop)),
        ctrl_start(std::move(start)),
        ctrl_restart(std::move(restart)),
        ctrl_finish(std::move(finish)),
        ctrl_is_paused(std::move(is_paused)) {}

    /* Now copyable shared_ptr handles the refcount */
    Tween(const Tween&) = default;
    Tween& operator=(const Tween&) = default;
    Tween(Tween&&) = default;
    Tween& operator=(Tween&&) = default;

    /* Implicit conversion from any TypedTween */
    template <typename T, typename... Args>
    Tween(const std::shared_ptr<ITween<T, Args...>>& ptr) {
        auto* raw = ptr.get();
        lifetime = ptr;  // shared_ptr<void> extends lifetime
        ctrl_stop = [raw] { raw->do_stop(); };
        ctrl_start = [raw] { raw->do_start(); };
        ctrl_restart = [raw] { raw->do_restart(); };
        ctrl_finish = [raw] { raw->do_finish(); };
        ctrl_is_paused = [raw] { return raw->is_paused(); };
    }

    void stop() {
        if (ctrl_stop) ctrl_stop();
    }
    void start() {
        if (ctrl_start) ctrl_start();
    }
    void restart() {
        if (ctrl_restart) ctrl_restart();
    }
    void finish() {
        if (ctrl_finish) ctrl_finish();
    }

    bool is_paused() const { return ctrl_is_paused ? ctrl_is_paused() : false; }
    bool is_valid() const { return lifetime != nullptr; }

   private:
    std::shared_ptr<void> lifetime;
    std::function<void()> ctrl_stop, ctrl_start, ctrl_restart, ctrl_finish;
    std::function<bool()> ctrl_is_paused;
};

template <typename ITweenType>
class TypedTweenHandle {
    std::shared_ptr<ITweenType> ptr = nullptr;

   public:
    using TweenType = ITweenType;
    TypedTweenHandle() : ptr(ITweenType::make_shared_default()) {}
    explicit TypedTweenHandle(std::shared_ptr<ITweenType> p) : ptr(std::move(p)) {}
    ITweenType* operator->() { return ptr.get(); }
    const ITweenType* operator->() const { return ptr.get(); }
    ITweenType& operator*() { return *ptr; }
    const ITweenType& operator*() const { return *ptr; }

    operator Tween() { return Tween(ptr); }

    bool is_valid() const { return ptr != nullptr; }

    std::shared_ptr<ITweenType> get_ptr() const { return ptr; }
};

template <typename T, typename... Args>
using TypedTween = TypedTweenHandle<ITween<T, Args...>>;

namespace Traits {

/* = operator */
template <typename T, typename... Args>
concept HasEqualOperator = requires(T a, T b) {
    { a = b } -> std::same_as<T&>;
};

/* + operator */
template <typename T, typename... Args>
concept HasAddOperator = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
};

/* - operator */
template <typename T, typename... Args>
concept HasSubOperator = requires(T a, T b) {
    { a - b } -> std::same_as<T>;
};

/* * operator */
template <typename T, typename... Args>
concept HasMulOperator = requires(T a, float b) {
    { a * b } -> std::same_as<T>;
};

/* Lerpable concept, requires all operators needed for linear interpolation */
template <typename T, typename... Args>
concept IsLerpable = HasEqualOperator<T, Args...> && HasAddOperator<T, Args...> && HasSubOperator<T, Args...> && HasMulOperator<T, Args...>;

/* Concept to check if ITween specialization exists for type T and Args... */
template <typename T, typename... Args>
concept HasITween = requires { sizeof(ITween<std::remove_cvref_t<T>, Args...>); };

}  // namespace Traits

namespace Detail {

/* Default context, used by default */
struct Default {};

struct TweenFrame {
    float alpha; /* Normalized time in the current tween cycle, between 0 and 1 */
    uint64_t frame_count;
};

/* Helpers */
template <typename T>
inline void lerp(T& v, const T& a, const T& b, const float t) {
    /* Linear interpolation */
    v = a + (b - a) * t;
}

template <typename T>
inline T lerp(const T& a, const T& b, const float t) {
    /* Linear interpolation */
    return a + (b - a) * t;
}

/* easing function, defined at bottom of file */
inline float apply_ease(float t, Ease ease);

inline float compute_cycle_time(const float duration, const float reverse_delay, const float repeat_delay, const LoopMode mode) {
    const float base = (mode == LoopMode::PING_PONG) ? 2.0f * duration + reverse_delay : duration;
    return base + repeat_delay;
}

/* Mixins */
template <typename Derived>
class EaseMixin {
   public:
    Derived& ease(const Ease easing) {
        ease_value = easing;
        return static_cast<Derived&>(*this);
    }

    Derived& reverse_ease(const Ease easing) {
        reverse_ease_value = easing;
        return static_cast<Derived&>(*this);
    }

    Ease& get_ease() { return ease_value; }
    Ease get_ease() const { return ease_value; }

    Ease get_reverse_ease() const { return reverse_ease_value.value_or(ease_value); }

    bool has_reverse_ease() const { return reverse_ease_value.has_value(); }

   protected:
    BEFRIEND_MIXINS
    Ease ease_value = Ease::LINEAR;
    std::optional<Ease> reverse_ease_value;
};

template <typename Derived>
class TimeMixin {
   public:
    /* duration of tween animation  */
    Derived& duration(const float seconds) {
        duration_value = seconds;
        return static_cast<Derived&>(*this);
    }

    /* Delay before the animation starts */
    Derived& start_delay(const float seconds) {
        start_delay_value = seconds;
        return static_cast<Derived&>(*this);
    }

    /* Delay at the peak of PING_PING cycle before reversing */
    Derived& reverse_delay(const float seconds) {
        reverse_delay_value = seconds;
        return static_cast<Derived&>(*this);
    }

    /* Delay between the end of one cycle and the start of the next when repeating */
    Derived& repeat_delay(const float seconds) {
        repeat_delay_value = seconds;
        return static_cast<Derived&>(*this);
    }

    /* How long to wait after tweening is done to finish tween */
    Derived& end_delay(const float seconds) {
        end_delay_value = seconds;
        return static_cast<Derived&>(*this);
    }

    float get_duration() const { return duration_value; }
    float get_start_delay() const { return start_delay_value; }
    float get_reverse_delay() const { return reverse_delay_value; }
    float get_repeat_delay() const { return repeat_delay_value; }
    float get_end_delay() const { return end_delay_value; }

    float& get_elapsed_time() { return elapsed_time; }
    float get_elapsed_time() const { return elapsed_time; }

   protected:
    BEFRIEND_MIXINS
    float duration_value = 1.0f; /* Default duration is 1 second */

    float start_delay_value = 0.0f;
    float reverse_delay_value = 0.0f;
    float repeat_delay_value = 0.0f;
    float end_delay_value = 0.0f;

    /* Internal timer */
    float elapsed_time = 0.0f;
};

template <typename Derived, typename T>
class FromToMixin {
   public:
    Derived& from(T start_val) {
        this->start_value = std::move(start_val);
        return static_cast<Derived&>(*this);
    }

    Derived& to(T end_val) {
        this->end_value = std::move(end_val);
        return static_cast<Derived&>(*this);
    }

    bool has_start() const { return start_value.has_value(); }

    bool has_end() const { return end_value.has_value(); }

    const T& get_start() const { return start_value.value(); }

    const T& get_end() const { return end_value.value(); }

   protected:
    BEFRIEND_MIXINS
    std::optional<T> start_value {};
    std::optional<T> end_value {};
};

template <typename Derived>
class LoopMixin {
   public:
    /* Loop = 0 -> 1, 0 -> 1, ... */
    Derived& loop(const LoopMode mode = LoopMode::RESTART) {
        loop_mode = mode;
        return static_cast<Derived&>(*this);
    }

    /* PingPong = 0->1, 1->0, ...*/
    Derived& ping_pong() {
        loop_mode = LoopMode::PING_PONG;
        return static_cast<Derived&>(*this);
    }

    LoopMode get_loop_mode() const { return loop_mode; }

   protected:
    BEFRIEND_MIXINS
    LoopMode loop_mode = LoopMode::RESTART;
};

template <typename Derived>
class RepeatMixin {
   public:
    /*
        0 = no repeat, play once
        -1 = infinite repeat
        >0 = repeat count
    */
    Derived& repeat(const int count) {
        repeat_count = count;
        return static_cast<Derived&>(*this);
    }

    int get_repeat_count() const { return repeat_count; }

   protected:
    BEFRIEND_MIXINS
    int repeat_count = 0;
};

/* Implemented and handled by Tweenbase */
template <typename Derived>
class CallbackMixin {
   public:
    /* Fires on the very first tick, before start_delay */
    Derived& on_pre_start(std::function<void()> cb) {
        on_pre_start_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* Fires when animation begins, after start_delay */
    Derived& on_start(std::function<void()> cb) {
        on_start_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* Fires at the beginning of each cycle (including the first) */
    Derived& on_cycle_begin(std::function<void()> cb) {
        on_cycle_begin_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* PING_PONG only: fires when the forward pass reaches t=1, before reverse_delay */
    Derived& on_forward_end(std::function<void()> cb) {
        on_forward_end_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* PING_PONG only: fires when the reverse pass begins, after reverse_delay */
    Derived& on_reverse_begin(std::function<void()> cb) {
        on_reverse_begin_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* PING_PONG only: fires when the reverse pass ends */
    Derived& on_reverse_end(std::function<void()> cb) {
        on_reverse_end_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* Fires at the end of each cycle */
    Derived& on_cycle_end(std::function<void()> cb) {
        on_cycle_end_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* Fires after repeat_delay, just before the next cycle begins */
    Derived& on_repeat(std::function<void()> cb) {
        on_repeat_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }
    /* Fires once when all cycles and end_delay are complete */
    Derived& on_complete(std::function<void()> cb) {
        on_complete_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }

   protected:
    BEFRIEND_MIXINS
    std::function<void()> on_pre_start_cb;
    std::function<void()> on_start_cb;
    std::function<void()> on_cycle_begin_cb;
    std::function<void()> on_forward_end_cb;
    std::function<void()> on_reverse_begin_cb;
    std::function<void()> on_reverse_end_cb;
    std::function<void()> on_cycle_end_cb;
    std::function<void()> on_repeat_cb;
    std::function<void()> on_complete_cb;
};

/* Needs to be Implemented and handled by ITween specialization */
template <typename Derived, typename T>
class TypedCallbackMixin {
   public:
    Derived& on_update(std::function<void(float, T)> cb) {
        on_update_cb = std::move(cb);
        return static_cast<Derived&>(*this);
    }

    void call_update(float t, T value) {
        if (on_update_cb) on_update_cb(t, value);
    }

   protected:
    BEFRIEND_MIXINS
    std::function<void(float, T)> on_update_cb;
};

template <typename Derived>
class ControllableMixin {
   public:
    void inject_control_fns(std::function<void()> stop, std::function<void()> start, std::function<void()> restart, std::function<void()> finish, std::function<bool()> is_paused) {
        ctrl_stop = std::move(stop);
        ctrl_start = std::move(start);
        ctrl_restart = std::move(restart);
        ctrl_finish = std::move(finish);
        ctrl_is_paused = std::move(is_paused);
    }

    void stop() { paused = true; }
    void start() { paused = false; }

    void restart() {
        static_assert(std::derived_from<Derived, TimeMixin<Derived>>, "Restart requires TimeMixin");
        auto* d = static_cast<Derived*>(this);
        d->get_elapsed_time() = 0.0f;
        d->reset_playback_state();
        paused = false;
    }

    void finish() {
        static_assert(std::derived_from<Derived, TimeMixin<Derived>>, "Finish requires TimeMixin");
        static_assert(std::derived_from<Derived, RepeatMixin<Derived>>, "Finish requires RepeatMixin");
        auto* d = static_cast<Derived*>(this);
        const float cycle = compute_cycle_time(d->get_duration(), d->get_reverse_delay(), d->get_repeat_delay(), d->get_loop_mode());
        d->get_elapsed_time() = d->get_start_delay() + cycle * (d->get_repeat_count() + 1) + d->get_end_delay();
    }

    bool is_paused() const { return paused; }

   private:
    BEFRIEND_MIXINS
    bool paused = false;

    std::function<void()> ctrl_stop, ctrl_start, ctrl_restart, ctrl_finish;
    std::function<bool()> ctrl_is_paused;
};

struct SubTweenTag {};

/* Mixins for sub-tweens to convert to Tween handle */
template <typename Parent>
class SubTweenMixin : public SubTweenTag {
   public:
    SubTweenMixin() = default;
    SubTweenMixin(Parent* parent) : parent(parent) {}

    operator Tween() { return parent->operator Tween(); }
    operator Parent&() { return *parent; }
    operator const Parent&() const { return *parent; }

   protected:
    Parent* parent = nullptr;
};

enum class CyclePhase : uint8_t {
    FORWARD,       /* 0 -> 1 */
    REVERSE_DELAY, /* hold at t=1 before reversing (PING_PONG only) */
    REVERSE,       /* 1 -> 0 animation (PING_PONG only) */
    REPEAT_DELAY,  /* hold at terminal value before next cycle */
};

template <typename Derived>
class TweenBase : public EaseMixin<Derived>,
                  public TimeMixin<Derived>,
                  public LoopMixin<Derived>,
                  public RepeatMixin<Derived>,
                  public ControllableMixin<Derived>,
                  public CallbackMixin<Derived> {
   public:
    void tick(const float delta_time) {
        if (this->is_paused()) return;
        if (this->is_done()) return;

        auto& elapsed = this->get_elapsed_time();
        const float duration = this->get_duration();
        const float start_delay = this->get_start_delay();
        const float rev_delay = this->get_reverse_delay();
        const float rep_delay = this->get_repeat_delay();
        const float end_delay = this->get_end_delay();
        const float cycle = compute_cycle_time(duration, rev_delay, rep_delay, this->get_loop_mode());

        /* on_pre_start: fires on the very first tick, before start_delay */
        if (!pre_start_fired) {
            pre_start_fired = true;
            if (this->on_pre_start_cb) this->on_pre_start_cb();
        }

        elapsed += delta_time;

        if (this->get_repeat_count() >= 0) {
            const float total_anim = cycle * (this->get_repeat_count() + 1) - rep_delay;
            const float max_elapsed = start_delay + total_anim + end_delay;
            elapsed = std::min(elapsed, max_elapsed);
        }

        /* Phase 1: start delay */
        if (elapsed < start_delay) return;

        const float anim_elapsed = elapsed - start_delay;
        const float total_anim_time = (this->get_repeat_count() >= 0) ? cycle * (this->get_repeat_count() + 1) - rep_delay : std::numeric_limits<float>::max();

        /* Phase 3: end delay */
        if (anim_elapsed >= total_anim_time) {
            if (!finalized) {
                finalized = true;

                /* Degenerate: duration == 0, never passed through Phase 2 */
                if (frame_count == 0) {
                    if (this->on_start_cb) this->on_start_cb();
                    if (this->on_cycle_begin_cb) this->on_cycle_begin_cb();
                }

                /* End-of-last-cycle callbacks.
                   Guard against double-firing: if last_sub_phase == REPEAT_DELAY
                   then on_cycle_end already fired when we entered that hold. */
                if (last_sub_phase != CyclePhase::REPEAT_DELAY) {
                    if (last_sub_phase == CyclePhase::REVERSE && this->on_reverse_end_cb) this->on_reverse_end_cb();
                    if (this->on_cycle_end_cb) this->on_cycle_end_cb();
                }

                const float last_anim = std::nextafter(total_anim_time, 0.0f);
                bool is_reversing = false;
                CyclePhase phase = CyclePhase::FORWARD;
                const float computed = compute_progress(last_anim, duration, rev_delay, rep_delay, is_reversing, phase);

                float t;
                if (is_reversing && this->has_reverse_ease())
                    t = apply_ease(computed, this->get_reverse_ease());
                else if (is_reversing)
                    t = 1.0f - apply_ease(1.0f - computed, this->get_ease());
                else
                    t = apply_ease(computed, this->get_ease());

                TweenFrame frame { t, frame_count };
                static_cast<Derived*>(this)->interpolate(frame); /* fires on_update */
                frame_count++;

                if (this->on_complete_cb) this->on_complete_cb();
            }
            return;
        }

        finalized = false;

        /* Phase 2: active animation */
        bool is_reversing = false;
        CyclePhase current_phase = CyclePhase::FORWARD;
        const float computed = compute_progress(anim_elapsed, duration, rev_delay, rep_delay, is_reversing, current_phase);

        float t;
        if (is_reversing && this->has_reverse_ease())
            t = apply_ease(computed, this->get_reverse_ease());
        else if (is_reversing)
            t = 1.0f - apply_ease(1.0f - computed, this->get_ease());
        else
            t = apply_ease(computed, this->get_ease());

        TweenFrame frame { t, frame_count };

        /* on_start + on_cycle_begin: first animation tick (also fires after restart) */
        if (frame_count == 0) {
            if (this->on_start_cb) this->on_start_cb();
            if (this->on_cycle_begin_cb) this->on_cycle_begin_cb();
        }

        /* Cycle boundary detection */
        const int cycle_index = (cycle > 0.0f) ? static_cast<int>(anim_elapsed / cycle) : 0;
        const bool new_cycle = (cycle_index > last_cycle_index) && (frame_count > 0);

        if (new_cycle) {
            /*
             * Crossed into a new cycle.  If there was no REPEAT_DELAY hold,
             * on_cycle_end hasn't fired yet, do it now before on_repeat.
             */
            if (last_sub_phase != CyclePhase::REPEAT_DELAY) {
                if (last_sub_phase == CyclePhase::REVERSE && this->on_reverse_end_cb) this->on_reverse_end_cb();
                if (this->on_cycle_end_cb) this->on_cycle_end_cb();
            }
            if (this->on_repeat_cb) this->on_repeat_cb();
            if (this->on_cycle_begin_cb) this->on_cycle_begin_cb();
        } else if (frame_count > 0) {
            /* Intra-cycle phase transition detection */

            /* PING_PONG: forward pass just reached t=1 */
            if (last_sub_phase == CyclePhase::FORWARD && (current_phase == CyclePhase::REVERSE_DELAY || current_phase == CyclePhase::REVERSE)) {
                if (this->on_forward_end_cb) this->on_forward_end_cb();
            }

            /* PING_PONG: reverse pass just started (handles both with and without reverse_delay) */
            if (last_sub_phase != CyclePhase::REVERSE && current_phase == CyclePhase::REVERSE) {
                if (this->on_reverse_begin_cb) this->on_reverse_begin_cb();
            }

            /* PING_PONG: reverse pass just ended, entering repeat_delay hold */
            if (last_sub_phase == CyclePhase::REVERSE && current_phase == CyclePhase::REPEAT_DELAY) {
                if (this->on_reverse_end_cb) this->on_reverse_end_cb();
                if (this->on_cycle_end_cb) this->on_cycle_end_cb();
            }

            /* RESTART: forward pass just ended, entering repeat_delay hold */
            if (last_sub_phase == CyclePhase::FORWARD && current_phase == CyclePhase::REPEAT_DELAY) {
                if (this->on_cycle_end_cb) this->on_cycle_end_cb();
            }
        }

        last_cycle_index = cycle_index;
        last_sub_phase = current_phase;

        static_cast<Derived*>(this)->interpolate(frame); /* fires on_update */
        frame_count++;
    }

    bool is_done() const {
        if (this->get_repeat_count() < 0) return false;
        const float cycle = compute_cycle_time(this->get_duration(), this->get_reverse_delay(), this->get_repeat_delay(), this->get_loop_mode());
        const float total_anim = cycle * (this->get_repeat_count() + 1) - this->get_repeat_delay();
        const float total = this->get_start_delay() + total_anim + this->get_end_delay();
        return this->get_elapsed_time() >= total;
    }

    void reset_playback_state() {
        frame_count = 0;
        finalized = false;
        pre_start_fired = false;
        last_cycle_index = -1;
        last_sub_phase = CyclePhase::FORWARD;
    }

    void set_self_weak(std::shared_ptr<void> sp) { self_weak = sp; }
    std::shared_ptr<void> get_self_shared() { return self_weak.lock(); }

    operator TypedTweenHandle<Derived>() {
        auto sp = std::static_pointer_cast<Derived>(self_weak.lock());
        return TypedTweenHandle<Derived>(std::move(sp));
    }

    operator Tween() {
        auto* d = static_cast<Derived*>(this);
        return Tween(self_weak.lock(), [d] { d->stop(); }, [d] { d->start(); }, [d] { d->restart(); }, [d] { d->finish(); }, [d] { return d->is_paused(); });
    }

   private:
    uint64_t frame_count = 0;
    bool finalized = false;
    bool pre_start_fired = false;
    int last_cycle_index = -1;
    CyclePhase last_sub_phase = CyclePhase::FORWARD;

    std::weak_ptr<void> self_weak;

    float compute_progress(const float anim_elapsed, const float dur, const float rev_delay, const float rep_delay, bool& is_reversing, CyclePhase& phase) const {
        is_reversing = false;

        switch (this->get_loop_mode()) {
            case LoopMode::RESTART: {
                const float cycle_time = dur + rep_delay;
                const float cycle_pos = (cycle_time > 0.0f) ? std::fmod(anim_elapsed, cycle_time) : 0.0f;
                if (cycle_pos >= dur) {
                    phase = CyclePhase::REPEAT_DELAY;
                    return 1.0f;
                }
                phase = CyclePhase::FORWARD;
                return (dur > 0.0f) ? cycle_pos / dur : 1.0f;
            }

            case LoopMode::PING_PONG: {
                const float cycle_time = 2.0f * dur + rev_delay + rep_delay;
                const float cycle_pos = (cycle_time > 0.0f) ? std::fmod(anim_elapsed, cycle_time) : 0.0f;
                if (cycle_pos <= dur) {
                    phase = CyclePhase::FORWARD;
                    return (dur > 0.0f) ? cycle_pos / dur : 1.0f;
                } else if (cycle_pos <= dur + rev_delay) {
                    phase = CyclePhase::REVERSE_DELAY;
                    return 1.0f;
                } else if (cycle_pos <= 2.0f * dur + rev_delay) {
                    is_reversing = true;
                    phase = CyclePhase::REVERSE;
                    const float rev_elapsed = cycle_pos - dur - rev_delay;
                    return (dur > 0.0f) ? 1.0f - (rev_elapsed / dur) : 0.0f;
                } else {
                    phase = CyclePhase::REPEAT_DELAY;
                    return 0.0f;
                }
            }

            default:
                phase = CyclePhase::FORWARD;
                return (dur > 0.0f) ? std::min(anim_elapsed / dur, 1.0f) : 1.0f;
        }
    }
};

/* Manager that stores collections of tweens for each context. */
template <typename Context>
class Manager {
   public:
    static void update(const float delta_time_seconds) {
        for (auto& [_, collection] : collections) {
            if (collection.update) collection.update(delta_time_seconds);
        }
    }

    template <typename CollectionType>
    static void register_collection() {
        const std::type_index type_index(typeid(CollectionType));

        if (collections.find(type_index) != collections.end()) {
            return;  // Collection already registered
        }

        collections[type_index].update = [](float delta_time) { CollectionType::update(delta_time); };
        collections[type_index].clear = []() { CollectionType::clear(); };
    }

    static void clear() {
        for (auto& [_, collection] : collections) {
            if (collection.clear) collection.clear();
        }
    }

   private:
    struct Callbacks {
        std::function<void(float)> update = nullptr;
        std::function<void()> clear = nullptr;
    };

    static inline std::unordered_map<std::type_index, Callbacks> collections;
};

/* Collection that stores all tween of a specialization */
template <typename Context, typename T, typename... Args>
class TweenCollection {
    using TweenType = ITween<T, Args...>;

   public:
    static TweenType& add(TweenType tween) {
        ensure_registered();
        auto ptr = std::make_shared<TweenType>(std::move(tween));
        ptr->set_self_weak(ptr);

        ptr->inject_control_fns(
            [p = ptr.get()] { p->stop(); },     //
            [p = ptr.get()] { p->start(); },    //
            [p = ptr.get()] { p->restart(); },  //
            [p = ptr.get()] { p->finish(); },   //
            [p = ptr.get()] { return p->is_paused(); }
        );

        tweens.push_back(ptr);
        return *ptr;  // reference into the deque-owned shared_ptr, stable forever
    }

    static TweenType& add_existing(std::shared_ptr<TweenType> ptr) {
        ensure_registered();
        ptr->set_self_weak(ptr);
        ptr->inject_control_fns(
            [p = ptr.get()] { p->stop(); },     //
            [p = ptr.get()] { p->start(); },    //
            [p = ptr.get()] { p->restart(); },  //
            [p = ptr.get()] { p->finish(); },   //
            [p = ptr.get()] { return p->is_paused(); }
        );
        tweens.push_back(ptr);
        return *tweens.back();
    }

    static void update(const float delta_time) {
        /* Erase tweens nobody holds a handle to anymore that are also done */
        std::erase_if(tweens, [](const std::shared_ptr<TweenType>& t) {
            /*<br>*/
            return t->is_done() && t.use_count() == 1;
        });

        for (auto& tween : tweens) {
            tween->tick(delta_time);
        }
    }

    static void clear() { tweens.clear(); }

   private:
    static inline std::deque<std::shared_ptr<TweenType>> tweens {};

    static void ensure_registered() {
        static bool registered = [] {
            Detail::Manager<Context>::template register_collection<TweenCollection<Context, T, Args...>>();
            return true;
        }();
        (void)registered;
    }
};

}  // namespace Detail

template <typename Context>
struct Ctx {};

/* Main update */
template <typename Context = Detail::Default>
inline void update(const float delta_time_seconds) {
    Detail::Manager<Context>::update(delta_time_seconds);
}

template <typename Context = Detail::Default>
inline void clear() {
    Detail::Manager<Context>::clear();
}

/* Context entry point */
template <typename... Args, typename T, typename Context>
requires Traits::HasITween<T, Args...>
ITween<std::remove_cvref_t<T>, Args...>& tween(T& value, Ctx<Context>) {
    using BareT = std::remove_cvref_t<T>;
    return Detail::TweenCollection<Context, BareT, Args...>::add(ITween<BareT, Args...>::create(value));
}

/* Default entry point */
template <typename... Args, typename T>
requires Traits::HasITween<T, Args...>
ITween<std::remove_cvref_t<T>, Args...>& tween(T& value) {
    return tween<Args...>(value, Ctx<Detail::Default> {});
}

/* Context-aware overload for TypedTweenHandle */
template <typename T, typename... Args, typename Context>
ITween<T, Args...>& tween(TypedTweenHandle<ITween<T, Args...>>& handle, Ctx<Context>) {
    return Detail::TweenCollection<Context, T, Args...>::add_existing(handle.get_ptr());
}

/* Default-context overload for TypedTweenHandle */
template <typename T, typename... Args>
ITween<T, Args...>& tween(TypedTweenHandle<ITween<T, Args...>>& handle) {
    return tween(handle, Ctx<Detail::Default> {});
}

/* Error fallback */
template <typename... Args, typename T>
void tween(T& value) {
    static_assert(Traits::HasITween<T, Args...>, "No ITween specialization found for type T and Args...");
}

// === Body: TweenType is in the pattern, no default needed ===
template <typename T, typename TweenType>
requires Traits::IsLerpable<T>
struct ITween<T, TweenType> : public Detail::TweenBase<TweenType>, public Detail::FromToMixin<TweenType, T>, public Detail::TypedCallbackMixin<TweenType, T*> {
   protected:
    ITween(T* value) : value(value) {}

   public:
    void interpolate(const Detail::TweenFrame& frame) {
        if (frame.frame_count == 0) {
            if (!this->has_start()) this->from(*value);
            if (!this->has_end()) this->to(*value);
        }
        if (value) Detail::lerp(*value, this->get_start(), this->get_end(), frame.alpha);
        if (this->on_update_cb) this->on_update_cb(frame.alpha, value);
    }

   private:
    T* value = nullptr;
};

/* Entry point: zero-arg case, wires self in as the CRTP type */
template <typename T>
requires Traits::IsLerpable<T>
struct ITween<T> : public ITween<T, ITween<T>> {
    ITween(T* value) : ITween<T, ITween<T>>(value) {}
    static ITween create(T& value) { return ITween(&value); }
    static std::shared_ptr<ITween> make_shared_default() { return std::make_shared<ITween>(nullptr); }
};

struct Owned {};

/* Default owned implemenation */
template <typename T>
requires Traits::IsLerpable<T>
struct ITween<T, Owned> : public ITween<T, ITween<T, Owned>> {
    using Base = ITween<T, ITween<T, Owned>>;

    ITween() : Base(&owned_value) {}
    explicit ITween(T initial) : Base(&owned_value), owned_value(std::move(initial)) {}
    ITween(std::nullptr_t) : Base(&owned_value) {}

    static std::shared_ptr<ITween> make_shared_default() { return std::make_shared<ITween>(); }

    ITween(const ITween&) = delete;
    ITween& operator=(const ITween&) = delete;
    ITween(ITween&&) = delete;
    ITween& operator=(ITween&&) = delete;

    T& get() { return owned_value; }
    const T& get() const { return owned_value; }

   private:
    T owned_value {};
};

}  // namespace Tweening

/* Hide long implementation details */
namespace Tweening::Detail {

inline float apply_ease(float t, Ease ease) {
    using std::cos;
    using std::pow;
    using std::sin;
    using std::sqrt;

    constexpr auto TWEEN_PI = std::numbers::pi_v<float>;

    switch (ease) {
        // Linear
        case Ease::LINEAR:
            return t;

        // Sine
        case Ease::IN_SINE:
            return 1.0f - cos((t * TWEEN_PI) / 2.0f);
        case Ease::OUT_SINE:
            return sin((t * TWEEN_PI) / 2.0f);
        case Ease::IN_OUT_SINE:
            return -(cos(TWEEN_PI * t) - 1.0f) / 2.0f;

        // Quadratic
        case Ease::IN_QUAD:
            return t * t;
        case Ease::OUT_QUAD:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case Ease::IN_OUT_QUAD:
            return (t < 0.5f) ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        // Cubic
        case Ease::IN_CUBIC:
            return t * t * t;
        case Ease::OUT_CUBIC:
            return 1.0f - pow(1.0f - t, 3.0f);
        case Ease::IN_OUT_CUBIC:
            return (t < 0.5f) ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;

        // Quartic
        case Ease::IN_QUART:
            return pow(t, 4.0f);
        case Ease::OUT_QUART:
            return 1.0f - pow(1.0f - t, 4.0f);
        case Ease::IN_OUT_QUART:
            return (t < 0.5f) ? 8.0f * pow(t, 4.0f) : 1.0f - pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;

        // Quintic
        case Ease::IN_QUINT:
            return pow(t, 5.0f);
        case Ease::OUT_QUINT:
            return 1.0f - pow(1.0f - t, 5.0f);
        case Ease::IN_OUT_QUINT:
            return (t < 0.5f) ? 16.0f * pow(t, 5.0f) : 1.0f - pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;

        // Exponential
        case Ease::IN_EXPO:
            return (t == 0.0f) ? 0.0f : pow(2.0f, 10.0f * t - 10.0f);
        case Ease::OUT_EXPO:
            return (t == 1.0f) ? 1.0f : 1.0f - pow(2.0f, -10.0f * t);
        case Ease::IN_OUT_EXPO:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return (t < 0.5f) ? pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;

        // Circular
        case Ease::IN_CIRC:
            return 1.0f - sqrt(1.0f - t * t);
        case Ease::OUT_CIRC:
            return sqrt(1.0f - pow(t - 1.0f, 2.0f));
        case Ease::IN_OUT_CIRC:
            return (t < 0.5f) ? (1.0f - sqrt(1.0f - pow(2.0f * t, 2.0f))) / 2.0f : (sqrt(1.0f - pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;

        // Back
        case Ease::IN_BACK: {
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.0f;
            return c3 * t * t * t - c1 * t * t;
        }
        case Ease::OUT_BACK: {
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.0f;
            float tm = t - 1.0f;
            return 1.0f + c3 * tm * tm * tm + c1 * tm * tm;
        }
        case Ease::IN_OUT_BACK: {
            constexpr float c1 = 1.70158f;
            constexpr float c2 = c1 * 1.525f;
            return (t < 0.5f) ? (pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f : (pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (2.0f * t - 2.0f) + c2) + 2.0f) / 2.0f;
        }

        // Elastic
        case Ease::IN_ELASTIC:
            if (t == 0.0f || t == 1.0f) return t;
            return -pow(2.0f, 10.0f * t - 10.0f) * sin((t * 10.0f - 10.75f) * (2.0f * TWEEN_PI) / 3.0f);
        case Ease::OUT_ELASTIC:
            if (t == 0.0f || t == 1.0f) return t;
            return pow(2.0f, -10.0f * t) * sin((t * 10.0f - 0.75f) * (2.0f * TWEEN_PI) / 3.0f) + 1.0f;
        case Ease::IN_OUT_ELASTIC:
            if (t == 0.0f || t == 1.0f) return t;
            return (t < 0.5f) ? -(pow(2.0f, 20.0f * t - 10.0f) * sin((20.0f * t - 11.125f) * (2.0f * TWEEN_PI) / 4.5f)) / 2.0f
                              : (pow(2.0f, -20.0f * t + 10.0f) * sin((20.0f * t - 11.125f) * (2.0f * TWEEN_PI) / 4.5f)) / 2.0f + 1.0f;
        // Bounce
        case Ease::OUT_BOUNCE: {
            if (t < 1.0f / 2.75f) {
                return 7.5625f * t * t;
            } else if (t < 2.0f / 2.75f) {
                t -= 1.5f / 2.75f;
                return 7.5625f * t * t + 0.75f;
            } else if (t < 2.5f / 2.75f) {
                t -= 2.25f / 2.75f;
                return 7.5625f * t * t + 0.9375f;
            } else {
                t -= 2.625f / 2.75f;
                return 7.5625f * t * t + 0.984375f;
            }
        }
        case Ease::IN_BOUNCE:
            return 1.0f - apply_ease(1.0f - t, Ease::OUT_BOUNCE);
        case Ease::IN_OUT_BOUNCE:
            return (t < 0.5f) ? (1.0f - apply_ease(1.0f - 2.0f * t, Ease::OUT_BOUNCE)) / 2.0f : (1.0f + apply_ease(2.0f * t - 1.0f, Ease::OUT_BOUNCE)) / 2.0f;

        default:
            return t;
    }
}

}  // namespace Tweening::Detail

/* === Loop === */
/*
on_pre_start
  [start_delay]
on_start
on_cycle_begin
  [forward] -> on_update
on_cycle_end
  [end_delay]
on_complete
*/

/* === Loop with repeat === */
/*
on_pre_start
  [start_delay]
on_start
on_cycle_begin
  [forward pass] -> on_update
on_cycle_end
  [repeat_delay]
on_repeat
on_cycle_begin
  [forward pass] -> on_update
on_cycle_end
  [repeat_delay]
on_repeat
on_cycle_begin
  [forward pass] -> on_update
on_cycle_end
  [end_delay]
on_complete
*/

/* === Ping Pong === */
/*
on_pre_start
  [start_delay]
on_start
on_cycle_begin
  [forward pass] -> on_update
on_forward_end
  [reverse_delay]
on_reverse_begin
  [reverse pass] -> on_update
on_reverse_end
on_cycle_end
  [end_delay]
on_complete
*/

/* === Ping Pong with repeat === */
/*
on_pre_start
  [start_delay]
on_start
on_cycle_begin
  [forward pass] -> on_update
on_forward_end
  [reverse_delay]
on_reverse_begin
  [reverse pass] -> on_update
on_reverse_end
on_cycle_end
  [repeat_delay]
on_repeat
...
  [end_delay]
on_complete
*/