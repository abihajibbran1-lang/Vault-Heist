#include "backend/MaxHeap.h"
#include <sstream>

long long MaxHeap::getComparisons() const
{
    return comparisonCount;
}

long long MaxHeap::getSwaps() const
{
    return swapCount;
}

void MaxHeap::resetCounters()
{
    comparisonCount = 0;
    swapCount = 0;
}

std::string MaxHeap::serialize() const
{
    std::ostringstream output;
    output << arraySize << ' ' << heapSize;
    for (int i = 0; i < arraySize; i++)
    {
        output << ' ' << arr[i];
    }
    return output.str();
}

bool MaxHeap::repairSwap(int first, int second)
{
    events.clear();
    if (first < 0 || second < 0 || first >= heapSize || second >= heapSize || first == second)
    {
        return false;
    }
    int parent = first;
    int child = second;
    if ((first - 1) / 2 == second && first > 0)
    {
        parent = second;
        child = first;
    }
    if (child == 0 || (child - 1) / 2 != parent || arr[parent] >= arr[child])
    {
        return false;
    }
    compare(parent, child);
    swapCells(parent, child, "REPAIR");
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::rebuildActive()
{
    events.clear();
    emit("HEAP_BUILD", -1, -1, "ACTIVE_REGION", true);
    for (int i = heapSize / 2 - 1; i >= 0; i--)
    {
        siftDownSteps(i);
    }
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::extractToSorted()
{
    events.clear();
    if (heapSize == 0 || !validate())
    {
        return false;
    }
    emit("EXTRACT_MAX", 0);
    swapCells(0, heapSize - 1, "EXTRACT_TO_SORTED");
    int oldSize = heapSize;
    heapSize--;
    if (events.isEnabled())
    {
        BackendEvent event;
        event.type = "HEAP_SIZE_CHANGE";
        event.oldSize = oldSize;
        event.newSize = heapSize;
        event.hasFrame = true;
        event.frame = snapshot();
        events.append(event);
    }
    emit("MARK_SORTED", heapSize, -1, "", true);
    if (heapSize == 0 && validate())
    {
        emit("HEAP_SORT_COMPLETE", -1, -1, "", true);
    }
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::loadArray(const int* values, int count)
{
    if (count < 0 || (count > 0 && values == NULL))
    {
        return false;
    }
    reserve(count);
    for (int i = 0; i < count; i++)
    {
        arr[i] = values[i];
    }
    arraySize = count;
    heapSize = count;
    events.clear();
    return true;
}

bool MaxHeap::swapIndices(int first, int second)
{
    events.clear();
    if (first < 0 || second < 0 || first >= heapSize || second >= heapSize || first == second)
    {
        return false;
    }
    swapCells(first, second, "MANUAL");
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::sortStep()
{
    events.clear();
    if (heapSize <= 0 || !validate())
    {
        return false;
    }
    emit("EXTRACT_MAX", 0);
    swapCells(0, heapSize - 1, "SORT");
    int previousSize = heapSize;
    heapSize--;
    if (events.isEnabled())
    {
        BackendEvent event;
        event.type = "HEAP_SIZE_CHANGE";
        event.oldSize = previousSize;
        event.newSize = heapSize;
        event.hasFrame = true;
        event.frame = snapshot();
        events.append(event);
    }
    emit("MARK_SORTED", heapSize, -1, "", true);
    if (heapSize > 0)
    {
        siftDownSteps(0);
    }
    else
    {
        emit("HEAP_SORT_COMPLETE", -1, -1, "", true);
    }
    emit("DONE", -1, -1, "", true);
    return true;
}

MaxHeap::MaxHeap() : arr(NULL), heapSize(0), arraySize(0), capacity(0)
{
}

MaxHeap::MaxHeap(const MaxHeap& other) : MaxHeap()
{
    *this = other;
}

MaxHeap& MaxHeap::operator=(const MaxHeap& other)
{
    if (this != &other)
    {
        int* replacement = new int[other.arraySize];
        for (int i = 0; i < other.arraySize; i++)
        {
            replacement[i] = other.arr[i];
        }
        try
        {
            events = other.events;
        }
        catch (...)
        {
            delete[] replacement;
            throw;
        }
        delete[] arr;
        arr = replacement;
        arraySize = other.arraySize;
        heapSize = other.heapSize;
        capacity = other.arraySize;
        comparisonCount = other.comparisonCount;
        swapCount = other.swapCount;
    }
    return *this;
}

MaxHeap::~MaxHeap()
{
    delete[] arr;
}

void MaxHeap::reserve(int required)
{
    if (required > capacity)
    {
        int newCapacity = capacity == 0 ? 8 : capacity * 2;
        if (newCapacity < required)
        {
            newCapacity = required;
        }
        int* replacement = new int[newCapacity];
        for (int i = 0; i < arraySize; i++)
        {
            replacement[i] = arr[i];
        }
        delete[] arr;
        arr = replacement;
        capacity = newCapacity;
    }
}

BackendFrame MaxHeap::snapshot() const
{
    BackendFrame frame;
    frame.arraySize = arraySize;
    frame.heapSize = heapSize;
    frame.values = new int[arraySize];
    for (int i = 0; i < arraySize; i++)
    {
        frame.values[i] = arr[i];
    }
    return frame;
}

void MaxHeap::emit(const std::string& type, int first, int second,
                   const std::string& detail, bool frame)
{
    if (events.isEnabled())
    {
        BackendEvent event;
        event.type = type;
        event.indexA = first;
        event.indexB = second;
        event.detail = detail;
        event.oldSize = heapSize;
        event.newSize = heapSize;
        if (first >= 0 && first < arraySize)
        {
            event.newValue = arr[first];
        }
        event.hasFrame = frame;
        if (frame)
        {
            event.frame = snapshot();
        }
        events.append(event);
    }
}

void MaxHeap::compare(int first, int second)
{
    comparisonCount++;
    if (events.isEnabled())
    {
        BackendEvent event;
        event.type = "COMPARE";
        event.indexA = first;
        event.indexB = second;
        event.oldValue = arr[first];
        event.newValue = arr[second];
        event.detail = arr[second] > arr[first] ? "GREATER" :
                       (arr[second] < arr[first] ? "LESS" : "EQUAL");
        events.append(event);
    }
}

void MaxHeap::swapCells(int first, int second, const std::string& reason)
{
    swapCount++;
    int temporary = arr[first];
    arr[first] = arr[second];
    arr[second] = temporary;
    emit("HEAP_SWAP", first, second, reason, true);
}

void MaxHeap::siftDownSteps(int index)
{
    emit("HEAPIFY_START", index);
    while (index < heapSize)
    {
        int highest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        if (left < heapSize)
        {
            compare(highest, left);
            if (arr[left] > arr[highest])
            {
                highest = left;
            }
        }
        if (right < heapSize)
        {
            compare(highest, right);
            if (arr[right] > arr[highest])
            {
                highest = right;
            }
        }
        if (highest == index)
        {
            break;
        }
        swapCells(index, highest, "SIFT_DOWN");
        index = highest;
    }
    emit("HEAPIFY_DONE", index);
}

void MaxHeap::siftUpSteps(int index)
{
    while (index > 0)
    {
        int parent = (index - 1) / 2;
        compare(parent, index);
        if (arr[parent] >= arr[index])
        {
            break;
        }
        swapCells(parent, index, "SIFT_UP");
        index = parent;
    }
}

bool MaxHeap::buildMaxHeap(const int* values, int count)
{
    events.clear();
    if (count < 0 || (count > 0 && values == NULL))
    {
        return false;
    }
    reserve(count);
    for (int i = 0; i < count; i++)
    {
        arr[i] = values[i];
    }
    heapSize = count;
    arraySize = count;
    emit("HEAP_BUILD", -1, -1, "", true);
    for (int i = heapSize / 2 - 1; i >= 0; i--)
    {
        siftDownSteps(i);
    }
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::insert(int value)
{
    events.clear();
    // A sorted suffix must be reset/rebuilt explicitly before inserting.
    if (arraySize != heapSize)
    {
        return false;
    }
    reserve(arraySize + 1);
    arr[heapSize++] = value;
    arraySize++;
    emit("INSERT", heapSize - 1, -1, "", true);
    siftUpSteps(heapSize - 1);
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::extractMax(int& value)
{
    events.clear();
    if (heapSize == 0)
    {
        emit("DONE", -1, -1, "Empty heap");
        return false;
    }
    value = arr[0];
    emit("EXTRACT_MAX", 0);
    swapCells(0, heapSize - 1, "EXTRACT");
    int previousSize = heapSize;
    heapSize--;
    arraySize = heapSize;
    if (events.isEnabled())
    {
        BackendEvent event;
        event.type = "HEAP_SIZE_CHANGE";
        event.oldSize = previousSize;
        event.newSize = heapSize;
        event.hasFrame = true;
        event.frame = snapshot();
        events.append(event);
    }
    if (heapSize > 0)
    {
        siftDownSteps(0);
    }
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::siftUp(int index)
{
    events.clear();
    if (index < 0 || index >= heapSize)
    {
        return false;
    }
    siftUpSteps(index);
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::siftDown(int index)
{
    events.clear();
    if (index < 0 || index >= heapSize)
    {
        return false;
    }
    siftDownSteps(index);
    emit("DONE", -1, -1, "", true);
    return true;
}

bool MaxHeap::heapify(int index)
{
    return siftDown(index);
}

bool MaxHeap::heapSort()
{
    events.clear();
    // Rebuild all cells so sorting is repeatable, including an existing suffix.
    heapSize = arraySize;
    emit("HEAP_BUILD", -1, -1, "", true);
    for (int i = heapSize / 2 - 1; i >= 0; i--)
    {
        siftDownSteps(i);
    }
    while (heapSize > 0)
    {
        emit("EXTRACT_MAX", 0);
        swapCells(0, heapSize - 1, "SORT");
        int previousSize = heapSize;
        heapSize--;
        if (events.isEnabled())
        {
            BackendEvent event;
            event.type = "HEAP_SIZE_CHANGE";
            event.oldSize = previousSize;
            event.newSize = heapSize;
            event.hasFrame = true;
            event.frame = snapshot();
            events.append(event);
        }
        emit("MARK_SORTED", heapSize, -1, "", true);
        if (heapSize > 0)
        {
            siftDownSteps(0);
        }
    }
    emit("HEAP_SORT_COMPLETE", -1, -1, "", true);
    emit("DONE");
    return true;
}

bool MaxHeap::validate() const
{
    if (heapSize < 0 || heapSize > arraySize || arraySize > capacity)
    {
        return false;
    }
    for (int i = 1; i < heapSize; i++)
    {
        if (arr[(i - 1) / 2] < arr[i])
        {
            return false;
        }
    }
    for (int i = heapSize + 1; i < arraySize; i++)
    {
        if (arr[i - 1] > arr[i])
        {
            return false;
        }
    }
    return heapSize == 0 || heapSize == arraySize || arr[0] <= arr[heapSize];
}

int MaxHeap::size() const
{
    return heapSize;
}

void MaxHeap::setEventRecording(bool enabled)
{
    events.setEnabled(enabled);
}

const BackendEventBatch& MaxHeap::getEvents() const
{
    return events;
}
