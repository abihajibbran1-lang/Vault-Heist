#ifndef VAULT_GAME_EVENT_H
#define VAULT_GAME_EVENT_H

#include "middleware/GameSnapshot.h"

struct GameEvent
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
    GameEvent();
};

class EventBatch
{
private:
    GameEvent* events;
    int count;

public:
    EventBatch();
    EventBatch(const EventBatch& other);
    EventBatch& operator=(const EventBatch& other);
    ~EventBatch();
    void map(const BackendEventBatch& source);
    void append(const EventBatch& source);
    void clear();
    int getCount() const;
    const GameEvent& get(int index) const;
};

struct OperationResult
{
    bool accepted;
    bool success;
    std::string message;
    EventBatch events;
    GameSnapshot before;
    GameSnapshot after;
    GameState gameState;
    OperationResult();
};

#endif
