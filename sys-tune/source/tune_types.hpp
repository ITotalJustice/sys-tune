#pragma once

#include "../../ipc/tune.h"

#include <functional>

namespace tune {

    enum class PlayerStatus : u8 {
        Playing,
        FetchNext,
    };

    enum class ShuffleMode : u8 {
        Off,
        On,
    };

    enum class RepeatMode : u8 {
        Off,
        One,
        All,
    };

    enum class EnqueueType : u8 {
        Front,
        Back,
    };

    struct CurrentStats : TuneCurrentStats {};

    template<typename Function>
    struct ScopeGuard {
        explicit ScopeGuard(Function&& function) : m_function(std::forward<Function>(function)) {

        }
        ~ScopeGuard() {
            m_function();
        }

        ScopeGuard(const ScopeGuard&) = delete;
        void operator=(const ScopeGuard&) = delete;

    private:
        const Function m_function;
    };

    #define CONCATENATE_IMPL(s1, s2) s1##s2
    #define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
    #define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)

    #define ON_SCOPE_EXIT(_f) tune::ScopeGuard ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){[&] { _f; }};

}
