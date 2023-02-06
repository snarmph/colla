#include "coropool.h"

#if 0
#include <stdlib.h>
#include <tracelog.h>

#include "jobpool.h"

enum {
    NUM_JOBS = 50
};

struct _pool_internal_t;

typedef union job_t {
    struct {
        mco_coro *co;
        struct _pool_internal_t *pool;
    };
    struct {
        union job_t *next_free;
        void *__padding;
    };
} job_t;

static inline bool _isJobValid(job_t *job) {
    return job->pool != NULL;
}

typedef struct jobchunk_t {
    job_t jobs[NUM_JOBS];
    job_t *first_free;
    struct jobchunk_t *next;
} jobchunk_t;

void _chunkInit(jobchunk_t *chunk) {
    if (!chunk) return;
    chunk->first_free = chunk->jobs;
    for (int i = 0; i < (NUM_JOBS - 1); ++i) {
        chunk->jobs[i].next_free = chunk->jobs + i + 1;
    }
}

jobchunk_t *_chunkNew(jobchunk_t *prev) {
    jobchunk_t *chunk = calloc(1, sizeof(jobchunk_t));
    _chunkInit(chunk);
    prev->next = chunk;
    return chunk;
}

job_t *_chunkGetFirstFree(jobchunk_t *chunk) {
    job_t *first_free = chunk->first_free;
    if (first_free) {
        chunk->first_free = first_free->next_free;
    }
    return first_free;
}

void _chunkSetFree(jobchunk_t *chunk, job_t *job) {
    job->pool = NULL;
    job->next_free = chunk->first_free;
    chunk->first_free = job;
}

typedef struct _pool_internal_t {
    jobpool_t pool;
    jobchunk_t head_chunk;
    cmutex_t chunk_mtx;
} _pool_internal_t;

static int _worker(void *arg);

coropool_t copoInit(uint32 num) {
    _pool_internal_t *pool = calloc(1, sizeof(_pool_internal_t));
    pool->pool = poolInit(num);
    _chunkInit(&pool->head_chunk);
    pool->chunk_mtx = mtxInit();
    return pool;
}

void copoFree(coropool_t copo) {
    _pool_internal_t *pool = copo;

    poolWait(pool->pool);
    poolFree(pool->pool);

    jobchunk_t *chunk = &pool->head_chunk;
    while (chunk) {
        jobchunk_t *next = chunk->next;
        free(chunk);
        chunk = next;
    }
    
    mtxFree(pool->chunk_mtx);
    free(pool);
}

bool copoAdd(coropool_t copo, copo_func_f fn) {
    mco_desc desc = mco_desc_init(fn, 0);
    return copoAddDesc(copo, &desc);
}

bool copoAddDesc(coropool_t copo, mco_desc *desc) {
    mco_coro *co;
    mco_create(&co, desc);
    return copoAddCo(copo, co);
}

static bool _copoAddJob(job_t *job) {
    //return poolAdd(job->pool->pool, _worker, job);
    return true;
}

static bool _copoRemoveJob(job_t *job) {
    _pool_internal_t *pool = job->pool;
    // find chunk
    jobchunk_t *chunk = &pool->head_chunk;
    while (chunk) {
        if (chunk->jobs < job && (chunk->jobs + NUM_JOBS) > job) {
            break;
        }
        chunk = chunk->next;
    }
    if (!chunk) return false;
    mtxLock(pool->chunk_mtx);
    _chunkSetFree(chunk, job);
    mtxUnlock(pool->chunk_mtx);
}

bool copoAddCo(coropool_t copo, mco_coro *co) {
    _pool_internal_t *pool = copo;

    job_t *new_job = NULL;
    jobchunk_t *chunk = &pool->head_chunk;

    mtxLock(pool->chunk_mtx);
    while (!new_job) {
        new_job = _chunkGetFirstFree(chunk);
        if (!new_job) {
            if (!chunk->next) {
                info("adding new chunk");
                chunk->next = _chunkNew(chunk);
            }
            chunk = chunk->next;
        }
    }
    mtxUnlock(pool->chunk_mtx);

    new_job->co = co;
    new_job->pool = pool;

    //return poolAdd(pool->pool, _worker, new_job);
    return _copoAddJob(new_job);
}

void copoWait(coropool_t copo) {
    _pool_internal_t *pool = copo;
    poolWait(pool->pool);
}

static int _worker(void *arg) {
    job_t *job = arg;
    mco_result res = mco_resume(job->co);
    if (res != MCO_DEAD) {
        _copoAddJob(job);
    }
    else {
        _copoRemoveJob(job);
    }
    return 0;
}
#endif

#include <vec.h>

typedef struct {
    vec(mco_coro *) jobs;
    uint32 head;
    cmutex_t work_mutex;
    condvar_t work_cond;
    condvar_t working_cond;
    int32 working_count;
    int32 thread_count;
    bool stop;
} _copo_internal_t;

static mco_coro *_getJob(_copo_internal_t *copo);
static int _copoWorker(void *arg);

coropool_t copoInit(uint32 num) {
    if (!num) num = 2;

    _copo_internal_t *copo = malloc(sizeof(_copo_internal_t));
    *copo = (_copo_internal_t){
        .work_mutex = mtxInit(),
        .work_cond = condInit(),
        .working_cond = condInit(),
        .thread_count = (int32)num
    };

    for (usize i = 0; i < num; ++i) {
        thrDetach(thrCreate(_copoWorker, copo));
    }

    return copo;
}

void copoFree(coropool_t copo_in) {
    _copo_internal_t *copo = copo_in;
    if (!copo) return;

    mtxLock(copo->work_mutex);
    copo->stop = true;
    condWakeAll(copo->work_cond);
    mtxUnlock(copo->work_mutex);

    copoWait(copo);

    vecFree(copo->jobs);
    mtxFree(copo->work_mutex);
    condFree(copo->work_cond);
    condFree(copo->working_cond);
    
    free(copo);
}

bool copoAdd(coropool_t copo, copo_func_f fn) {
    mco_desc desc = mco_desc_init(fn, 0);
    return copoAddDesc(copo, &desc);
}

bool copoAddDesc(coropool_t copo, mco_desc *desc) {
    mco_coro *co;
    mco_create(&co, desc);
    return copoAddCo(copo, co);
}

bool copoAddCo(coropool_t copo_in, mco_coro *co) {
    _copo_internal_t *copo = copo_in;
    if (!copo) return false;

    mtxLock(copo->work_mutex);

    if (copo->head > vecLen(copo->jobs)) {
        vecClear(copo->jobs);
        copo->head = 0;
    }

    vecAppend(copo->jobs, co);

    condWake(copo->work_cond);
    mtxUnlock(copo->work_mutex);

    return true;
}

void copoWait(coropool_t copo_in) {
    _copo_internal_t *copo = copo_in;
    if (!copo) return;
    
    mtxLock(copo->work_mutex);
    // while its either
    //  - working and there's still some threads doing some work
    //  - not working and there's still some threads exiting
    while ((!copo->stop && copo->working_count > 0) || 
            (copo->stop && copo->thread_count > 0)
    ) {
        condWait(copo->working_cond, copo->work_mutex);
    }
    mtxUnlock(copo->work_mutex);
}       

// == PRIVATE FUNCTIONS ===================================

static mco_coro *_getJob(_copo_internal_t *copo) {
    if (copo->head >= vecLen(copo->jobs)) {
        copo->head = 0;
    }
    return copo->jobs[copo->head++];
}

static int _copoWorker(void *arg) {
    _copo_internal_t *copo = arg;

    while (true) {
        mtxLock(copo->work_mutex);
        // wait for a new job
        while (copo->head >= vecLen(copo->jobs) && !copo->stop) {
            condWait(copo->work_cond, copo->work_mutex);
        }

        if (copo->stop) {
            break;
        }

        mco_coro *job = _getJob(copo);
        copo->working_count++;
        mtxUnlock(copo->work_mutex);

        if (job && mco_status(job) != MCO_DEAD) {
            mco_resume(job);
            if (mco_status(job) != MCO_DEAD) {
                copoAddCo(copo, job);
            }
        }

        mtxLock(copo->work_mutex);
        copo->working_count--;
        if (!copo->stop && 
            copo->working_count == 0 && 
            copo->head == vecLen(copo->jobs)
        ) {
            condWake(copo->working_cond);
        }
        mtxUnlock(copo->work_mutex);
    }

    copo->thread_count--;
    condWake(copo->working_cond);
    mtxUnlock(copo->work_mutex);
    return 0;
}