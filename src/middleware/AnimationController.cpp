#include "middleware/AnimationController.h"
#include <cmath>

AnimationController::AnimationController()
    : position(0), elapsed(0), speed(1), playing(false)
{
}

void AnimationController::load(const GameSnapshot& initial, const EventBatch& batch,
                               const GameSnapshot& final)
{
    before = initial;
    after = final;
    events = batch;
    position = 0;
    elapsed = 0;
    visual = initial;
    playing = events.getCount() > 0;
    if (!playing)
    {
        visual = after;
    }
    prepare();
}

void AnimationController::prepare()
{
    from = visual;
    to = visual;
    if (position < events.getCount())
    {
        const GameEvent& event = events.get(position);
        if (event.hasFrame)
        {
            to.model = event.frame;
        }
    }
    else
    {
        to = after;
        from = after;
    }
}

void AnimationController::update(double seconds)
{
    if (!playing || seconds < 0 || !std::isfinite(seconds))
    {
        return;
    }
    elapsed += seconds * speed;
    while (elapsed >= 0.20 && position < events.getCount())
    {
        elapsed -= 0.20;
        visual = to;
        position++;
        if (position == events.getCount())
        {
            visual = after;
            playing = false;
            elapsed = 0;
        }
        prepare();
    }
}

void AnimationController::play()
{
    playing = position < events.getCount();
}

void AnimationController::pause()
{
    playing = false;
}

void AnimationController::rebuild(int target)
{
    visual = before;
    position = 0;
    elapsed = 0;
    playing = false;
    while (position < target && position < events.getCount())
    {
        if (events.get(position).hasFrame)
        {
            visual.model = events.get(position).frame;
        }
        position++;
    }
    if (position == events.getCount())
    {
        visual = after;
    }
    prepare();
}

void AnimationController::next()
{
    rebuild(position < events.getCount() ? position + 1 : position);
}

void AnimationController::previous()
{
    rebuild(position > 0 ? position - 1 : 0);
}

void AnimationController::setSpeed(double value)
{
    if (std::isfinite(value) && value >= 0.25 && value <= 4)
    {
        speed = value;
    }
}

bool AnimationController::isPlaying() const
{
    return playing;
}

bool AnimationController::isFinished() const
{
    return position == events.getCount();
}

int AnimationController::getPosition() const
{
    return position;
}

double AnimationController::getProgress() const
{
    return elapsed / 0.20;
}

const GameSnapshot& AnimationController::getVisual() const
{
    return visual;
}

const GameSnapshot& AnimationController::getFrom() const
{
    return from;
}

const GameSnapshot& AnimationController::getTo() const
{
    return to;
}

const GameEvent* AnimationController::getEvent() const
{
    return position < events.getCount() ? &events.get(position) : NULL;
}
