#include "middleware/GameController.h"
#include "middleware/JsonDocument.h"
#include <cassert>
#include <iostream>
#include <fstream>

class CampaignTests
{
public:
    static int id(const BackendFrame& frame, int key)
    {
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (frame.nodes[i].value == key)
            {
                return frame.nodes[i].id;
            }
        }
        assert(false);
        return -1;
    }

    static int value(const BackendFrame& frame, int nodeId, bool height = false)
    {
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (frame.nodes[i].id == nodeId)
            {
                return height ? frame.nodes[i].height : frame.nodes[i].value;
            }
        }
        assert(false);
        return 0;
    }

    static void act(GameController& game, const std::string& name, int node = -1, int answer = 0,
                    const std::string& text = "", int other = 0)
    {
        PlayerAction action;
        action.name = name;
        action.nodeId = node;
        action.value = answer;
        action.otherValue = other;
        action.text = text;
        OperationResult result = game.performAction(action);
        if (!result.accepted || !result.success)
        {
            std::cerr << game.getState().act << "." << game.getState().level << " " << name << ": " << result.message << "\n";
        }
        assert(result.accepted && result.success);
        if (name == "EXTRACT_SORTED" && result.after.model.heapSize == 0)
        {
            int completedEvents = 0;
            for (int i = 0; i < result.events.getCount(); i++)
            {
                completedEvents += result.events.get(i).type == "HEAP_SORT_COMPLETE" ? 1 : 0;
            }
            assert(completedEvents == 1);
        }
        game.update(100);
    }

    static void failAndRetry(GameController& game)
    {
        std::string kind = game.getCampaignKind();
        if (kind == "AVL_SINGLE" || kind == "AVL_DOUBLE")
        {
            act(game, "SELECT_PIVOT", game.getSnapshot().model.rootId);
        }
        PlayerAction wrong;
        wrong.name = "SELECT_NODE";
        wrong.nodeId = game.getSnapshot().model.rootId;
        if (kind.find("HEAP_") == 0)
        {
            wrong.name = "REPAIR_SWAP";
            wrong.value = 0;
            wrong.otherValue = 0;
        }
        if (kind == "AVL_SINGLE" || kind == "AVL_DOUBLE" || kind == "AVL_PREDICT")
        {
            wrong.name = "ROTATION_CASE";
            wrong.text = "NONE";
        }
        if (kind == "TRAVERSAL")
        {
            wrong.nodeId = game.getSnapshot().model.nodes[1].id;
        }
        else if (kind == "RECONSTRUCT")
        {
            wrong.name = "PLACE_TOKEN";
            wrong.nodeId = game.blueprint().model.rootId;
            wrong.value = 99999;
        }
        else if (kind == "HEIGHTS" || kind == "RANGE")
        {
            wrong.name = "SUBMIT";
            wrong.value = 99999;
            wrong.otherValue = 99999;
        }
        else if (kind == "SEARCH" || kind == "INSERT_PATH")
        {
            wrong.name = "DIRECTION";
            wrong.text = "FOUND";
        }
        else if (kind == "DELETE_CASE")
        {
            wrong.name = "DELETE_CASE";
            wrong.text = "TWO_CHILDREN"; // The first target is a leaf.
        }
        int attempts = 0;
        while (!game.getState().failed && attempts++ < 200)
        {
            int beforeAlarm = game.getState().alarmSteps;
            game.performAction(wrong);
            game.update(100);
            if (attempts == 1 && (kind == "AVL_SINGLE" || kind == "AVL_DOUBLE" || kind == "AVL_PREDICT"))
            {
                assert(game.getState().alarmSteps == beforeAlarm + 2);
                assert(game.getState().pointPenalties == 50 && game.getState().score == 930);
            }
        }
        assert(game.getState().failed && game.getState().stars == 0);
        assert(game.resetLevel() && !game.getState().failed && game.getState().cursor == 0);
    }

    static void solveAvl(GameController& game, const JsonDocument& data, const IntBuffer& values,
                         const IntBuffer& inorder, const IntBuffer& targets)
    {
        const std::string kind = game.getCampaignKind();
        int width = 0;
        data.integer("caseSize", width);
        if (kind == "AVL_BADGES")
        {
            GameSnapshot frame = game.getSnapshot();
            for (int i = 0; i < frame.model.nodeCount; i++)
            {
                int left = 0;
                int right = 0;
                for (int j = 0; j < frame.model.nodeCount; j++)
                {
                    if (frame.model.nodes[j].id == frame.model.nodes[i].leftId)
                    {
                        left = frame.model.nodes[j].height;
                    }
                    if (frame.model.nodes[j].id == frame.model.nodes[i].rightId)
                    {
                        right = frame.model.nodes[j].height;
                    }
                }
                if (left - right > 1 || left - right < -1)
                {
                    act(game, "SELECT_NODE", frame.model.nodes[i].id);
                }
            }
        }
        else if (kind == "AVL_SINGLE" || kind == "AVL_DOUBLE" || kind == "AVL_PREDICT")
        {
            for (int round = 0; round < values.getCount() / width; round++)
            {
                IntBuffer input;
                IntBuffer order;
                for (int i = 0; i < width; i++)
                {
                    input.append(values.get(round * width + i));
                    if (inorder.getCount() > round * width + i)
                    {
                        order.append(inorder.get(round * width + i));
                    }
                }
                BackendAdapter reference;
                assert(reference.load("AVLTree", input, order, kind != "AVL_PREDICT"));
                GameSnapshot original = reference.snapshot();
                int pivot = original.model.rootId;
                if (kind == "AVL_PREDICT")
                {
                    act(game, "ROTATION_CASE", -1, 0, reference.predict("INSERT", targets.get(round)));
                }
                else
                {
                    std::string required = reference.rotationCase(pivot);
                    act(game, "SELECT_PIVOT", pivot);
                    act(game, "ROTATION_CASE", -1, 0, required);
                    if (kind == "AVL_DOUBLE")
                    {
                        bool lr = required == "LR";
                        int child = lr ? original.model.nodes[0].leftId : original.model.nodes[0].rightId;
                        act(game, "SELECT_PIVOT", child);
                        if (round == 0)
                        {
                            PlayerAction wrongPrimitive;
                            wrongPrimitive.name = "ROTATE_PRIMITIVE";
                            wrongPrimitive.text = lr ? "RIGHT" : "LEFT";
                            int alarm = game.getState().alarmSteps;
                            assert(!game.performAction(wrongPrimitive).success);
                            assert(game.getState().alarmSteps == alarm + 2 && game.getState().pointPenalties == 50);
                            assert(game.undo() && game.getState().pointPenalties == 0);
                        }
                        act(game, "ROTATE_PRIMITIVE", -1, 0, lr ? "LEFT" : "RIGHT");
                        assert(game.getSnapshot().model.nodes[0].height == 3);
                        assert(game.getSnapshot().model.nodes[0].subtreeSize == 3);
                        if (round == 0)
                        {
                            assert(game.getState().answers.getCount() == 1);
                            assert(game.saveCampaign());
                            assert(game.resumeCampaign());
                            assert(game.getState().choice == required && game.getState().answers.getCount() == 1);
                        }
                        act(game, "SELECT_PIVOT", pivot);
                        act(game, "ROTATE_PRIMITIVE", -1, 0, lr ? "RIGHT" : "LEFT");
                    }
                }
            }
            GameSnapshot final = game.getSnapshot();
            assert(final.model.nodeCount == 3 && final.model.nodes[0].height == 2 && final.model.nodes[0].subtreeSize == 3);
        }
        else
        {
            BackendAdapter reference;
            assert(reference.load("AVLTree", values, inorder, false));
            IntBuffer path = reference.query("KTH_PATH", targets.get(0));
            for (int i = 0; i < path.getCount(); i++)
            {
                act(game, "SELECT_NODE", path.get(i));
            }
            act(game, "SUBMIT", -1, reference.query("RANK_VALUE", targets.get(1)).get(0));
            act(game, "SUBMIT", -1, values.getCount());
        }
    }

    static void solveHeap(GameController& game, const IntBuffer& values)
    {
        if (game.getCampaignKind() == "HEAP_REPAIR")
        {
            act(game, "SELECT_HEAP_INDEX", 0);
            act(game, "HEAPIFY");
            assert(game.getState().stage == 1);
            assert(game.saveCampaign() && game.resumeCampaign());
            assert(game.getState().stage == 1 && game.getSnapshot().model.heapSize == 7);
            act(game, "SELECT_HEAP_INDEX", 6);
            act(game, "SIFT_UP");
            return;
        }
        BackendAdapter reference;
        IntBuffer empty;
        assert(reference.load("MaxHeap", values, empty, true));
        PlayerAction operation;
        std::string ignored;
        if (!reference.validate())
        {
            act(game, "BUILD_ACTIVE_HEAP");
            operation.name = "BUILD_ACTIVE_HEAP";
            assert(reference.execute(operation, ignored) && reference.validate());
        }
        if (game.getCampaignKind() == "HEAP_BUILD")
        {
            return;
        }
        int extracted = 0;
        while (reference.snapshot().model.heapSize > 0)
        {
            GameSnapshot before = reference.snapshot();
            int maximum = before.model.values[0];
            for (int i = 0; i < before.model.heapSize; i++)
            {
                assert(maximum >= before.model.values[i]);
            }
            act(game, "EXTRACT_SORTED");
            operation.name = "EXTRACT_SORTED";
            assert(reference.execute(operation, ignored));
            GameSnapshot after = game.getSnapshot();
            assert(after.model.arraySize == values.getCount());
            assert(after.model.heapSize == before.model.heapSize - 1);
            assert(after.model.values[after.model.heapSize] == maximum);
            extracted++;
            if (!reference.validate())
            {
                if (extracted == 1)
                {
                    PlayerAction invalid;
                    invalid.name = "EXTRACT_SORTED";
                    assert(!game.performAction(invalid).success);
                    assert(game.undo());
                    assert(game.saveCampaign());
                    bool resumed = game.resumeCampaign();
                    if (!resumed)
                    {
                        std::cerr << "Heap checkpoint " << game.getState().level << ": " << game.getState().message << '\n';
                    }
                    assert(resumed);
                    assert(game.getSnapshot().model.heapSize == after.model.heapSize);
                    for (int i = 0; i < after.model.arraySize; i++)
                    {
                        assert(game.getSnapshot().model.values[i] == after.model.values[i]);
                    }
                }
                act(game, "HEAPIFY", -1, 0);
                operation.name = "HEAPIFY";
                operation.indexA = 0;
                assert(reference.execute(operation, ignored) && reference.validate());
            }
            GameSnapshot current = reference.snapshot();
            for (int i = 0; i < current.model.arraySize; i++)
            {
                assert(current.model.values[i] == game.getSnapshot().model.values[i]);
            }
        }
        const BackendFrame& sorted = game.getSnapshot().model;
        for (int i = 1; i < sorted.arraySize; i++)
        {
            assert(sorted.values[i - 1] <= sorted.values[i]);
        }
        assert(game.getState().cursor == values.getCount());
    }

    static void solve(GameController& game, int actNumber, int level)
    {
        JsonDocument data;
        assert(data.load("data/levels/act" + std::to_string(actNumber) + "-level" + std::to_string(level) + ".json"));
        IntBuffer values;
        IntBuffer inorder;
        IntBuffer targets;
        assert(data.array("values", values) && data.array("inorder", inorder) && data.array("targets", targets));
        if (actNumber == 4)
        {
            solveHeap(game, values);
            assert(game.getState().completed && game.getState().stars == 3);
            assert(game.getState().alarmSteps == game.getState().optimalSteps);
            return;
        }
        if (actNumber == 3)
        {
            solveAvl(game, data, values, inorder, targets);
            assert(game.getState().completed && game.getState().stars == 3);
            assert(game.getState().alarmSteps == game.getState().optimalSteps);
            return;
        }
        BackendAdapter model;
        assert(model.load(actNumber == 1 ? "BinaryTree" : "BST", values, inorder, false));
        GameSnapshot original = model.snapshot();
        std::string kind = game.getCampaignKind();
        if (kind == "TRAVERSAL")
        {
            const char* orders[4] = {"PREORDER", "INORDER", "POSTORDER", "LEVELORDER"};
            for (int round = 0; round < 4; round++)
            {
                IntBuffer route = model.query(orders[round]);
                for (int i = 0; i < route.getCount(); i++)
                {
                    act(game, "SELECT_NODE", route.get(i));
                }
            }
        }
        else if (kind == "RECONSTRUCT")
        {
            IntBuffer route = model.query("PREORDER");
            for (int round = 0; round < 2; round++)
            {
                for (int i = 0; i < route.getCount(); i++)
                {
                    act(game, "PLACE_TOKEN", route.get(i), value(original.model, route.get(i)));
                }
            }
            assert(game.getSnapshot().model.nodeCount == original.model.nodeCount);
        }
        else if (kind == "HEIGHTS")
        {
            IntBuffer route = model.query("POSTORDER");
            for (int i = 0; i < route.getCount(); i++)
            {
                act(game, "SELECT_NODE", route.get(i));
                act(game, "SUBMIT", -1, value(original.model, route.get(i), true));
            }
            act(game, "SUBMIT", -1, model.query("DIAMETER").get(0));
        }
        else if (kind == "LCA_PATH")
        {
            int first = id(original.model, targets.get(0));
            int second = id(original.model, targets.get(1));
            act(game, "SELECT_NODE", first);
            act(game, "SELECT_NODE", second);
            act(game, "SELECT_NODE", model.query("LCA", first, second).get(0));
            IntBuffer route = model.query("PATH_BETWEEN", first, second);
            for (int i = 0; i < route.getCount(); i++)
            {
                act(game, "SELECT_NODE", route.get(i));
            }
            act(game, "SUBMIT", -1, route.getCount() - 1);
        }
        else if (kind == "MIRROR")
        {
            IntBuffer route = model.query("POSTORDER");
            for (int i = 0; i < route.getCount(); i++)
            {
                act(game, "SELECT_NODE", route.get(i));
            }
        }
        else if (kind == "SEARCH" || kind == "INSERT_PATH")
        {
            IntBuffer route = model.query("SEARCH_PATH", targets.get(0));
            for (int i = 0; i < route.getCount(); i++)
            {
                int current = value(original.model, route.get(i));
                std::string direction = targets.get(0) < current ? "LEFT" : (targets.get(0) > current ? "RIGHT" : "FOUND");
                act(game, "DIRECTION", -1, 0, direction);
            }
            if (kind == "INSERT_PATH")
            {
                assert(game.getSnapshot().model.nodeCount == original.model.nodeCount + 1);
            }
        }
        else if (kind == "DELETE_CASE")
        {
            for (int i = 0; i < targets.getCount(); i++)
            {
                GameSnapshot current = model.snapshot();
                int targetId = id(current.model, targets.get(i));
                int children = 0;
                for (int j = 0; j < current.model.nodeCount; j++)
                {
                    if (current.model.nodes[j].id == targetId)
                    {
                        children = (current.model.nodes[j].leftId != -1) + (current.model.nodes[j].rightId != -1);
                    }
                }
                act(game, "DELETE_CASE", -1, 0, children == 0 ? "LEAF" : (children == 1 ? "ONE_CHILD" : "TWO_CHILDREN"));
                PlayerAction action;
                action.name = "DELETE";
                action.value = targets.get(i);
                std::string message;
                assert(model.execute(action, message) && model.validate());
            }
            assert(game.getSnapshot().model.nodeCount == original.model.nodeCount - targets.getCount());
        }
        else if (kind == "RANGE")
        {
            act(game, "SUBMIT", -1, targets.get(0), "", targets.get(1));
            IntBuffer route = model.query("RANGE_IDS", targets.get(0), targets.get(1));
            for (int i = route.getCount() - 1; i >= 0; i--)
            {
                act(game, "SELECT_NODE", route.get(i));
            }
        }
        assert(game.getState().completed && !game.getState().failed);
        assert(game.getState().stars == (game.getState().hinted ? 2 : 3));
        assert(game.getState().alarmSteps == game.getState().optimalSteps);
    }
};

int main(int argc, char* argv[])
{
    assert(argc == 2);
    GameController game(argv[1]);
    assert(!game.levelAvailable(1, 2));
    for (int act = 1; act <= 4; act++)
    {
        for (int level = 1; level <= 5; level++)
        {
            assert(game.levelAvailable(act, level));
            assert(game.loadLevel(act, level));
            CampaignTests::failAndRetry(game);
            CampaignTests::solve(game, act, level);
            assert(game.levelStars(act, level) == 3);
            int alarm = game.getState().alarmSteps;
            assert(game.replayLevel());
            game.update(1000);
            assert(game.getState().alarmSteps == alarm && game.getState().stars == 3);
            std::cout << "Campaign " << act << "." << level << ": solve/fail/retry, score and replay passed.\n";
        }
    }
    GameController restored(argv[1]);
    assert(restored.loadLevel(4, 1));
    JsonDocument heapData;
    IntBuffer heapValues;
    assert(heapData.load("data/levels/act4-level1.json") && heapData.array("values", heapValues));
    int* input = new int[heapValues.getCount()];
    for (int i = 0; i < heapValues.getCount(); i++)
    {
        input[i] = heapValues.get(i);
    }
    MaxHeap referenceHeap;
    assert(referenceHeap.buildMaxHeap(input, heapValues.getCount()));
    delete[] input;
    const BackendEventBatch& swaps = referenceHeap.getEvents();
    for (int i = 0; i < swaps.getCount(); i++)
    {
        if (swaps.get(i).type == "HEAP_SWAP")
        {
            CampaignTests::act(restored, "SELECT_HEAP_INDEX", swaps.get(i).indexA);
            CampaignTests::act(restored, "SELECT_HEAP_INDEX", swaps.get(i).indexB);
            CampaignTests::act(restored, "REPAIR_SWAP");
        }
    }
    assert(restored.getState().completed && restored.getState().stars == 3);
    for (int act = 1; act <= 4; act++)
    {
        for (int level = 1; level <= 5; level++)
        {
            assert(restored.levelAvailable(act, level) && restored.levelStars(act, level) == 3);
        }
    }
    assert(restored.loadLevel(1, 5));
    CampaignTests::act(restored, "SELECT_NODE", restored.blueprint().model.nodes[2].id);
    CampaignTests::act(restored, "AUTO");
    assert(restored.getState().completed);
    assert(restored.loadLevel(1, 1));
    PlayerAction click;
    click.name = "SELECT_NODE";
    click.nodeId = restored.getSnapshot().model.rootId;
    assert(restored.performAction(click).success);
    restored.update(10);
    assert(restored.saveCampaign());
    GameController resumed(argv[1]);
    assert(resumed.resumeCampaign());
    assert(resumed.getState().cursor == 1 && resumed.getState().alarmSteps == 1);
    assert(resumed.undo() && resumed.getState().cursor == 0);
    std::ofstream damagedProgress((std::string(argv[1]) + "/campaign-progress.json").c_str());
    damagedProgress << "{}";
    damagedProgress.close();
    GameController recovered(argv[1]);
    assert(!recovered.levelAvailable(1, 2));
    assert(recovered.resumeCampaign());
    assert(recovered.levelAvailable(4, 5) && recovered.levelStars(4, 5) == 3);
    assert(restored.undo() && restored.getState().cursor == 0 && restored.getState().alarmSteps == 0);
    std::string hint;
    assert(restored.requestHint(hint));
    CampaignTests::solve(restored, 1, 1); // Checked separately below for the hint cap.
    return 0;
}
