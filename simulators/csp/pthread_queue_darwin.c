#include "pthread_queue.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <csp/csp.h>

static int get_deadline(struct timespec* ts, uint32_t timeoutMs) {
    struct timeval now;
    if (gettimeofday(&now, NULL) != 0) {
        return -1;
    }

    ts->tv_sec = now.tv_sec;
    ts->tv_nsec = now.tv_usec * 1000;

    uint32_t sec = timeoutMs / 1000;
    uint32_t nsec = (timeoutMs - 1000 * sec) * 1000000;
    ts->tv_sec += sec;
    if (ts->tv_nsec + nsec >= 1000000000) {
        ts->tv_sec++;
    }
    ts->tv_nsec = (ts->tv_nsec + nsec) % 1000000000;
    return 0;
}

pthread_queue_t* pthread_queue_create(int length, size_t itemSize) {
    pthread_queue_t* queue = malloc(sizeof(pthread_queue_t));
    if (queue == NULL) {
        return NULL;
    }

    queue->buffer = malloc(length * itemSize);
    if (queue->buffer == NULL) {
        free(queue);
        return NULL;
    }

    queue->size = length;
    queue->item_size = itemSize;
    queue->items = 0;
    queue->in = 0;
    queue->out = 0;

    if (pthread_mutex_init(&(queue->mutex), NULL) != 0 ||
        pthread_cond_init(&(queue->cond_full), NULL) != 0 ||
        pthread_cond_init(&(queue->cond_empty), NULL) != 0) {
        free(queue->buffer);
        free(queue);
        return NULL;
    }

    return queue;
}

void pthread_queue_delete(pthread_queue_t* queue) {
    if (queue == NULL) {
        return;
    }
    free(queue->buffer);
    free(queue);
}

static int wait_slot_available(pthread_queue_t* queue, struct timespec* ts) {
    while (queue->items == queue->size) {
        const int ret = ts == NULL ? pthread_cond_wait(&(queue->cond_full), &(queue->mutex))
                                   : pthread_cond_timedwait(&(queue->cond_full), &(queue->mutex), ts);
        if (ret != 0 && ret != EINTR) {
            return PTHREAD_QUEUE_FULL;
        }
    }
    return PTHREAD_QUEUE_OK;
}

int pthread_queue_enqueue(pthread_queue_t* queue, const void* value, uint32_t timeout) {
    struct timespec ts = {};
    struct timespec* pts = NULL;
    if (timeout != CSP_MAX_TIMEOUT) {
        if (get_deadline(&ts, timeout) != 0) {
            return PTHREAD_QUEUE_ERROR;
        }
        pts = &ts;
    }

    pthread_mutex_lock(&(queue->mutex));
    const int ret = wait_slot_available(queue, pts);
    if (ret == PTHREAD_QUEUE_OK) {
        memcpy((char*)queue->buffer + (queue->in * queue->item_size), value, queue->item_size);
        queue->items++;
        queue->in = (queue->in + 1) % queue->size;
    }
    pthread_mutex_unlock(&(queue->mutex));

    if (ret == PTHREAD_QUEUE_OK) {
        pthread_cond_broadcast(&(queue->cond_empty));
    }
    return ret;
}

static int wait_item_available(pthread_queue_t* queue, struct timespec* ts) {
    while (queue->items == 0) {
        const int ret = ts == NULL ? pthread_cond_wait(&(queue->cond_empty), &(queue->mutex))
                                   : pthread_cond_timedwait(&(queue->cond_empty), &(queue->mutex), ts);
        if (ret != 0 && ret != EINTR) {
            return PTHREAD_QUEUE_EMPTY;
        }
    }
    return PTHREAD_QUEUE_OK;
}

int pthread_queue_dequeue(pthread_queue_t* queue, void* buf, uint32_t timeout) {
    if (queue == NULL) {
        csp_print("csp not initialized\n");
        return PTHREAD_QUEUE_ERROR;
    }

    struct timespec ts = {};
    struct timespec* pts = NULL;
    if (timeout != CSP_MAX_TIMEOUT) {
        if (get_deadline(&ts, timeout) != 0) {
            return PTHREAD_QUEUE_ERROR;
        }
        pts = &ts;
    }

    pthread_mutex_lock(&(queue->mutex));
    const int ret = wait_item_available(queue, pts);
    if (ret == PTHREAD_QUEUE_OK) {
        memcpy(buf, (char*)queue->buffer + (queue->out * queue->item_size), queue->item_size);
        queue->items--;
        queue->out = (queue->out + 1) % queue->size;
    }
    pthread_mutex_unlock(&(queue->mutex));

    if (ret == PTHREAD_QUEUE_OK) {
        pthread_cond_broadcast(&(queue->cond_full));
    }
    return ret;
}

int pthread_queue_items(pthread_queue_t* queue) {
    pthread_mutex_lock(&(queue->mutex));
    const int items = queue->items;
    pthread_mutex_unlock(&(queue->mutex));
    return items;
}

int pthread_queue_free(pthread_queue_t* queue) {
    pthread_mutex_lock(&(queue->mutex));
    const int freeSlots = queue->size - queue->items;
    pthread_mutex_unlock(&(queue->mutex));
    return freeSlots;
}

void pthread_queue_empty(pthread_queue_t* queue) {
    pthread_mutex_lock(&(queue->mutex));
    queue->items = 0;
    pthread_mutex_unlock(&(queue->mutex));
    pthread_cond_broadcast(&(queue->cond_full));
}
