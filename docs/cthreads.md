# Threads
----------

Cross platform threads, mutexes and conditional variables.
The api is very similar to pthreads, here is a small example program that uses threads and mutexes.

```c
struct {
    bool exit;
    cmutex_t mtx;
} state = {0};

int f1(void *) {
    mtxLock(state.mtx);
        state.exit = true;
    mtxUnlock(state.mtx);
    return 0;
}

int f2(void *) {
    while (true) {
        bool exit = false;
        if (mtxTryLock(state.mtx)) {
            exit = state.exit;
            mtxUnlock(state.mtx);
        }

        if (exit) {
            break;
        }
    }
    return 0;
}

int main() {
    state.mtx = mtxInit();

    cthread_t t1 = thrCreate(f1, NULL);
    thrDetach(t1);
    
    cthread_t t2 = thrCreate(f2, NULL);
    thrJoin(t2, NULL);

    mtxFree(state.mtx);
}
```