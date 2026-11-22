#include "frontend/HeapRenderer.h"
#include "frontend/Button.h"
#include <cmath>

int HeapRenderer::pick(const BackendFrame& frame, sf::Vector2f mouse) const
{
    float cell = frame.arraySize > 0 ? 1080.0F / frame.arraySize : 50;
    if (cell > 58)
    {
        cell = 58;
    }
    float left = 640 - cell * frame.arraySize / 2;
    for (int i = 0; i < frame.arraySize; i++)
    {
        if (sf::FloatRect(left + i * cell, 510, cell - 3, 43).contains(mouse))
        {
            return i;
        }
    }
    for (int i = 0; i < frame.heapSize; i++)
    {
        int depth = static_cast<int>(std::floor(std::log2(i + 1)));
        float x = 100 + (i - ((1 << depth) - 1) + 0.5F) * 1080 / (1 << depth);
        float y = 200 + depth * 68.0F;
        float dx = x - mouse.x;
        float dy = y - mouse.y;
        if (dx * dx + dy * dy <= 26 * 26)
        {
            return i;
        }
    }
    return -1;
}

void HeapRenderer::draw(sf::RenderWindow& window, const sf::Font& font,
                         const AnimationController& animation, const IntBuffer* selected) const
{
    const BackendFrame& frame = animation.getTo().model;
    float cell = frame.arraySize > 0 ? 1080.0F / frame.arraySize : 50;
    if (cell > 58)
    {
        cell = 58;
    }
    float left = 640 - cell * frame.arraySize / 2;
    const GameEvent* event = animation.getEvent();
    bool swapping = event != NULL && event->type == "HEAP_SWAP";
    float progress = static_cast<float>(animation.getProgress());
    float ease = progress * progress * (3 - 2 * progress);
    if (frame.heapSize > 0)
    {
        Button::text(window, font, "ACTIVE", left, 491, 12);
    }
    if (frame.heapSize < frame.arraySize)
    {
        Button::text(window, font, "SORTED", left + frame.heapSize * cell, 491, 12);
    }
    Button::text(window, font, "ACTIVE HEAP: " + std::to_string(frame.heapSize) + "   |   SORTED: " + std::to_string(frame.arraySize - frame.heapSize), 60, 180, 14);
    for (int i = 0; i < frame.arraySize; i++)
    {
        bool chosen = false;
        if (selected != NULL)
        {
            for (int j = 0; j < selected->getCount(); j++)
            {
                chosen = chosen || selected->get(j) == i;
            }
        }
        bool active = event != NULL && (event->indexA == i || event->indexB == i);
        Button::panel(window, sf::FloatRect(left + i * cell, 510, cell - 3, 43),
                      chosen || active ? sf::Color(130, 205, 247) :
                      (i < frame.heapSize ? sf::Color(255, 180, 137) : sf::Color(164, 221, 151)), 8);
        float labelIndex = static_cast<float>(i);
        int value = frame.values[i];
        if (swapping && i < animation.getFrom().model.arraySize && (i == event->indexA || i == event->indexB))
        {
            int destination = i == event->indexA ? event->indexB : event->indexA;
            labelIndex += (destination - i) * ease;
            value = animation.getFrom().model.values[i];
        }
        Button::text(window, font, std::to_string(value), left + labelIndex * cell + 8, 517, 17);
        Button::text(window, font, std::string(chosen ? "[" : "") + std::to_string(i) + (chosen ? "]" : ""), left + i * cell + 10, 554, 13);
    }
    for (int i = 0; i < frame.heapSize; i++)
    {
        int depth = static_cast<int>(std::floor(std::log2(i + 1)));
        int first = (1 << depth) - 1;
        int rowCount = 1 << depth;
        float x = 100 + (i - first + 0.5F) * 1080 / rowCount;
        float y = 200 + depth * 68.0F;
        if (i > 0)
        {
            int parent = (i - 1) / 2;
            int parentFirst = (1 << (depth - 1)) - 1;
            float px = 100 + (parent - parentFirst + 0.5F) * 1080 / (1 << (depth - 1));
            sf::Vertex line[2] = {sf::Vertex(sf::Vector2f(x, y), sf::Color(200, 130, 145)),
                sf::Vertex(sf::Vector2f(px, y - 68), sf::Color(200, 130, 145))};
            window.draw(line, 2, sf::Lines);
        }
        sf::CircleShape circle(21);
        circle.setPosition(x - 21, y - 21);
        circle.setFillColor(sf::Color(255, 165, 151));
        bool chosen = false;
        if (selected != NULL)
        {
            for (int j = 0; j < selected->getCount(); j++)
            {
                chosen = chosen || selected->get(j) == i;
            }
        }
        bool active = event != NULL && (event->indexA == i || event->indexB == i);
        if (chosen || active)
        {
            circle.setFillColor(sf::Color(130, 205, 247));
            circle.setOutlineThickness(3);
            circle.setOutlineColor(sf::Color(48, 98, 140));
        }
        else if (i > 0 && frame.values[(i - 1) / 2] < frame.values[i])
        {
            circle.setOutlineThickness(3);
            circle.setOutlineColor(sf::Color(190, 60, 65));
        }
        window.draw(circle);
        Button::text(window, font, "i=" + std::to_string(i), x - 15, y + 22, 11);
        int value = frame.values[i];
        float labelX = x;
        float labelY = y;
        if (swapping && i < animation.getFrom().model.arraySize && (i == event->indexA || i == event->indexB))
        {
            int destination = i == event->indexA ? event->indexB : event->indexA;
            int destinationDepth = static_cast<int>(std::floor(std::log2(destination + 1)));
            int destinationFirst = (1 << destinationDepth) - 1;
            float destinationX = 100 + (destination - destinationFirst + 0.5F) * 1080 / (1 << destinationDepth);
            labelX += (destinationX - x) * ease;
            labelY += ((200 + destinationDepth * 68.0F) - y) * ease;
            value = animation.getFrom().model.values[i];
        }
        Button::text(window, font, std::to_string(value), labelX - 10, labelY - 13, 18);
    }
}
