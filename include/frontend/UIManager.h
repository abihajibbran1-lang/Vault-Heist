#ifndef VAULT_UI_MANAGER_H
#define VAULT_UI_MANAGER_H

#include "frontend/Screen.h"
#include "frontend/TreeRenderer.h"
#include "frontend/HeapRenderer.h"
#include "middleware/GameController.h"

class UIManager
{
private:
    GameController controller;
    Screen screen;
    sf::Font font;
    TreeRenderer tree;
    HeapRenderer heap;
    std::string input;
    std::string notice;
    sf::Vector2f pan;
    float zoom;
    bool clicked;
    bool dragging;
    bool editingInput;
    sf::Vector2f lastMouse;
    bool muted;
    double speed;
    int sandboxOperation;
    int draggedToken = -1;
    int draggedSubtree = -1;
    void fitTree();
    std::uint32_t nextSeed;
    void startEndless();
    void operation(const std::string& name, const std::string& detail = "");
    bool button(sf::RenderWindow& window, const std::string& text, float x, float y,
                float width, sf::Vector2f mouse, bool enabled = true);

public:
    explicit UIManager(const std::string& saveDirectory = "data/saves");
    bool openCampaign(int act, int level, bool briefing = false);
    bool prepareEvidence(const std::string& phase);
    bool initialize();
    void openEndless();
    void handle(const sf::Event& event, sf::RenderWindow& window);
    void update(double seconds);
    void draw(sf::RenderWindow& window);
};

#endif
