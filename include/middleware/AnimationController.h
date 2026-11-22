#ifndef VAULT_ANIMATION_CONTROLLER_H
#define VAULT_ANIMATION_CONTROLLER_H

#include "middleware/GameEvent.h"

class AnimationController
{
private:
    EventBatch events;
    GameSnapshot before;
    GameSnapshot after;
    GameSnapshot visual;
    GameSnapshot from;
    GameSnapshot to;
    int position;
    double elapsed;
    double speed;
    bool playing;
    void prepare();
    void rebuild(int target);

public:
    AnimationController();
    void load(const GameSnapshot& initial, const EventBatch& batch, const GameSnapshot& final);
    void update(double seconds);
    void play();
    void pause();
    void next();
    void previous();
    void setSpeed(double value);
    bool isPlaying() const;
    bool isFinished() const;
    int getPosition() const;
    double getProgress() const;
    const GameSnapshot& getVisual() const;
    const GameSnapshot& getFrom() const;
    const GameSnapshot& getTo() const;
    const GameEvent* getEvent() const;
};

#endif
