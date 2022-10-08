#include <stdio.h>
#include <stdlib.h>
#include <hardware/sensors.h>
#include <pthread.h>
#include <cutils/atomic.h>

#include "SensorEventQueue.cpp"

// Unit tests for the SensorEventQueue.

// Run it like this:
//
// make sensorstests -j32 && \
// out/host/linux-x86/obj/EXECUTABLES/sensorstests_intermediates/sensorstests

bool checkWritableBufferSize(SensorEventQueue* queue, int requested, int expected) {
    sensors_event_t* buffer;
    int actual = queue->getWritableRegion(requested, &buffer);
    if (actual != expected) {
        printf("Expected buffer size was %d; actual was %d\n", expected, actual);
        return false;
    }
    return true;
}

bool checkSize(SensorEventQueue* queue, int expected) {
    int actual = queue->getSize();
    if (actual != expected) {
        printf("Expected queue size was %d; actual was %d\n", expected, actual);
        return false;
    }
    return true;
}

bool checkInt(char* msg, int expected, int actual) {
    if (actual != expected) {
        printf("%s; expected %d; actual was %d\n", msg, expected, actual);
        return false;
    }
    return true;
}

bool testSimpleWriteSizeCounts() {
    printf("testSimpleWriteSizeCounts\n");
    SensorEventQueue* queue = new SensorEventQueue(10);
    if (!checkSize(queue, 0)) return false;
    if (!checkWritableBufferSize(queue, 11, 10)) return false;
    if (!checkWritableBufferSize(queue, 10, 10)) return false;
    if (!checkWritableBufferSize(queue, 9, 9)) return false;

    queue->markAsWritten(7);
    if (!checkSize(queue, 7)) return false;
    if (!checkWritableBufferSize(queue, 4, 3)) return false;
    if (!checkWritableBufferSize(queue, 3, 3)) return false;
    if (!checkWritableBufferSize(queue, 2, 2)) return false;

    queue->markAsWritten(3);
    if (!checkSize(queue, 10)) return false;
    if (!checkWritableBufferSize(queue, 1, 0)) return false;

    printf("passed\n");
    return true;
}

bool testWrappingWriteSizeCounts() {
    printf("testWrappingWriteSizeCounts\n");
    SensorEventQueue* queue = new SensorEventQueue(10);
    queue->markAsWritten(9);
    if (!checkSize(queue, 9)) return false;

    // dequeue from the front
    queue->dequeue();
    queue->dequeue();
    if (!checkSize(queue, 7)) return false;
    if (!checkWritableBufferSize(queue, 100, 1)) return false;

    // Write all the way to the end.
    queue->markAsWritten(1);
    if (!checkSize(queue, 8)) return false;
    // Now the two free spots in the front are available.
    if (!checkWritableBufferSize(queue, 100, 2)) return false;

    // Fill the queue again
    queue->markAsWritten(2);
    if (!checkSize(queue, 10)) return false;

    printf("passed\n");
    return true;
}



struct TaskContext {
  bool success;
  SensorEventQueue* queue;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t dataAvailableCond = PTHREAD_COND_INITIALIZER;

int FULL_QUEUE_CAPACITY = 5;
int FULL_QUEUE_EVENT_COUNT = 31;

void *fullQueueWriterTask(void* ptr) {
    TaskContext* ctx = (TaskContext*)ptr;
    SensorEventQueue* queue = ctx->queue;
    ctx->success = true;
    int totalWaits = 0;
    int totalWrites = 0;
    sensors_event_t* buffer;

    while (totalWrites < FULL_QUEUE_EVENT_COUNT) {
        pthread_mutex_lock(&mutex);
        if (queue->waitForSpace(&mutex)) {
            totalWaits++;
            printf(".");
        }
        int writableSize = queue->getWritableRegion(FULL_QUEUE_CAPACITY, &buffer);
        queue->markAsWritten(writableSize);
        totalWrites += writableSize;
        for (int i = 0; i < writableSize; i++) {
            printf("w");
        }
        pthread_cond_broadcast(&dataAvailableCond);
        pthread_mutex_unlock(&mutex);
    }
    printf("\n");

    ctx->success =
            checkInt("totalWrites", FULL_QUEUE_EVENT_COUNT, totalWrites) &&
            checkInt("totalWaits", FULL_QUEUE_EVENT_COUNT - FULL_QUEUE_CAPACITY, totalWaits);
    return NULL;
}

bool fullQueueReaderShouldRead(int queueSize, int totalReads) {
    if (queueSize == 0) {
        return false;
    }
    int totalWrites = totalReads + queueSize;
    return queueSize == FULL_QUEUE_CAPACITY || totalWrites == FULL_QUEUE_EVENT_COUNT;
}

void* fullQueueReaderTask(void* ptr) {
    TaskContext* ctx = (TaskContext*)ptr;
    SensorEventQueue* queue = ctx->queue;
    int totalReads = 0;
    while (totalReads < FULL_QUEUE_EVENT_COUNT) {
        pthread_mutex_lock(&mutex);
        // Only read if there are events,
        // and either the queue is full, or if we're reading the last few events.
        while (!fullQueueReaderShouldRead(queue->getSize(), totalReads)) {
            pthread_cond_wait(&dataAvailableCond, &mutex);
        }
        queue->dequeue();
        totalReads++;
        printf("r");
        pthread_mutex_unlock(&mutex);
    }
    printf("\n");
    ctx->success = ctx->success && checkInt("totalreads", FULL_QUEUE_EVENT_COUNT, totalReads);
    return NULL;
}

// Test internal queue-full waiting and broadcasting.
bool testFullQueueIo() {
    printf("testFullQueueIo\n");
    SensorEventQueue* queue = new SensorEventQueue(FULL_QUEUE_CAPACITY);

    TaskContext readerCtx;
    readerCtx.success = true;
    readerCtx.queue = queue;

    TaskContext writerCtx;
    writerCtx.success = true;
    writerCtx.queue = queue;

    pthread_t writer, reader;
    pthread_create(&reader, NULL, fullQueueReaderTask, &readerCtx);
    pthread_create(&writer, NULL, fullQueueWriterTask, &writerCtx);

    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    if (!readerCtx.success || !writerCtx.success) return false;
    printf("passed\n");
    return true;
}


int main(int argc, char **argv) {
    if (testSimpleWriteSizeCounts() &&
            testWrappingWriteSizeCounts() &&
            testFullQueueIo()) {
        printf("ALL PASSED\n");
    } else {
        printf("SOMETHING FAILED\n");
    }
    return EXIT_SUCCESS;
}
