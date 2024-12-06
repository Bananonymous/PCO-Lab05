#ifndef QUICKSORT_H
#define QUICKSORT_H

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

struct Task {
    int lo;
    int hi;
};

template<typename T>
class Quicksort {
private:
    std::vector<T> *array;
    std::atomic<int> activeTasks{0};
    std::vector<PcoThread> threadPool;
    std::queue<Task> tasks;
    PcoSemaphore isDone;
    PcoMutex taskMutex;
    PcoConditionVariable taskCondition;

public:
    Quicksort(unsigned int nbThreads) : isDone(0) {
        for (unsigned int i = 0; i < nbThreads; ++i) {
            threadPool.emplace_back(&Quicksort::workerThread, this);
        }
    }

    void sort(std::vector<T> &array) {
        this->array = &array;
        if (array.size() <= 1 || isSorted(array)) {
            signalCompletion();
            return;
        } {
            taskMutex.lock();
            activeTasks = 1;  // Initialize to 1 for the initial task
            tasks.push({0, static_cast<int>(array.size() - 1)});
            taskMutex.unlock();
            taskCondition.notifyOne();
        }

        waitForCompletion();
    }

private:
    void workerThread() {
        try {
            while (true) {
                Task task;
                taskMutex.lock();
                while (tasks.empty() && !PcoThread::thisThread()->stopRequested()) {
                    taskCondition.wait(&taskMutex);
                }

                if (tasks.empty() && PcoThread::thisThread()->stopRequested()) {
                    taskMutex.unlock();
                    return;
                }

                if (!tasks.empty()) {
                    task = tasks.front();
                    tasks.pop();
                } else {
                    taskMutex.unlock();
                    continue;
                }
                taskMutex.unlock();

                if (task.lo == -1 && task.hi == -1) {
                    return;
                }

                quicksort(task.lo, task.hi);

                if (activeTasks.fetch_sub(1) == 1) {
                    // This was the last active task
                    isDone.release();
                }

            }
        } catch (const std::exception &e) {
            std::cerr << "[WorkerThread] Exception caught: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[WorkerThread] Unknown exception caught\n";
        }
    }

    void quicksort(int lo, int hi) {
        if (lo >= hi || lo < 0 || hi >= array->size()) return;

        int pivotIndex = partition(lo, hi);

        if (lo < pivotIndex) {
            activeTasks.fetch_add(1);
            taskMutex.lock();
            tasks.push({lo, pivotIndex - 1});
            taskMutex.unlock();
            taskCondition.notifyOne();
        }

        if (pivotIndex + 1 <= hi) {
            activeTasks.fetch_add(1);
            taskMutex.lock();
            tasks.push({pivotIndex + 1, hi});
            taskMutex.unlock();
            taskCondition.notifyOne();
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
        isDone.acquire();
        signalCompletion();
    }

    void signalCompletion() {
        for (auto &thread : threadPool) {
            thread.requestStop();
        }

        taskMutex.lock();
        for (unsigned int i = 0; i < threadPool.size(); ++i) {
            tasks.push({-1, -1});
        }
        taskMutex.unlock();
        taskCondition.notifyAll();

        for (auto &thread : threadPool) {
            thread.join();
        }
    }
};

#endif // QUICKSORT_H
