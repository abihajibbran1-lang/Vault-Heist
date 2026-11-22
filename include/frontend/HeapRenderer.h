#ifndef VAULT_HEAP_RENDERER_H
#define VAULT_HEAP_RENDERER_H

#include "frontend/TreeRenderer.h"

class HeapRenderer
{
public:
    int pick(const BackendFrame& frame, sf::Vector2f mouse) const;
    void draw(sf::RenderWindow& window, const sf::Font& font,
              const AnimationController& animation, const IntBuffer* selected = NULL) const;
};

#endif
