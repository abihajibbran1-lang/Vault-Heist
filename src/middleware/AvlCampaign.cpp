#include "middleware/GameController.h"

class AvlLevelTools
{
public:
    static const BackendNodeView* node(const BackendFrame& frame, int id)
    {
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (frame.nodes[i].id == id)
            {
                return &frame.nodes[i];
            }
        }
        return NULL;
    }

    static bool run(BackendAdapter& backend, const PlayerAction& action, OperationResult& result)
    {
        bool success = backend.execute(action, result.message);
        result.events.append(backend.getEvents());
        return success;
    }
};

bool GameController::loadAvlRound(int round)
{
    IntBuffer values;
    IntBuffer inorder;
    int first = round * campaignCaseSize;
    if (campaignCaseSize <= 0 || first + campaignCaseSize > campaignValues.getCount())
    {
        return false;
    }
    for (int i = first; i < first + campaignCaseSize; i++)
    {
        values.append(campaignValues.get(i));
        if (campaignInorder.getCount() > i)
        {
            inorder.append(campaignInorder.get(i));
        }
    }
    bool raw = campaignKind != "AVL_PREDICT";
    state.choice.clear();
    state.answers.clear();
    state.selectedId = -1;
    return backend.load("AVLTree", values, inorder, raw) && backend.validateStructure();
}

void GameController::avlCampaignAction(const PlayerAction& action, OperationResult& result, bool& complete)
{
    const BackendFrame& frame = snapshot.model;
    const BackendNodeView* root = AvlLevelTools::node(frame, frame.rootId);
    PlayerAction operation;
    operation.name = "SELECT_NODE";
    operation.nodeId = action.nodeId;
    if (campaignKind == "AVL_BADGES" && action.name == "SELECT_NODE")
    {
        result.success = false;
        for (int i = 0; i < campaignRoute.getCount(); i++)
        {
            if (campaignRoute.get(i) == action.nodeId)
            {
                result.success = true;
            }
        }
        for (int i = 0; i < state.answers.getCount(); i++)
        {
            if (state.answers.get(i) == action.nodeId)
            {
                result.success = false;
            }
        }
        AvlLevelTools::run(backend, operation, result);
        if (result.success)
        {
            state.answers.append(action.nodeId);
            complete = state.answers.getCount() == campaignRoute.getCount();
        }
        result.message = result.success ? "Unbalanced room identified" : "That room is balanced or already selected";
        return;
    }
    if (campaignKind == "AVL_SINGLE" || campaignKind == "AVL_DOUBLE")
    {
        if (action.name == "SELECT_PIVOT" || action.name == "SELECT_NODE")
        {
            if (AvlLevelTools::node(frame, action.nodeId) == NULL)
            {
                result.accepted = false;
                return;
            }
            state.selectedId = action.nodeId;
            result.success = true;
            result.message = "Pivot selected; choose the rotation";
            return;
        }
        std::string required = backend.rotationCase(frame.rootId);
        if (action.name == "ROTATION_CASE" && state.choice.empty())
        {
            if (state.selectedId == -1)
            {
                result.accepted = false;
                result.message = "Drag a subtree onto its pivot first";
                return;
            }
            result.success = state.selectedId == frame.rootId && action.text == required;
            if (!result.success)
            {
                result.message = "Wrong pivot/case: +2 alarm, -50 points";
                return;
            }
            operation.nodeId = state.selectedId;
            AvlLevelTools::run(backend, operation, result);
            if (campaignKind == "AVL_DOUBLE")
            {
                state.choice = required;
                state.selectedId = -1;
                result.message = "Case identified. Perform each primitive rotation separately.";
                return;
            }
            operation.name = "ROTATE";
            operation.text = required;
            result.success = AvlLevelTools::run(backend, operation, result) && backend.validate();
        }
        else if (action.name == "ROTATE_PRIMITIVE" && !state.choice.empty())
        {
            bool first = state.answers.getCount() == 0;
            bool lr = state.choice == "LR";
            int pivot = first ? (lr ? root->leftId : root->rightId) : root->id;
            std::string direction = first ? (lr ? "LEFT" : "RIGHT") : (lr ? "RIGHT" : "LEFT");
            result.success = state.selectedId == pivot && action.text == direction;
            if (!result.success)
            {
                result.message = "Wrong primitive rotation: +2 alarm, -50 points";
                return;
            }
            operation.nodeId = pivot;
            AvlLevelTools::run(backend, operation, result);
            operation.name = "ROTATE_PRIMITIVE";
            operation.text = state.choice + "_" + direction;
            result.success = AvlLevelTools::run(backend, operation, result) && backend.validateStructure();
            if (!result.success)
            {
                result.accepted = false;
                return;
            }
            state.answers.append(pivot);
            state.selectedId = -1;
            if (state.answers.getCount() == 1)
            {
                result.message = "First rotation complete. Now repair the original root.";
                return;
            }
            result.success = backend.validate();
        }
        else
        {
            result.accepted = false;
            return;
        }
        if (result.success)
        {
            state.stage++;
            state.cursor++;
            complete = state.stage == campaignValues.getCount() / campaignCaseSize;
            if (!complete && !loadAvlRound(state.stage))
            {
                result.accepted = false;
            }
        }
        return;
    }
    if (campaignKind == "AVL_PREDICT" && action.name == "ROTATION_CASE")
    {
        int key = campaignTargets.get(state.stage);
        result.success = action.text == backend.predict("INSERT", key);
        if (!result.success)
        {
            result.message = "Wrong prediction: +2 alarm, -50 points";
            return;
        }
        operation.name = "INSERT";
        operation.value = key;
        result.success = AvlLevelTools::run(backend, operation, result) && backend.validate();
        if (!result.success)
        {
            result.accepted = false;
            return;
        }
        state.stage++;
        state.cursor++;
        complete = state.stage == campaignTargets.getCount();
        if (!complete && !loadAvlRound(state.stage))
        {
            result.accepted = false;
        }
        return;
    }
    if (campaignKind == "AVL_ORDER")
    {
        if (state.stage == 0 && action.name == "SELECT_NODE")
        {
            AvlLevelTools::run(backend, operation, result);
            result.success = action.nodeId == campaignRoute.get(state.cursor);
            if (result.success)
            {
                state.cursor++;
                if (state.cursor == campaignRoute.getCount())
                {
                    state.stage = 1;
                }
            }
            return;
        }
        if (state.stage == 1 && action.name == "SUBMIT")
        {
            result.success = action.value == backend.query("RANK_VALUE", campaignTargets.get(1)).get(0);
            operation.name = "RANK";
            operation.value = campaignTargets.get(1);
            AvlLevelTools::run(backend, operation, result);
            if (result.success)
            {
                state.stage = 2;
            }
            return;
        }
        if (state.stage == 2 && action.name == "SUBMIT")
        {
            operation.nodeId = frame.rootId;
            AvlLevelTools::run(backend, operation, result);
            result.success = action.value == root->subtreeSize && backend.validate();
            complete = result.success;
            return;
        }
    }
    result.accepted = false;
    result.message = "Use the operation requested by this AVL objective";
}
