#include "middleware/GameController.h"

bool GameController::heapReady() const
{
    return snapshot.structure == "MaxHeap" && backend.validate();
}

bool GameController::loadHeapRound(int round)
{
    if (campaignCaseSize <= 0 || (round + 1) * campaignCaseSize > campaignValues.getCount())
    {
        return false;
    }
    IntBuffer input;
    IntBuffer empty;
    for (int i = 0; i < campaignCaseSize; i++)
    {
        input.append(campaignValues.get(round * campaignCaseSize + i));
    }
    state.answers.clear();
    state.selectedId = -1;
    return backend.load("MaxHeap", input, empty, true);
}

void GameController::heapCampaignAction(const PlayerAction& action, OperationResult& result, bool& complete)
{
    const BackendFrame& before = snapshot.model;
    if (action.name == "SELECT_HEAP_INDEX")
    {
        int index = action.nodeId;
        if (index < 0 || index >= before.heapSize)
        {
            result.accepted = false;
            result.message = "Select an active cell; sorted cells are locked";
            return;
        }
        if (state.answers.getCount() == 2)
        {
            state.answers.clear();
        }
        state.answers.append(index);
        state.selectedId = index;
        result.success = true;
        result.message = "Selected heap index " + std::to_string(index);
        return;
    }
    if ((campaignKind == "HEAP_BUILD" || campaignKind == "HEAP_REPAIR") && action.name == "EXTRACT_SORTED")
    {
        result.accepted = false;
        result.message = "Restore the heap in this level; extraction comes next";
        return;
    }
    if (campaignKind == "HEAP_REPAIR" && action.name == "BUILD_ACTIVE_HEAP")
    {
        result.accepted = false;
        result.message = "Use sift-down, sift-up or repair swaps for this challenge";
        return;
    }
    PlayerAction operation = action;
    operation.indexA = action.text == "INPUT" ? action.value :
                       (action.indexA >= 0 ? action.indexA : (state.selectedId != -1 ? state.selectedId : action.value));
    operation.indexB = action.indexB >= 0 ? action.indexB : action.otherValue;
    if (action.name == "REPAIR_SWAP" && state.answers.getCount() == 2 && action.text != "INPUT")
    {
        operation.indexA = state.answers.get(0);
        operation.indexB = state.answers.get(1);
    }
    result.success = backend.execute(operation, result.message);
    result.events = backend.getEvents();
    GameSnapshot after = backend.snapshot();
    bool changed = before.heapSize != after.model.heapSize;
    for (int i = 0; i < before.arraySize && i < after.model.arraySize; i++)
    {
        changed = changed || before.values[i] != after.model.values[i];
    }
    if (result.success && !backend.validate() && !changed)
    {
        result.success = false;
        result.message = "That step did not repair the disturbed heap";
    }
    if (result.success)
    {
        state.selectedId = -1;
        state.answers.clear();
        if (action.name == "EXTRACT_SORTED")
        {
            state.cursor++;
        }
        if (campaignKind == "HEAP_BUILD")
        {
            complete = backend.validate();
        }
        else if (campaignKind == "HEAP_REPAIR" && backend.validate())
        {
            state.stage++;
            complete = state.stage == campaignTargets.getCount();
            if (!complete && !loadHeapRound(state.stage))
            {
                result.accepted = false;
            }
        }
        else
        {
            complete = after.model.heapSize == 0 && backend.validate();
        }
    }
    if (!result.success)
    {
        result.message = "Invalid heap action: +1 alarm. " + result.message;
    }
}
