#ifndef VAULT_MAX_HEAP_H
#define VAULT_MAX_HEAP_H

#include "backend/OperationEvent.h"

class MaxHeap
{
private:
    int* arr;
    int heapSize;
    int arraySize;
    int capacity;
    long long comparisonCount = 0;
    long long swapCount = 0;
    BackendEventBatch events;
    void reserve(int required);
    void emit(const std::string& type, int first = -1, int second = -1,
              const std::string& detail = "", bool frame = false);
    void swapCells(int first, int second, const std::string& reason);
    void siftDownSteps(int index);
    void siftUpSteps(int index);
    void compare(int first, int second);

public:
    long long getComparisons() const;
    long long getSwaps() const;
    void resetCounters();
    MaxHeap();
    MaxHeap(const MaxHeap& other);
    MaxHeap& operator=(const MaxHeap& other);
    ~MaxHeap();
    bool buildMaxHeap(const int* values, int count);
    bool loadArray(const int* values, int count);
    bool swapIndices(int first, int second);
    bool sortStep();
    bool extractToSorted();
    bool repairSwap(int first, int second);
    bool rebuildActive();
    std::string serialize() const;
    bool insert(int value);
    bool extractMax(int& value);
    bool siftUp(int index);
    bool siftDown(int index);
    bool heapify(int index);
    bool heapSort();
    bool validate() const;
    int size() const;
    BackendFrame snapshot() const;
    void setEventRecording(bool enabled);
    const BackendEventBatch& getEvents() const;
};

#endif
