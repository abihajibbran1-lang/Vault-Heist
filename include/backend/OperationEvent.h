#ifndef VAULT_OPERATION_EVENT_H
#define VAULT_OPERATION_EVENT_H

#include <string>

// A backend-owned frame contains values and IDs only, never live node pointers.
struct BackendNodeView
{
    int id;
    int value;
    int parentId;
    int leftId;
    int rightId;
    int height;
    int subtreeSize;
};

class BackendFrame
{
public:
    BackendNodeView* nodes;
    int nodeCount;
    int rootId;
    int* values;
    int arraySize;
    int heapSize;
    BackendFrame();
    BackendFrame(const BackendFrame& other);
    BackendFrame& operator=(const BackendFrame& other);
    ~BackendFrame();
    void clear();
};

struct BackendEvent
{
    std::string type;
    std::string description;
    std::string detail;
    int nodeId;
    int otherNodeId;
    int parentId;
    int pivotId;
    int indexA;
    int indexB;
    int oldValue;
    int newValue;
    int oldSize;
    int newSize;
    bool hasFrame;
    BackendFrame frame;
    BackendEvent();
};

class BackendEventBatch
{
private:
    BackendEvent* events;
    int count;
    int capacity;
    bool enabled;

public:
    BackendEventBatch();
    BackendEventBatch(const BackendEventBatch& other);
    BackendEventBatch& operator=(const BackendEventBatch& other);
    ~BackendEventBatch();
    void clear();
    void setEnabled(bool value);
    bool isEnabled() const;
    void append(const BackendEvent& event);
    int getCount() const;
    const BackendEvent& get(int index) const;
};

// Used for traversal/query results; this is a manually owned array.
class IntBuffer
{
private:
    int* values;
    int count;
    int capacity;

public:
    IntBuffer();
    IntBuffer(const IntBuffer& other);
    IntBuffer& operator=(const IntBuffer& other);
    ~IntBuffer();
    void clear();
    void append(int value);
    int getCount() const;
    int get(int index) const;
};

#endif
