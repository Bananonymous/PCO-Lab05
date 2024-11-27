#ifndef QUICKSORT_H
#define QUICKSORT_H

#include <vector>
#include <atomic>
#include <thread>
#include <queue>
#include <iostream> // For debug messages
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcoconditionvariable.h>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>

/**
 * @brief The Task struct represents a subarray to sort.
 */
struct Task {
    int lo;
    int hi;
};

/**
 * @brief The Buffer class implements the task queue with synchronization.
 */
class Buffer {
private:
    PcoMutex mutex;                      // Mutex to protect buffer access
    PcoConditionVariable isFree;         // Condition: space available
    PcoConditionVariable isFull;         // Condition: data available
    std::queue<Task> taskQueue;          // Queue to hold tasks
    size_t maxSize = 10;                 // Default buffer size

public:
    Buffer() = default;

    void put(Task task) {
        mutex.lock(); // Lock the mutex explicitly
        while (taskQueue.size() == maxSize) {
            // Wait while the buffer is full, releasing and relocking the mutex
            std::cout << "[Buffer] Waiting: Buffer full\n";
            isFree.wait(&mutex);
        }
        taskQueue.push(task);
        std::cout << "[Buffer] Added task: " << task.lo << ", " << task.hi << "\n";
        isFull.notifyOne(); // Notify that data is available
        mutex.unlock();     // Unlock the mutex explicitly
    }

    Task get() {
        mutex.lock(); // Lock the mutex explicitly
        while (taskQueue.empty()) {
            // Wait while the buffer is empty, releasing and relocking the mutex
            std::cout << "[Buffer] Waiting: Buffer empty\n";
            isFull.wait(&mutex);
        }
        Task task = taskQueue.front();
        taskQueue.pop();
        std::cout << "[Buffer] Retrieved task: " << task.lo << ", " << task.hi << "\n";
        isFree.notifyOne(); // Notify that space is available
        mutex.unlock();     // Unlock the mutex explicitly
        return task;
    }

    bool empty() {
        mutex.lock();
        const bool isEmpty = taskQueue.empty();
        mutex.unlock();
        return isEmpty;
    }
};

/**
 * @brief The Quicksort class implements a multithreaded Quicksort algorithm.
 */
template<typename T>
class Quicksort {
private:
    std::vector<T>* array;
    Buffer buffer;
    std::atomic<int> activeTasks{0};
    std::vector<PcoThread> threadPool;
    PcoSemaphore isDone;
    bool stopThreads = false;

public:
    explicit Quicksort(unsigned int nbThreads):isDone(0) {
        for (unsigned int i = 0; i < nbThreads; ++i) {
            threadPool.emplace_back(&Quicksort::workerThread, this);
        }
    }

    void sort(std::vector<T>& array) {
        this->array = &array;
        // Check if the buffer is empty, if so, return
        if (array.size() <= 1) {
            waitForCompletion();
            return;
        }
        // Add the initial task (entire array)
        activeTasks++;
        buffer.put({0, (array.size() - 1)});

        // Wait for all tasks to complete
        waitForCompletion();
    }

private:
    void workerThread() {
        while (true) {
            if (stopThreads) return;

            Task task = buffer.get();

            quicksort(task.lo, task.hi);

            if (--activeTasks == 0) {
                isDone.release();
            }
        }
    }

    void quicksort(int lo, int hi) {
        if (lo >= hi) return;

        int pivotIndex = partition(lo, hi);

        // Add new tasks for left and right partitions only if they are valid
        if (lo < pivotIndex - 1) {
            activeTasks++;
            buffer.put({lo, pivotIndex - 1});
        }
        if (pivotIndex + 1 < hi) {
            activeTasks++;
            buffer.put({pivotIndex + 1, hi});
        }
    }

    int partition(int lo, int hi) {
        T pivot = (*array)[hi];
        int i = lo;

        for (int j = lo; j < hi; ++j) {
            if ((*array)[j] <= pivot) {
                std::swap((*array)[i], (*array)[j]);
                i++;
            }
        }
        std::swap((*array)[i], (*array)[hi]);
        return i;
    }

    void waitForCompletion() {
        std::cout << "[Quicksort] All tasks completed, stopping threads\n";
        stopThreads = true;

        // Request all threads to stop
        for (auto& thread : threadPool) {
            thread.requestStop();
        }

        // Join all threads safely
        for (auto& thread : threadPool) {
                thread.join();
        }
    }

};

#endif // QUICKSORT_H