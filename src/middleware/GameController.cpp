#include "middleware/GameController.h"
#include "middleware/JsonDocument.h"
#include "middleware/SaveFile.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <stdexcept>
#include <climits>

EndlessState::EndlessState()
    : key(0), completedOperations(0), survivalSeconds(0), deadline(0),
      seed(0), randomState(0), recorded(false)
{
}

LeaderboardEntry::LeaderboardEntry() : seconds(0), operations(0), seed(0)
{
}

GameController::GameController(const std::string& directory)
    : history(NULL), historyCount(0), historyCapacity(0), leaderboardCount(0), saveDirectory(directory), focused(true)
{
    state.mode = "MENU";
    loadLeaderboard();
    loadCampaignProgress();
    snapshot = backend.snapshot();
}

GameController::~GameController()
{
    delete[] history;
}

void GameController::clearHistory()
{
    delete[] history;
    history = NULL;
    historyCount = 0;
    historyCapacity = 0;
}

void GameController::remember(const BackendAdapter& before, const PlayerAction& action, const GameState& oldState)
{
    if (historyCount == historyCapacity)
    {
        int capacity = historyCapacity == 0 ? 8 : historyCapacity * 2;
        HistoryStep* replacement = new HistoryStep[capacity];
        try
        {
            for (int i = 0; i < historyCount; i++)
            {
                replacement[i] = history[i];
            }
        }
        catch (...)
        {
            delete[] replacement;
            throw;
        }
        delete[] history;
        history = replacement;
        historyCapacity = capacity;
    }
    history[historyCount].before = before;
    history[historyCount].action = action;
    history[historyCount].state = oldState;
    history[historyCount].playback = backend.getEvents();
    historyCount++;
}

bool GameController::undo()
{
    if ((state.mode != "SANDBOX" && state.mode != "CAMPAIGN") || historyCount == 0)
    {
        return false;
    }
    historyCount--;
    backend = history[historyCount].before;
    state = history[historyCount].state;
    history[historyCount] = HistoryStep();
    snapshot = backend.snapshot();
    EventBatch empty;
    animator.load(snapshot, empty, snapshot);
    return true;
}

bool GameController::replayLevel()
{
    if (state.mode != "SANDBOX" && state.mode != "CAMPAIGN")
    {
        return false;
    }
    BackendAdapter replay = initialBackend;
    GameSnapshot before = replay.snapshot();
    if (state.mode == "CAMPAIGN" && campaignKind == "RECONSTRUCT")
    {
        before.model.clear();
    }
    EventBatch events;
    for (int i = 0; i < historyCount; i++)
    {
        events.append(history[i].playback);
    }
    animator.load(before, events, snapshot);
    return true;
}

bool GameController::resetLevel()
{
    if (state.mode != "SANDBOX" && state.mode != "CAMPAIGN")
    {
        return false;
    }
    backend = initialBackend;
    if (state.mode == "CAMPAIGN" && campaignKind == "RECONSTRUCT")
    {
        backend.select("BinaryTree");
    }
    if (state.mode == "CAMPAIGN")
    {
        state = initialState;
    }
    clearHistory();
    snapshot = backend.snapshot();
    EventBatch empty;
    animator.load(snapshot, empty, snapshot);
    return true;
}

double GameController::intervalAt(double seconds)
{
    double interval = 8.0 - std::floor(seconds / 30.0);
    return interval < 3.0 ? 3.0 : interval;
}

std::uint32_t GameController::random()
{
    endless.randomState = endless.randomState * 1664525u + 1013904223u;
    return endless.randomState;
}

void GameController::generateRequest()
{
    // Consume an unbiased operation roll even for forced choices, then an
    // unbiased key/index roll. Insertion collisions scan with no extra rolls.
    int count = backend.size();
    std::uint32_t operationRoll = randomBelow(100);
    bool insertion = count == 0 || (count < 31 && operationRoll < 60u);
    if (insertion)
    {
        endless.request = "INSERT";
        endless.key = static_cast<int>(randomBelow(999)) + 1;
        while (backend.contains(endless.key))
        {
            endless.key = endless.key == 999 ? 1 : endless.key + 1;
        }
    }
    else
    {
        endless.request = "DELETE";
        GameSnapshot current = backend.snapshot();
        int* keys = new int[count];
        for (int i = 0; i < count; i++)
        {
            keys[i] = current.model.nodes[i].value;
        }
        // At most 31 keys: explicit insertion sort avoids STL algorithms.
        for (int i = 1; i < count; i++)
        {
            int value = keys[i];
            int j = i - 1;
            while (j >= 0 && keys[j] > value)
            {
                keys[j + 1] = keys[j];
                j--;
            }
            keys[j + 1] = value;
        }
        endless.key = keys[randomBelow(static_cast<std::uint32_t>(count))];
        delete[] keys;
    }
    endless.deadline = endless.survivalSeconds + intervalAt(endless.survivalSeconds);
    endless.phase = "WAITING_FOR_ANSWER";
}

std::uint32_t GameController::randomBelow(std::uint32_t bound)
{
    // Rejection avoids modulo bias while keeping the generator per-run.
    std::uint32_t threshold = (0u - bound) % bound;
    std::uint32_t value = random();
    while (value < threshold)
    {
        value = random();
    }
    return value % bound;
}

bool GameController::removeSavedRun()
{
    const std::string path = saveDirectory + "/endless-save.json";
    std::ifstream input(path.c_str());
    if (!input)
    {
        return true;
    }
    input.close();
    return std::remove(path.c_str()) == 0;
}

bool GameController::startEndless(std::uint32_t seed)
{
    clearHistory();
    // Prevent an old saved run from being resumed after this run fails.
    if (!removeSavedRun())
    {
        state.message = "Cannot replace the previous Endless save";
        return false;
    }
    backend.select("AVLTree");
    int keys[7] = {50, 30, 70, 20, 40, 60, 80};
    for (int i = 0; i < 7; i++)
    {
        PlayerAction action;
        action.name = "INSERT";
        action.value = keys[i];
        std::string ignored;
        if (!backend.execute(action, ignored))
        {
            return false;
        }
    }
    state = GameState();
    state.mode = "ENDLESS";
    state.objective = "Predict the first rotation case before time expires";
    endless = EndlessState();
    endless.seed = seed;
    endless.randomState = seed;
    snapshot = backend.snapshot();
    EventBatch empty;
    animator.load(snapshot, empty, snapshot);
    if (!backend.validate())
    {
        endless.phase = "ERROR";
        state.message = "Initial AVL invariant error";
        return false;
    }
    generateRequest();
    return true;
}

bool GameController::selectMode(const std::string& mode)
{
    if (mode != "SANDBOX" && mode != "MENU")
    {
        return false;
    }
    state = GameState();
    state.mode = mode;
    return selectStructure("AVLTree");
}

bool GameController::selectStructure(const std::string& structure)
{
    if (state.mode == "ENDLESS")
    {
        return false;
    }
    if (!backend.select(structure))
    {
        return false;
    }
    clearHistory();
    initialBackend = backend;
    snapshot = backend.snapshot();
    EventBatch empty;
    animator.load(snapshot, empty, snapshot);
    return true;
}

OperationResult GameController::performAction(const PlayerAction& action)
{
    if (state.mode == "CAMPAIGN")
    {
        return campaignAction(action);
    }
    BackendAdapter oldBackend = backend;
    GameState oldState = state;
    OperationResult result;
    result.before = snapshot;
    result.after = snapshot;
    if (!focused)
    {
        result.message = "Resume the focused game first";
        return result;
    }
    if (state.mode == "ENDLESS")
    {
        if (endless.phase != "WAITING_FOR_ANSWER" || action.name != "SELECT_ROTATION")
        {
            result.message = "Wait for the next request";
            return result;
        }
        if (endless.survivalSeconds >= endless.deadline)
        {
            failEndless("Time expired");
            result.message = endless.reason;
            return result;
        }
        if (action.text != "NONE" && action.text != "LL" && action.text != "RR" &&
            action.text != "LR" && action.text != "RL")
        {
            result.message = "Choose NONE, LL, RR, LR or RL";
            return result;
        }
        result.accepted = true;
        if (action.text != backend.predict(endless.request, endless.key))
        {
            failEndless("Wrong prediction");
            result.message = endless.reason;
            result.gameState = state;
            return result;
        }
        PlayerAction operation;
        operation.name = endless.request;
        operation.value = endless.key;
        result.success = backend.execute(operation, result.message);
        if (!result.success || !backend.validate())
        {
            endless.phase = "ERROR";
            endless.reason = "AVL invariant error";
            state.message = endless.reason;
            removeSavedRun();
            result.message = endless.reason;
            return result;
        }
        endless.completedOperations++;
        endless.phase = "ANIMATING";
        result.message = "Correct prediction";
    }
    else if (state.mode == "SANDBOX")
    {
        if (!animator.isFinished())
        {
            result.message = "Finish or step through the current animation first";
            return result;
        }
        result.accepted = true;
        result.success = backend.execute(action, result.message);
        if (!backend.validate())
        {
            if (snapshot.structure == "AVLTree" && result.success && backend.validateStructure() &&
                (action.name == "BUILD_PRE_IN" || action.name == "BUILD_POST_IN" || action.name == "ROTATE"))
            {
                state.status = "REPAIRING";
                result.message = "Valid BST shape; repair its AVL balance with a rotation";
            }
            else if (snapshot.structure == "MaxHeap" && action.name == "HEAP_SWAP" && result.success)
            {
                state.status = "REPAIRING";
                result.message = "Swap applied; heap order still needs repair";
            }
            else
            {
                backend = oldBackend;
                state.status = "ERROR";
                result.message = "Operation rejected: backend invariant error";
                result.success = false;
            }
        }
        else
        {
            state.status = "READY";
        }
        if (result.success)
        {
            remember(oldBackend, action, oldState);
        }
    }
    else
    {
        result.message = "Select a game mode first";
        return result;
    }
    snapshot = backend.snapshot();
    result.after = snapshot;
    result.events = backend.getEvents();
    state.message = result.message;
    result.gameState = state;
    animator.load(result.before, result.events, result.after);
    return result;
}

void GameController::update(double seconds)
{
    if (!std::isfinite(seconds) || seconds < 0 || !focused)
    {
        return;
    }
    if (state.mode != "ENDLESS")
    {
        animator.update(seconds);
        if (state.mode == "CAMPAIGN" && animator.isFinished())
        {
            finishCampaignAction();
        }
        return;
    }
    if (endless.phase == "ANIMATING" ||
        (endless.phase == "WAITING_FOR_NEXT" && !animator.isFinished()))
    {
        animator.update(seconds);
        if (animator.isFinished())
        {
            endless.phase = "WAITING_FOR_NEXT";
        }
        return;
    }
    if (endless.phase != "WAITING_FOR_ANSWER" && endless.phase != "WAITING_FOR_NEXT")
    {
        return;
    }
    double remaining = seconds;
    while (remaining > 0 &&
           (endless.phase == "WAITING_FOR_ANSWER" || endless.phase == "WAITING_FOR_NEXT"))
    {
        double untilDeadline = endless.deadline - endless.survivalSeconds;
        if (remaining < untilDeadline)
        {
            endless.survivalSeconds += remaining;
            break;
        }
        endless.survivalSeconds = endless.deadline;
        remaining -= untilDeadline;
        if (endless.phase == "WAITING_FOR_ANSWER")
        {
            failEndless("Time expired");
        }
        else
        {
            generateRequest();
        }
    }
}

void GameController::pause()
{
    if (state.mode == "ENDLESS" && (endless.phase == "WAITING_FOR_ANSWER" ||
        endless.phase == "WAITING_FOR_NEXT" || endless.phase == "ANIMATING"))
    {
        endless.previousPhase = endless.phase;
        endless.phase = "PAUSED";
    }
}

void GameController::resume()
{
    if (endless.phase == "PAUSED")
    {
        endless.phase = endless.previousPhase;
    }
}

void GameController::setFocused(bool value)
{
    focused = value;
    if (!value)
    {
        pause();
    }
}

void GameController::exitEndless()
{
    pause();
    state.mode = "MENU";
}

void GameController::failEndless(const std::string& reason)
{
    if (endless.phase == "FAILED" || endless.phase == "ERROR")
    {
        return;
    }
    endless.phase = "FAILED";
    endless.reason = reason;
    state.message = reason;
    state.failed = true;
    if (!removeSavedRun())
    {
        endless.phase = "ERROR";
        state.message = "Cannot invalidate the failed-run save";
        return;
    }
    if (!endless.recorded)
    {
        LeaderboardEntry entry;
        entry.seconds = endless.survivalSeconds > INT_MAX ? INT_MAX : static_cast<int>(std::floor(endless.survivalSeconds));
        entry.operations = endless.completedOperations;
        entry.seed = endless.seed;
        int position = 0;
        while (position < leaderboardCount &&
               (leaderboard[position].seconds > entry.seconds ||
                (leaderboard[position].seconds == entry.seconds && leaderboard[position].operations >= entry.operations)))
        {
            position++;
        }
        if (position < 10)
        {
            if (leaderboardCount < 10)
            {
                leaderboardCount++;
            }
            for (int i = leaderboardCount - 1; i > position; i--)
            {
                leaderboard[i] = leaderboard[i - 1];
            }
            leaderboard[position] = entry;
        }
        endless.recorded = true;
        if (!writeLeaderboard())
        {
            state.message += " (leaderboard could not be saved)";
        }
    }
}

bool GameController::writeLeaderboard()
{
    std::string path = saveDirectory + "/endless-leaderboard.json";
    std::ofstream output((path + ".tmp").c_str());
    output << "{\"seconds\":[";
    for (int i = 0; i < leaderboardCount; i++)
    {
        output << (i == 0 ? "" : ",") << leaderboard[i].seconds;
    }
    output << "],\"operations\":[";
    for (int i = 0; i < leaderboardCount; i++)
    {
        output << (i == 0 ? "" : ",") << leaderboard[i].operations;
    }
    output << "],\"seeds\":\"";
    for (int i = 0; i < leaderboardCount; i++)
    {
        output << leaderboard[i].seed << ' ';
    }
    output << "\"}\n";
    output.close();
    return !output.fail() && SaveFile::commit(path + ".tmp", path);
}

bool GameController::loadLeaderboard()
{
    JsonDocument document;
    IntBuffer scores;
    IntBuffer operations;
    std::string seeds;
    if (!document.load(saveDirectory + "/endless-leaderboard.json") ||
        !document.only("|seconds||operations||seeds|") || !document.array("seconds", scores) ||
        !document.array("operations", operations) || !document.string("seeds", seeds) ||
        scores.getCount() != operations.getCount() || scores.getCount() > 10)
    {
        return false;
    }
    std::istringstream input(seeds);
    LeaderboardEntry restored[10];
    for (int i = 0; i < scores.getCount(); i++)
    {
        unsigned long long seed = 0;
        if (!(input >> seed) || seed > 4294967295ULL || scores.get(i) < 0 || operations.get(i) < 0)
        {
            return false;
        }
        restored[i].seconds = scores.get(i);
        restored[i].operations = operations.get(i);
        restored[i].seed = static_cast<std::uint32_t>(seed);
        if (i > 0 && (restored[i].seconds > restored[i - 1].seconds ||
            (restored[i].seconds == restored[i - 1].seconds && restored[i].operations > restored[i - 1].operations)))
        {
            return false;
        }
    }
    std::string extra;
    if (input >> extra)
    {
        return false;
    }
    leaderboardCount = scores.getCount();
    for (int i = 0; i < leaderboardCount; i++)
    {
        leaderboard[i] = restored[i];
    }
    return true;
}

bool GameController::saveEndless()
{
    std::string phase = endless.phase == "PAUSED" ? endless.previousPhase : endless.phase;
    if (state.mode != "ENDLESS" || !animator.isFinished() ||
        (phase != "WAITING_FOR_ANSWER" && phase != "WAITING_FOR_NEXT"))
    {
        state.message = "Finish the animation before saving";
        return false;
    }
    std::string path = saveDirectory + "/endless-save.json";
    std::ofstream output((path + ".tmp").c_str());
    output << std::setprecision(17)
           << "{\"version\":1,\"tree\":\"" << backend.serializeTree()
           << "\",\"phase\":\"" << phase << "\",\"request\":\"" << endless.request
           << "\",\"key\":" << endless.key << ",\"seed\":" << endless.seed
           << ",\"randomState\":" << endless.randomState << ",\"survivalSeconds\":"
           << endless.survivalSeconds << ",\"completedOperations\":" << endless.completedOperations
           << ",\"remainingSeconds\":" << endless.deadline - endless.survivalSeconds << "}\n";
    output.close();
    bool success = !output.fail() && SaveFile::commit(path + ".tmp", path);
    state.message = success ? "Endless run saved" : "Could not save Endless run";
    return success;
}

bool GameController::resumeEndless()
{
    JsonDocument document;
    EndlessState restored;
    std::string tree;
    double seed = 0;
    double randomState = 0;
    double remaining = 0;
    int version = 0;
    bool valid = document.load(saveDirectory + "/endless-save.json") &&
        document.only("|version||tree||phase||request||key||seed||randomState||survivalSeconds||completedOperations||remainingSeconds|") &&
        document.integer("version", version) && version == 1 &&
        document.string("tree", tree) && document.string("phase", restored.previousPhase) &&
        document.string("request", restored.request) && document.integer("key", restored.key) &&
        document.number("seed", seed) && document.number("randomState", randomState) &&
        document.number("survivalSeconds", restored.survivalSeconds) &&
        document.integer("completedOperations", restored.completedOperations) &&
        document.number("remainingSeconds", remaining);
    if (!valid || seed < 0 || seed > 4294967295.0 || std::floor(seed) != seed ||
        randomState < 0 || randomState > 4294967295.0 || std::floor(randomState) != randomState ||
        restored.survivalSeconds < 0 || restored.survivalSeconds > INT_MAX || restored.completedOperations < 0 || remaining <= 0 || remaining > 8 ||
        (restored.previousPhase != "WAITING_FOR_ANSWER" && restored.previousPhase != "WAITING_FOR_NEXT") ||
        (restored.request != "INSERT" && restored.request != "DELETE") || restored.key < 1 || restored.key > 999)
    {
        state.message = "Missing or corrupt Endless save; start a new run";
        return false;
    }
    BackendAdapter candidate;
    if (!candidate.restoreAVL(tree) || candidate.size() > 31)
    {
        state.message = "Invalid saved AVL; start a new run";
        return false;
    }
    GameSnapshot restoredSnapshot = candidate.snapshot();
    for (int i = 0; i < restoredSnapshot.model.nodeCount; i++)
    {
        if (restoredSnapshot.model.nodes[i].value < 1 || restoredSnapshot.model.nodes[i].value > 999)
        {
            state.message = "Invalid saved key";
            return false;
        }
    }
    if (restored.previousPhase == "WAITING_FOR_ANSWER" &&
        ((restored.request == "INSERT" && (candidate.contains(restored.key) || candidate.size() >= 31)) ||
         (restored.request == "DELETE" && !candidate.contains(restored.key))))
    {
        state.message = "Invalid saved request";
        return false;
    }
    backend = candidate;
    restored.seed = static_cast<std::uint32_t>(seed);
    restored.randomState = static_cast<std::uint32_t>(randomState);
    restored.deadline = restored.survivalSeconds + remaining;
    restored.phase = "PAUSED";
    endless = restored;
    state = GameState();
    state.mode = "ENDLESS";
    state.objective = "Predict the first rotation case before time expires";
    state.message = "Run restored paused; choose Resume";
    snapshot = backend.snapshot();
    EventBatch empty;
    animator.load(snapshot, empty, snapshot);
    return true;
}

const GameSnapshot& GameController::getSnapshot() const
{
    return snapshot;
}

const GameState& GameController::getState() const
{
    return state;
}

const EndlessState& GameController::getEndless() const
{
    return endless;
}

AnimationController& GameController::animation()
{
    return animator;
}

int GameController::getLeaderboardCount() const
{
    return leaderboardCount;
}

const LeaderboardEntry& GameController::getLeaderboardEntry(int index) const
{
    if (index < 0 || index >= leaderboardCount)
    {
        throw std::out_of_range("Leaderboard index");
    }
    return leaderboard[index];
}
