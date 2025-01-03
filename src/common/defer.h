#ifndef _DEFER_H
#define _DEFER_H

// defer(xxx) like golang's defer
#define defer _DEFER_ACTION_MAKE /* ([&](){ ... }); */

#define _DEFER_ACTION_MAKE auto \
        _DEFER_ACTION_VAR(_defer_action_line, __LINE__, _) = _DeferredActionCtor
#define _DEFER_ACTION_VAR(a, b, c) _DEFER_TOKEN_CONNECT(a, b, c)
#define _DEFER_TOKEN_CONNECT(a, b, c) a ## b ## c

#include <functional>

class _DeferredAction final {
private:
        std::function<void()> func_;

        template<typename T>
        friend _DeferredAction _DeferredActionCtor(T&& p);

        template<typename T>
        _DeferredAction(T&& p) : func_(std::forward<T>(p)) {}

public:
        _DeferredAction(_DeferredAction&& other) :
                func_(other.func_) {
                other.func_ = nullptr;
        }
        ~_DeferredAction() {
                if (func_) { func_(); }
        }
};

template<typename T>
_DeferredAction _DeferredActionCtor(T&& p) {
        return _DeferredAction(p);
}


#endif
