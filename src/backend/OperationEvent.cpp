#include "backend/OperationEvent.h"
#include <stdexcept>

BackendFrame::BackendFrame()
    : nodes(NULL), nodeCount(0), rootId(-1), values(NULL), arraySize(0), heapSize(0)
{
}

BackendFrame::BackendFrame(const BackendFrame& other) : BackendFrame()
{
    *this = other;
}

BackendFrame& BackendFrame::operator=(const BackendFrame& other)
{
    if (this != &other)
    {
        BackendNodeView* newNodes = new BackendNodeView[other.nodeCount];
        int* newValues = NULL;
        try
        {
            newValues = new int[other.arraySize];
        }
        catch (...)
        {
            delete[] newNodes;
            throw;
        }
        for (int i = 0; i < other.nodeCount; i++)
        {
            newNodes[i] = other.nodes[i];
        }
        for (int i = 0; i < other.arraySize; i++)
        {
            newValues[i] = other.values[i];
        }
        clear();
        nodes = newNodes;
        values = newValues;
        nodeCount = other.nodeCount;
        rootId = other.rootId;
        arraySize = other.arraySize;
        heapSize = other.heapSize;
    }
    return *this;
}

BackendFrame::~BackendFrame()
{
    clear();
}

void BackendFrame::clear()
{
    delete[] nodes;
    delete[] values;
    nodes = NULL;
    values = NULL;
    nodeCount = 0;
    rootId = -1;
    arraySize = 0;
    heapSize = 0;
}

BackendEvent::BackendEvent()
    : nodeId(-1), otherNodeId(-1), parentId(-1), pivotId(-1), indexA(-1),
      indexB(-1), oldValue(0), newValue(0), oldSize(0), newSize(0), hasFrame(false)
{
}

BackendEventBatch::BackendEventBatch()
    : events(NULL), count(0), capacity(0), enabled(true)
{
}

BackendEventBatch::BackendEventBatch(const BackendEventBatch& other)
    : BackendEventBatch()
{
    *this = other;
}

BackendEventBatch& BackendEventBatch::operator=(const BackendEventBatch& other)
{
    if (this != &other)
    {
        BackendEvent* replacement = new BackendEvent[other.count];
        try
        {
            for (int i = 0; i < other.count; i++)
            {
                replacement[i] = other.events[i];
            }
        }
        catch (...)
        {
            delete[] replacement;
            throw;
        }
        delete[] events;
        events = replacement;
        count = other.count;
        capacity = other.count;
        enabled = other.enabled;
    }
    return *this;
}

BackendEventBatch::~BackendEventBatch()
{
    delete[] events;
}

void BackendEventBatch::clear()
{
    // Release frames from the previous operation immediately.
    delete[] events;
    events = NULL;
    count = 0;
    capacity = 0;
}

void BackendEventBatch::setEnabled(bool value)
{
    enabled = value;
    clear();
}

bool BackendEventBatch::isEnabled() const
{
    return enabled;
}

void BackendEventBatch::append(const BackendEvent& event)
{
    if (!enabled)
    {
        return;
    }
    if (count == capacity)
    {
        int newCapacity = capacity == 0 ? 8 : capacity * 2;
        BackendEvent* replacement = new BackendEvent[newCapacity];
        try
        {
            for (int i = 0; i < count; i++)
            {
                replacement[i] = events[i];
            }
        }
        catch (...)
        {
            delete[] replacement;
            throw;
        }
        delete[] events;
        events = replacement;
        capacity = newCapacity;
    }
    events[count] = event;
    count++;
}

int BackendEventBatch::getCount() const
{
    return count;
}

const BackendEvent& BackendEventBatch::get(int index) const
{
    if (index < 0 || index >= count)
    {
        throw std::out_of_range("Event index");
    }
    return events[index];
}

IntBuffer::IntBuffer() : values(NULL), count(0), capacity(0)
{
}

IntBuffer::IntBuffer(const IntBuffer& other) : IntBuffer()
{
    *this = other;
}

IntBuffer& IntBuffer::operator=(const IntBuffer& other)
{
    if (this != &other)
    {
        int* replacement = new int[other.count];
        for (int i = 0; i < other.count; i++)
        {
            replacement[i] = other.values[i];
        }
        delete[] values;
        values = replacement;
        count = other.count;
        capacity = other.count;
    }
    return *this;
}

IntBuffer::~IntBuffer()
{
    delete[] values;
}

void IntBuffer::clear()
{
    count = 0;
}

void IntBuffer::append(int value)
{
    if (count == capacity)
    {
        int newCapacity = capacity == 0 ? 8 : capacity * 2;
        int* replacement = new int[newCapacity];
        for (int i = 0; i < count; i++)
        {
            replacement[i] = values[i];
        }
        delete[] values;
        values = replacement;
        capacity = newCapacity;
    }
    values[count++] = value;
}

int IntBuffer::getCount() const
{
    return count;
}

int IntBuffer::get(int index) const
{
    if (index < 0 || index >= count)
    {
        throw std::out_of_range("Result index");
    }
    return values[index];
}
