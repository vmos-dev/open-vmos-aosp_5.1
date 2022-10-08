
#include <pthread.h>

#ifndef __WIFI_HAL_SYNC_H__
#define __WIFI_HAL_SYNC_H__

class Mutex
{
private:
    pthread_mutex_t mMutex;
public:
    Mutex() {
        pthread_mutex_init(&mMutex, NULL);
    }
    ~Mutex() {
        pthread_mutex_destroy(&mMutex);
    }
    int tryLock() {
        return pthread_mutex_trylock(&mMutex);
    }
    int lock() {
        return pthread_mutex_lock(&mMutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mMutex);
    }
};

class Condition
{
private:
    pthread_cond_t mCondition;
    pthread_mutex_t mMutex;

public:
    Condition() {
        pthread_mutex_init(&mMutex, NULL);
        pthread_cond_init(&mCondition, NULL);
    }
    ~Condition() {
        pthread_cond_destroy(&mCondition);
        pthread_mutex_destroy(&mMutex);
    }

    int wait() {
        return pthread_cond_wait(&mCondition, &mMutex);
    }

    void signal() {
        pthread_cond_signal(&mCondition);
    }
};

#endif