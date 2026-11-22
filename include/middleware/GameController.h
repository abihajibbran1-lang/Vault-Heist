#ifndef VAULT_GAME_CONTROLLER_H
#define VAULT_GAME_CONTROLLER_H

#include "middleware/BackendAdapter.h"
#include "middleware/AnimationController.h"
#include <cstdint>

struct EndlessState
{
    std::string phase;
    std::string previousPhase;
    std::string request;
    std::string reason;
    int key;
    int completedOperations;
    double survivalSeconds;
    double deadline;
    std::uint32_t seed;
    std::uint32_t randomState;
    bool recorded;
    EndlessState();
};

struct LeaderboardEntry
{
    int seconds;
    int operations;
    std::uint32_t seed;
    LeaderboardEntry();
};

class GameController
{
private:
    struct HistoryStep
    {
        BackendAdapter before;
        PlayerAction action;
        GameState state;
        EventBatch playback;
    };
    HistoryStep* history;
    int historyCount;
    int historyCapacity;
    BackendAdapter initialBackend;
    void clearHistory();
    void remember(const BackendAdapter& before, const PlayerAction& action, const GameState& oldState);
    BackendAdapter backend;
    GameState state;
    GameSnapshot snapshot;
    AnimationController animator;
    EndlessState endless;
    LeaderboardEntry leaderboard[10];
    int leaderboardCount;
    std::string saveDirectory;
    bool focused;
    IntBuffer campaignRoute;
    IntBuffer campaignTargets;
    IntBuffer campaignValues;
    IntBuffer campaignInorder;
    int campaignCaseSize = 0;
    bool loadAvlRound(int round);
    void avlCampaignAction(const PlayerAction& action, OperationResult& result, bool& complete);
    void heapCampaignAction(const PlayerAction& action, OperationResult& result, bool& complete);
    bool loadHeapRound(int round);
    std::string campaignKind;
    std::string campaignTitle;
    std::string campaignOrders;
    std::string campaignHint;
    int campaignStars[20] = {};
    bool restoringCampaign = false;
    void loadCampaignProgress();
    bool saveCampaignProgress();
    GameState initialState;
    OperationResult campaignAction(const PlayerAction& action);
    void finishCampaignAction();
    std::uint32_t random();
    std::uint32_t randomBelow(std::uint32_t bound);
    void generateRequest();
    void failEndless(const std::string& reason);
    bool writeLeaderboard();
    bool loadLeaderboard();
    bool removeSavedRun();
    GameController(const GameController&) = delete;
    GameController& operator=(const GameController&) = delete;

public:
    bool heapReady() const;
    explicit GameController(const std::string& directory = "data/saves");
    ~GameController();
    bool startEndless(std::uint32_t seed);
    bool loadLevel(int act, int level);
    bool requestHint(std::string& message);
    bool saveCampaign();
    bool resumeCampaign();
    bool levelAvailable(int act, int level) const;
    int levelStars(int act, int level) const;
    const std::string& getCampaignTitle() const;
    const std::string& getCampaignKind() const;
    std::string campaignInstruction() const;
    std::string briefingInstructions() const;
    GameSnapshot blueprint() const;
    bool selectMode(const std::string& mode);
    bool selectStructure(const std::string& structure);
    OperationResult performAction(const PlayerAction& action);
    void update(double seconds);
    void pause();
    void resume();
    void setFocused(bool value);
    bool saveEndless();
    bool resumeEndless();
    void exitEndless();
    bool undo();
    bool replayLevel();
    bool resetLevel();
    const GameSnapshot& getSnapshot() const;
    const GameState& getState() const;
    const EndlessState& getEndless() const;
    AnimationController& animation();
    int getLeaderboardCount() const;
    const LeaderboardEntry& getLeaderboardEntry(int index) const;
    static double intervalAt(double seconds);
};

#endif
