#ifndef QUICKSORT_H
#define QUICKSORT_H

#include <algorithm> // For std::sort
#include <vector>
#include <atomic>
#include <thread>
#include <iostream> // For debug messages
#include <queue>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcoconditionvariable.h>

#include "utils.h"

// Represents a task for the quicksort algorithm, with indices for the subarray to sort
struct Task {
    int lo;
    int hi;
};

template<typename T>
class Quicksort {
    // Pointer to the array being sorted
    std::vector<T> *array;

    // Atomic counter to track active tasks
    std::atomic<int> activeTasks{0};

    // Thread pool for parallel sorting
    std::vector<std::unique_ptr<PcoThread>> threadPool;

    // Queue of tasks to be executed by worker threads
    std::queue<Task> tasks;

    // Threshold for switching to single-threaded sorting (for small subarrays)
    static constexpr int THRESHOLD = 1000;

    // Semaphore to signal completion of all tasks
    PcoSemaphore isDone;

    // Mutex and condition variable for task queue management
    PcoMutex taskMutex;
    PcoConditionVariable taskCondition;

public:
    // Constructor initializes the thread pool with the specified number of threads
    Quicksort(unsigned int nbThreads) : isDone(0) {
        for (unsigned int i = 0; i < nbThreads; ++i) {
            threadPool.emplace_back(std::make_unique<PcoThread>(&Quicksort::workerThread, this));
        }
    }

    // Sorts the input array using the quicksort algorithm
    void sort(std::vector<T> &array) {
        this->array = &array;

        // If the array is already sorted or has 1 or fewer elements, return immediately
        if (array.size() <= 1 || isSorted(array)) {
            signalCompletion();
            return;
        }

        // Initialize the first task and notify workers
        {
            taskMutex.lock();
            activeTasks = 1; // Start with one active task (the whole array)
            tasks.push({0, static_cast<int>(array.size() - 1)});
            taskMutex.unlock();
            taskCondition.notifyOne();
        }

        // Wait for all tasks to complete
        waitForCompletion();
    }

private:
    // Worker thread function that processes tasks from the queue
    void workerThread() {
        try {
            while (true) {
                Task task;

                // Lock the task queue to retrieve a task
                taskMutex.lock();
                while (tasks.empty() && !PcoThread::thisThread()->stopRequested()) {
                    taskCondition.wait(&taskMutex); // Wait for tasks or stop signal
                }

                // Exit if stop is requested and no tasks remain
                if (PcoThread::thisThread()->stopRequested()) {
                    taskMutex.unlock();
                    taskCondition.notifyOne();
                    return;
                }

                // Retrieve the next task
                task = tasks.front();
                tasks.pop();
                taskMutex.unlock();

                // Stop signal task (-1, -1)
                if (task.lo == -1 && task.hi == -1) {
                    return;
                }

                // Perform quicksort on the given subarray
                quicksort(task.lo, task.hi);

                // If this was the last active task, signal completion
                if (activeTasks.fetch_sub(1) == 1) {
                    isDone.release();
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "[WorkerThread] Exception caught: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[WorkerThread] Unknown exception caught\n";
        }
    }

    // Performs the quicksort algorithm on the subarray [lo, hi]
    void quicksort(int lo, int hi) {
        if (lo >= hi || lo < 0 || hi >= array->size()) return;

        // Switch to single-threaded sorting for small subarrays
        if (hi - lo < THRESHOLD) {
            std::sort(array->begin() + lo, array->begin() + hi + 1);
            return;
        }

        // Partition the array and get the pivot index
        int pivotIndex = partition(lo, hi);

        // Add left subarray as a new task
        if (lo < pivotIndex) {
            ++activeTasks;
            taskMutex.lock();
            tasks.push({lo, pivotIndex - 1});
            taskMutex.unlock();
            taskCondition.notifyOne();
        }

        // Add right subarray as a new task
        if (pivotIndex + 1 <= hi) {
            ++activeTasks;
            taskMutex.lock();
            tasks.push({pivotIndex + 1, hi});
            taskMutex.unlock();
            taskCondition.notifyOne();
        }
    }

    // Partitions the subarray [lo, hi] around a pivot
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

    // Waits for all tasks to complete
    void waitForCompletion() {
        isDone.acquire(); // Wait until the semaphore is released
        signalCompletion(); // Clean up threads
    }

    // Signals the end of sorting and shuts down worker threads
    void signalCompletion() {
        // Request all threads to stop
        for (auto &thread : threadPool) {
            thread->requestStop();
        }

        // Push stop signal tasks (-1, -1) for each thread
        taskMutex.lock();
        for (unsigned int i = 0; i < threadPool.size(); ++i) {
            tasks.push({-1, -1});
        }
        taskMutex.unlock();
        taskCondition.notifyOne();

        // Join all threads to ensure they have finished
        for (auto &thread : threadPool) {
            thread->join();
        }
    }
};

#endif // QUICKSORT_H
