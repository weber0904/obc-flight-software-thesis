#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#include <csp/csp.h>
#include "csp_semaphore.h"

typedef struct {
    csp_bin_sem_t* key;
    sem_t* handle;
} DarwinSemEntry;

static pthread_mutex_t g_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static DarwinSemEntry g_registry[256];
static unsigned int g_next_id = 0U;

static sem_t* find_sem_locked(csp_bin_sem_t* sem) {
    for (unsigned int i = 0U; i < sizeof(g_registry) / sizeof(g_registry[0]); ++i) {
        if (g_registry[i].key == sem) {
            return g_registry[i].handle;
        }
    }
    return NULL;
}

static int register_sem_locked(csp_bin_sem_t* sem, sem_t* handle) {
    for (unsigned int i = 0U; i < sizeof(g_registry) / sizeof(g_registry[0]); ++i) {
        if (g_registry[i].key == NULL || g_registry[i].key == sem) {
            g_registry[i].key = sem;
            g_registry[i].handle = handle;
            return 0;
        }
    }
    return -1;
}

static sem_t* find_sem(csp_bin_sem_t* sem) {
    pthread_mutex_lock(&g_registry_lock);
    sem_t* const handle = find_sem_locked(sem);
    pthread_mutex_unlock(&g_registry_lock);
    return handle;
}

void csp_bin_sem_init(csp_bin_sem_t* sem) {
    char name[64] = {};
    pthread_mutex_lock(&g_registry_lock);
    const unsigned int id = g_next_id++;
    pthread_mutex_unlock(&g_registry_lock);
    snprintf(name, sizeof(name), "/obc-csp-%d-%u", (int)getpid(), id);

    sem_t* const handle = sem_open(name, O_CREAT | O_EXCL, 0600, 1);
    if (handle != SEM_FAILED) {
        sem_unlink(name);
    }

    pthread_mutex_lock(&g_registry_lock);
    const int registered = register_sem_locked(sem, handle == SEM_FAILED ? NULL : handle);
    pthread_mutex_unlock(&g_registry_lock);

    if (registered != 0 && handle != SEM_FAILED) {
        fprintf(stderr, "csp_semaphore_darwin: semaphore registry exhausted\n");
        sem_close(handle);
    }
}

int csp_bin_sem_wait(csp_bin_sem_t* sem, unsigned int timeout) {
    sem_t* const handle = find_sem(sem);
    if (handle == NULL) {
        return CSP_SEMAPHORE_ERROR;
    }

    if (timeout == CSP_MAX_TIMEOUT) {
        return sem_wait(handle) == 0 ? CSP_SEMAPHORE_OK : CSP_SEMAPHORE_ERROR;
    }

    unsigned int waitedMs = 0U;
    while (waitedMs < timeout) {
        if (sem_trywait(handle) == 0) {
            return CSP_SEMAPHORE_OK;
        }
        if (errno != EAGAIN) {
            return CSP_SEMAPHORE_ERROR;
        }
        usleep(1000);
        waitedMs++;
    }

    return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_post(csp_bin_sem_t* sem) {
    sem_t* const handle = find_sem(sem);
    if (handle == NULL) {
        return CSP_SEMAPHORE_ERROR;
    }

    if (sem_trywait(handle) == 0) {
        return sem_post(handle) == 0 ? CSP_SEMAPHORE_OK : CSP_SEMAPHORE_ERROR;
    }
    if (errno != EAGAIN) {
        return CSP_SEMAPHORE_ERROR;
    }

    if (sem_post(handle) == 0 || errno == EOVERFLOW) {
        return CSP_SEMAPHORE_OK;
    }
    return CSP_SEMAPHORE_ERROR;
}
