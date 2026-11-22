#include "middleware/GameController.h"
#include "backend/AVLTree.h"
#include "middleware/JsonDocument.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

class EndlessTests
{
private:
    static void nodeText(const BackendFrame& frame, int id, std::ostream& output)
    {
        if (id == -1)
        {
            output << "# ";
            return;
        }
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (frame.nodes[i].id == id)
            {
                output << id << ' ' << frame.nodes[i].value << ' ';
                nodeText(frame, frame.nodes[i].leftId, output);
                nodeText(frame, frame.nodes[i].rightId, output);
                return;
            }
        }
        assert(false);
    }

public:
    static std::string prediction(GameController& game)
    {
        const BackendFrame& frame = game.getSnapshot().model;
        int nextId = 1;
        for (int i = 0; i < frame.nodeCount; i++)
        {
            if (frame.nodes[i].id >= nextId)
            {
                nextId = frame.nodes[i].id + 1;
            }
        }
        std::ostringstream output;
        output << nextId << ' ';
        nodeText(frame, frame.rootId, output);
        AVLTree preview;
        assert(preview.deserialize(output.str()) && preview.validate());
        const EndlessState& state = game.getEndless();
        if (state.request == "INSERT")
        {
            assert(preview.insert(state.key));
        }
        else
        {
            assert(preview.deleteKey(state.key));
        }
        assert(preview.validate());
        for (int i = 0; i < preview.getEvents().getCount(); i++)
        {
            if (preview.getEvents().get(i).type == "ROTATE")
            {
                return preview.getEvents().get(i).detail;
            }
        }
        return "NONE";
    }

    static void answer(GameController& game)
    {
        PlayerAction action;
        action.name = "SELECT_ROTATION";
        action.text = prediction(game);
        OperationResult result = game.performAction(action);
        assert(result.accepted && result.success);
        assert(game.getEndless().phase == "ANIMATING");
    }

    static void run(const std::string& firstDirectory, const std::string& secondDirectory)
    {
        GameController game(firstDirectory);
        GameController second(secondDirectory);
        assert(game.startEndless(42) && second.startEndless(42));
        assert(game.getSnapshot().model.nodeCount == 7);
        assert(game.getEndless().survivalSeconds == 0 && game.getEndless().completedOperations == 0);
        assert(game.getEndless().phase == "WAITING_FOR_ANSWER");
        assert(game.getEndless().deadline == 8);
        for (int i = 0; i < 6; i++)
        {
            assert(GameController::intervalAt(i * 30.0) == 8 - i);
        }
        assert(GameController::intervalAt(10000) == 3);

        bool sawInsert = false;
        bool sawDelete = false;
        bool sawFull = false;
        bool cases[5] = {};
        std::string names[5] = {"NONE", "LL", "RR", "LR", "RL"};
        for (int round = 0; round < 400; round++)
        {
            const EndlessState current = game.getEndless();
            assert(current.request == second.getEndless().request && current.key == second.getEndless().key);
            assert(current.randomState == second.getEndless().randomState);
            const BackendFrame& frame = game.getSnapshot().model;
            bool contains = false;
            for (int i = 0; i < frame.nodeCount; i++)
            {
                contains = contains || frame.nodes[i].value == current.key;
            }
            assert(current.key >= 1 && current.key <= 999);
            if (frame.nodeCount == 31)
            {
                sawFull = true;
                assert(current.request == "DELETE");
            }
            if (current.request == "INSERT")
            {
                sawInsert = true;
                assert(!contains);
            }
            else
            {
                sawDelete = true;
                assert(contains);
            }
            std::string kind = prediction(game);
            for (int i = 0; i < 5; i++)
            {
                cases[i] = cases[i] || kind == names[i];
            }
            answer(game);
            answer(second);
            double time = game.getEndless().survivalSeconds;
            game.update(100);
            second.update(100);
            assert(game.getEndless().survivalSeconds == time);
            assert(game.getEndless().phase == "WAITING_FOR_NEXT");
            assert(game.getEndless().completedOperations == round + 1);
            double remaining = current.deadline - time;
            game.update(remaining / 2);
            second.update(remaining / 2);
            assert(game.getEndless().phase == "WAITING_FOR_NEXT");
            assert(game.getEndless().deadline == current.deadline);
            game.update(remaining / 2);
            second.update(remaining / 2);
            assert(game.getEndless().phase == "WAITING_FOR_ANSWER");
            assert(game.getEndless().deadline == current.deadline + GameController::intervalAt(current.deadline));
        }
        assert(sawInsert && sawDelete && sawFull);
        for (int i = 0; i < 5; i++)
        {
            assert(cases[i]);
        }
        assert(game.getState().score == 0 && game.getState().stars == 0 && game.getState().alarmSteps == 0);
        double savedTime = game.getEndless().survivalSeconds;
        game.pause();
        game.update(100);
        assert(game.getEndless().survivalSeconds == savedTime);
        assert(game.saveEndless());
        GameController loaded(firstDirectory);
        assert(loaded.resumeEndless());
        assert(loaded.getEndless().phase == "PAUSED");
        loaded.update(50);
        assert(loaded.getEndless().survivalSeconds == savedTime);
        assert(loaded.getEndless().randomState == game.getEndless().randomState);
        assert(loaded.getEndless().completedOperations == game.getEndless().completedOperations);
        assert(loaded.getEndless().request == game.getEndless().request && loaded.getEndless().key == game.getEndless().key);
        assert(loaded.getEndless().deadline == game.getEndless().deadline);
        assert(loaded.getSnapshot().model.nodeCount == game.getSnapshot().model.nodeCount);
        for (int i = 0; i < game.getSnapshot().model.nodeCount; i++)
        {
            assert(game.getSnapshot().model.nodes[i].id == loaded.getSnapshot().model.nodes[i].id);
            assert(game.getSnapshot().model.nodes[i].leftId == loaded.getSnapshot().model.nodes[i].leftId);
            assert(game.getSnapshot().model.nodes[i].rightId == loaded.getSnapshot().model.nodes[i].rightId);
        }
        loaded.resume();
        loaded.setFocused(false);
        loaded.update(60);
        assert(loaded.getEndless().survivalSeconds == savedTime);
        loaded.setFocused(true);
        assert(loaded.getEndless().phase == "PAUSED");
        loaded.resume();
        std::string correct = prediction(loaded);
        PlayerAction wrong;
        wrong.name = "SELECT_ROTATION";
        wrong.text = correct == "NONE" ? "LL" : "NONE";
        GameSnapshot before = loaded.getSnapshot();
        int completed = loaded.getEndless().completedOperations;
        assert(!loaded.performAction(wrong).success);
        assert(loaded.getEndless().phase == "FAILED" && loaded.getEndless().reason == "Wrong prediction");
        assert(loaded.getSnapshot().model.nodeCount == before.model.nodeCount);
        assert(loaded.getEndless().completedOperations == completed);
        assert(loaded.getLeaderboardCount() == 1);
        loaded.performAction(wrong);
        loaded.update(100);
        assert(loaded.getLeaderboardCount() == 1);
        assert(!loaded.resumeEndless());

        assert(loaded.startEndless(2));
        loaded.update(8);
        PlayerAction late;
        late.name = "SELECT_ROTATION";
        late.text = "NONE";
        assert(!loaded.performAction(late).success);
        assert(loaded.getEndless().phase == "FAILED" && loaded.getEndless().reason == "Time expired");
        assert(loaded.getEndless().survivalSeconds == 8 && loaded.getSnapshot().model.nodeCount == 7);
        assert(loaded.getLeaderboardCount() == 2);
        GameController persisted(firstDirectory);
        assert(persisted.getLeaderboardCount() == 2);
        assert(persisted.getLeaderboardEntry(0).seconds >= persisted.getLeaderboardEntry(1).seconds);
        assert(persisted.startEndless(9));
        persisted.exitEndless();
        assert(persisted.getLeaderboardCount() == 2);

        std::ofstream corrupt((firstDirectory + "/endless-save.json").c_str());
        corrupt << "{\"tree\":\"broken\"}";
        corrupt.close();
        assert(!persisted.resumeEndless());

        assert(loaded.startEndless(77));
        answer(loaded);
        assert(!loaded.saveEndless());
        loaded.animation().setSpeed(0.25);
        loaded.update(1000);
        assert(loaded.getEndless().survivalSeconds == 0);
        assert(loaded.saveEndless());
        GameController waiting(firstDirectory);
        assert(waiting.resumeEndless());
        assert(waiting.getEndless().previousPhase == "WAITING_FOR_NEXT");
        waiting.resume();
        waiting.update(8);
        assert(waiting.getEndless().phase == "WAITING_FOR_ANSWER");
        PlayerAction failure;
        failure.name = "SELECT_ROTATION";
        failure.text = prediction(waiting) == "NONE" ? "LL" : "NONE";
        waiting.performAction(failure);
        assert(waiting.getLeaderboardEntry(1).seconds == 8);
        assert(waiting.getLeaderboardEntry(1).operations == 1);
        assert(waiting.getLeaderboardEntry(1).seed == 77);
        for (unsigned int seed = 100; seed < 112; seed++)
        {
            assert(waiting.startEndless(seed));
            waiting.update(8);
        }
        assert(waiting.getLeaderboardCount() == 10);
        assert(waiting.getLeaderboardEntry(2).seed == 2);
        assert(waiting.getLeaderboardEntry(3).seed == 100);
        GameController topTen(firstDirectory);
        assert(topTen.getLeaderboardCount() == 10);
        assert(topTen.getLeaderboardEntry(2).seed == 2);

        // The narrow JSON reader rejects invalid JSON numbers and structure.
        const std::string badJson[12] = {"{\"x\":01}", "{\"x\":+1}", "{\"x\":0x10}",
            "{\"x\":1.}", "{\"x\":.2}", "{\"x\":NaN}", "{\"x\":1e}",
            "{\"x\":1,}", "{\"x\":[1,]}", "{\"x\":1,\"x\":2}",
            "{\"x\":1e999}", "{\"x\":\"bad\\q\"}"};
        for (int i = 0; i < 12; i++)
        {
            std::string path = firstDirectory + "/invalid.json";
            std::ofstream output(path.c_str());
            output << badJson[i];
            output.close();
            JsonDocument document;
            assert(!document.load(path));
        }

        AVLTree serialized;
        assert(serialized.insert(3) && serialized.insert(1) && serialized.insert(5));
        std::string validTree = serialized.serialize();
        AVLTree restored;
        assert(restored.deserialize(validTree) && restored.validate());
        assert(restored.serialize() == validTree);
        assert(!restored.deserialize("4 1 3 1 1 # # 3 5 # #"));
        assert(!restored.deserialize("4 1 3 #"));
        assert(restored.serialize() == validTree);

        // Playback stepping must not mutate the authoritative controller state.
        assert(second.startEndless(3));
        answer(second);
        int nodes = second.getSnapshot().model.nodeCount;
        second.animation().pause();
        second.animation().next();
        second.animation().previous();
        assert(second.getSnapshot().model.nodeCount == nodes);
        second.update(50);
        assert(second.getEndless().survivalSeconds == 0);
        second.animation().play();
        second.update(100);
        assert(second.getEndless().phase == "WAITING_FOR_NEXT");
        second.animation().previous();
        double frozen = second.getEndless().survivalSeconds;
        second.update(25);
        assert(second.getEndless().survivalSeconds == frozen);
        assert(!second.saveEndless());
        second.animation().play();
        second.update(100);
        assert(second.getEndless().phase == "WAITING_FOR_NEXT");

        GameController sandbox(firstDirectory);
        assert(sandbox.selectMode("SANDBOX"));
        PlayerAction generate;
        generate.name = "GENERATE";
        assert(sandbox.performAction(generate).success);
        assert(sandbox.getSnapshot().model.nodeCount == 7);
        PlayerAction insert;
        insert.name = "INSERT";
        insert.value = 100;
        assert(sandbox.performAction(insert).success);
        assert(sandbox.getSnapshot().model.nodeCount == 8);
        assert(!sandbox.performAction(insert).accepted);
        sandbox.update(100);
        assert(!sandbox.performAction(insert).success);
        assert(sandbox.undo());
        assert(sandbox.getSnapshot().model.nodeCount == 7);
        assert(sandbox.replayLevel());
        sandbox.update(100);
        assert(sandbox.getSnapshot().model.nodeCount == 7);
        assert(sandbox.resetLevel() && sandbox.getSnapshot().model.nodeCount == 0);

        assert(sandbox.selectStructure("BinaryTree"));
        PlayerAction reconstruction;
        reconstruction.name = "BUILD_PRE_IN";
        reconstruction.text = "4 2 1 3 6 5 7 / 1 2 3 4 5 6 7";
        assert(sandbox.performAction(reconstruction).success);
        sandbox.update(100);
        GameSnapshot original = sandbox.getSnapshot();
        PlayerAction mirror;
        mirror.name = "MIRROR";
        assert(sandbox.performAction(mirror).success);
        assert(sandbox.undo());
        assert(sandbox.getSnapshot().model.nodes[0].leftId == original.model.nodes[0].leftId);
        assert(sandbox.selectStructure("MaxHeap"));
        assert(sandbox.performAction(generate).success);
        PlayerAction sort;
        sort.name = "HEAP_SORT";
        assert(sandbox.performAction(sort).success);
        const BackendFrame& sorted = sandbox.getSnapshot().model;
        assert(sorted.heapSize == 0 && sorted.arraySize == 7);
        for (int i = 1; i < sorted.arraySize; i++)
        {
            assert(sorted.values[i - 1] <= sorted.values[i]);
        }
        assert(sandbox.undo());
        assert(sandbox.getSnapshot().model.heapSize == 7);
        PlayerAction swap;
        swap.name = "HEAP_SWAP";
        swap.indexA = 0;
        swap.indexB = 6;
        assert(sandbox.performAction(swap).success);
        assert(sandbox.getState().status == "REPAIRING");
        assert(sandbox.undo());
        assert(sandbox.getState().status != "REPAIRING");
        assert(sandbox.selectStructure("AVLTree"));
        reconstruction.text = "30 10 20 / 10 20 30";
        assert(sandbox.performAction(reconstruction).success);
        sandbox.update(100);
        assert(sandbox.getState().status == "REPAIRING");
        PlayerAction rotate;
        rotate.name = "ROTATE";
        rotate.text = "LR";
        rotate.nodeId = sandbox.getSnapshot().model.rootId;
        assert(sandbox.performAction(rotate).success);
        assert(sandbox.getState().status == "READY");
        assert(sandbox.getSnapshot().model.nodes[0].value == 20);
        assert(sandbox.undo() && sandbox.getState().status == "REPAIRING");
    }
};

int main(int argc, char* argv[])
{
    assert(argc == 3);
    EndlessTests::run(argv[1], argv[2]);
    std::cout << "Endless: deterministic stream, predictions, timing, pause, leaderboard and save/resume tests passed.\n";
    return 0;
}
