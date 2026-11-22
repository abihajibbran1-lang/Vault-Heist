#include "middleware/GameEvent.h"
#include <stdexcept>

GameState::GameState()
    : act(0), level(0), alarmSteps(0), alarmCapacity(0), optimalSteps(0),
      score(0), penalties(0), stars(0), completed(false), failed(false)
{
}

PlayerAction::PlayerAction()
    : value(0), otherValue(0), nodeId(-1), otherNodeId(-1), indexA(-1), indexB(-1)
{
}

OperationResult::OperationResult() : accepted(false), success(false)
{
}

GameEvent::GameEvent()
    : nodeId(-1), otherNodeId(-1), parentId(-1), pivotId(-1), indexA(-1),
      indexB(-1), oldValue(0), newValue(0), oldSize(0), newSize(0), hasFrame(false)
{
}

EventBatch::EventBatch() : events(NULL), count(0)
{
}

EventBatch::EventBatch(const EventBatch& other) : EventBatch()
{
    *this = other;
}

EventBatch& EventBatch::operator=(const EventBatch& other)
{
    if (this != &other)
    {
        GameEvent* replacement = new GameEvent[other.count];
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
    }
    return *this;
}

EventBatch::~EventBatch()
{
    delete[] events;
}

void EventBatch::clear()
{
    delete[] events;
    events = NULL;
    count = 0;
}

void EventBatch::map(const BackendEventBatch& source)
{
    EventBatch replacement;
    replacement.events = new GameEvent[source.getCount()];
    replacement.count = source.getCount();
    for (int i = 0; i < replacement.count; i++)
    {
        const BackendEvent& input = source.get(i);
        GameEvent& output = replacement.events[i];
        output.type = input.type;
        output.description = input.description;
        output.detail = input.detail;
        output.nodeId = input.nodeId;
        output.otherNodeId = input.otherNodeId;
        output.parentId = input.parentId;
        output.pivotId = input.pivotId;
        output.indexA = input.indexA;
        output.indexB = input.indexB;
        output.oldValue = input.oldValue;
        output.newValue = input.newValue;
        output.oldSize = input.oldSize;
        output.newSize = input.newSize;
        output.hasFrame = input.hasFrame;
        output.frame = input.frame;
    }
    *this = replacement;
}

int EventBatch::getCount() const
{
    return count;
}

void EventBatch::append(const EventBatch& source)
{
    int newCount = count + source.count;
    GameEvent* replacement = new GameEvent[newCount];
    try
    {
        for (int i = 0; i < count; i++)
        {
            replacement[i] = events[i];
        }
        for (int i = 0; i < source.count; i++)
        {
            replacement[count + i] = source.events[i];
        }
    }
    catch (...)
    {
        delete[] replacement;
        throw;
    }
    delete[] events;
    events = replacement;
    count = newCount;
}

const GameEvent& EventBatch::get(int index) const
{
    if (index < 0 || index >= count)
    {
        throw std::out_of_range("Playback event index");
    }
    return events[index];
}
