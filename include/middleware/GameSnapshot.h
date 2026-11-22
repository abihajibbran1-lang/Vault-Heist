#ifndef VAULT_GAME_SNAPSHOT_H
#define VAULT_GAME_SNAPSHOT_H

#include "backend/OperationEvent.h"

struct GameSnapshot
{
    std::string structure;
    BackendFrame model;
};

struct GameState
{
    std::string mode;
    std::string status;
    std::string message;
    std::string objective;
    int act;
    int level;
    int alarmSteps;
    int alarmCapacity;
    int optimalSteps;
    int score;
    int penalties;
    int stars;
    int cursor = 0;
    bool hinted = false;
    int stage = 0;
    int selectedId = -1;
    IntBuffer answers;
    std::string choice;
    int pointPenalties = 0;
    bool completed;
    bool failed;
    GameState();
};

struct PlayerAction
{
    std::string name;
    std::string text;
    int value;
    int otherValue;
    int nodeId;
    int otherNodeId;
    int indexA;
    int indexB;
    PlayerAction();
};

#endif
