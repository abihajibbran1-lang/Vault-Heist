#ifndef VAULT_TREE_RENDERER_H
#define VAULT_TREE_RENDERER_H

#include <SFML/Graphics.hpp>
#include "middleware/AnimationController.h"

class TreeRenderer
{
private:
    struct Position
    {
        int id;
        float x;
        float y;
    };
    void place(const BackendFrame& frame, int id, int depth, int& order, Position* positions,
               int& count, float zoom, sf::Vector2f pan) const;
    sf::Vector2f locate(const Position* positions, int count, int id, sf::Vector2f fallback) const;

public:
    void drawSlots(sf::RenderWindow& window, const sf::Font& font, const BackendFrame& frame,
                   const IntBuffer& placed, float zoom, sf::Vector2f pan) const;
    int pick(const BackendFrame& frame, float zoom, sf::Vector2f pan, sf::Vector2f mouse) const;
    void draw(sf::RenderWindow& window, const sf::Font& font, const AnimationController& animation,
              float zoom, sf::Vector2f pan, sf::Vector2f mouse, int selectedId = -1, const IntBuffer* selected = NULL) const;
};

#endif
