#include "coroutine.h"

coroutine_t coInit() {
    return (coroutine_t) {
        .state = 0
    };
}

bool coIsDead(coroutine_t co) {
    return co.state == -1;
}
