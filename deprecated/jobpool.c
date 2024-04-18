#include "jobpool.h"

#include "vec.h"

typedef struct {
    cthread_func_t func;
    void *arg;
} job_t;

typedef struct {
    vec(job_t) jobs;
    uint32 head;
    cmutex_t work_mutex;
    condvar_t work_cond;
    condvar_t working_cond;
    int32 working_count;
    int32 thread_count;
    bool stop;
} _pool_internal_t;

static job_t _getJob(_pool_internal_t *pool);
static int _poolWorker(void *arg);

jobpool_t poolInit(uint32 num) {
    if (!num) num = 2;

    _pool_internal_t *pool = malloc(sizeof(_pool_internal_t));
    *pool = (_pool_internal_t){
        .work_mutex = mtxInit(),
        .work_cond = condInit(),
        .working_cond = condInit(),
        .thread_count = (int32)num
    };

    for (usize i = 0; i < num; ++i) {
        thrDetach(thrCreate(_poolWorker, pool));
    }

    return pool;
}

void poolFree(jobpool_t pool_in) {
    _pool_internal_t *pool = pool_in;
    if (!pool) return;

    mtxLock(pool->work_mutex);
    pool->stop = true;
    condWakeAll(pool->work_cond);
    mtxUnlock(pool->work_mutex);

    poolWait(pool);

    vecFree(pool->jobs);
    mtxFree(pool->work_mutex);
    condFree(pool->work_cond);
    condFree(pool->working_cond);
    
    free(pool);
}

bool poolAdd(jobpool_t pool_in, cthread_func_t func, void *arg) {
    _pool_internal_t *pool = pool_in;
    if (!pool) return false;

    mtxLock(pool->work_mutex);

    if (pool->head > vecLen(pool->jobs)) {
        vecClear(pool->jobs);
        pool->head = 0;
    }

    job_t job = { func, arg };
    vecAppend(pool->jobs, job);

    condWake(pool->work_cond);
    mtxUnlock(pool->work_mutex);

    return true;
}

void poolWait(jobpool_t pool_in) {
    _pool_internal_t *pool = pool_in;
    if (!pool) return;
    
    mtxLock(pool->work_mutex);
    // while its either
    //  - working and there's still some threads doing some work
    //  - not working and there's still some threads exiting
    while ((!pool->stop && pool->working_count > 0) || 
            (pool->stop && pool->thread_count > 0)
    ) {
        condWait(pool->working_cond, pool->work_mutex);
    }
    mtxUnlock(pool->work_mutex);
}       

// == PRIVATE FUNCTIONS ===================================

static job_t _getJob(_pool_internal_t *pool) {
    if (pool->head >= vecLen(pool->jobs)) {
        pool->head = 0;
    }
    job_t job = pool->jobs[pool->head++];
    return job;
}

static int _poolWorker(void *arg) {
    _pool_internal_t *pool = arg;

    while (true) {
        mtxLock(pool->work_mutex);
        // wait for a new job
        while (pool->head >= vecLen(pool->jobs) && !pool->stop) {
            condWait(pool->work_cond, pool->work_mutex);
        }

        if (pool->stop) {
            break;
        }

        job_t job = _getJob(pool);
        pool->working_count++;
        mtxUnlock(pool->work_mutex);

        if (job.func) {
            job.func(job.arg);
        }

        mtxLock(pool->work_mutex);
        pool->working_count--;
        if (!pool->stop && 
            pool->working_count == 0 && 
            pool->head == vecLen(pool->jobs)
        ) {
            condWake(pool->working_cond);
        }
        mtxUnlock(pool->work_mutex);
    }

    pool->thread_count--;
    condWake(pool->working_cond);
    mtxUnlock(pool->work_mutex);
    return 0;
}
