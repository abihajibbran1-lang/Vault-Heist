#include "frontend/TreeRenderer.h"
#include "frontend/Button.h"
#include <cmath>
#include <sstream>

void TreeRenderer::drawSlots(sf::RenderWindow& window, const sf::Font& font, const BackendFrame& frame,
                              const IntBuffer& placed, float zoom, sf::Vector2f pan) const
{
    Position* positions = new Position[frame.nodeCount];
    int count = 0;
    int order = 0;
    place(frame, frame.rootId, 0, order, positions, count, zoom, pan);
    for (int i = 0; i < frame.nodeCount; i++)
    {
        const BackendNodeView& node = frame.nodes[i];
        sf::Vector2f position = locate(positions, count, node.id, sf::Vector2f());
        if (node.parentId != -1)
        {
            sf::Vector2f parent = locate(positions, count, node.parentId, position);
            sf::Vertex line[2] = {sf::Vertex(position, sf::Color(200, 135, 155)), sf::Vertex(parent, sf::Color(200, 135, 155))};
            window.draw(line, 2, sf::Lines);
        }
        bool filled = false;
        for (int j = 0; j < placed.getCount(); j++)
        {
            filled = filled || placed.get(j) == node.id;
        }
        sf::CircleShape slot(24);
        slot.setPosition(position.x - 24, position.y - 24);
        slot.setFillColor(filled ? sf::Color(255, 168, 177) : sf::Color(255, 249, 240));
        slot.setOutlineColor(sf::Color(161, 105, 142));
        slot.setOutlineThickness(2);
        window.draw(slot);
        Button::text(window, font, filled ? std::to_string(node.value) : "?", position.x - 11, position.y - 13, 18);
    }
    delete[] positions;
}

int TreeRenderer::pick(const BackendFrame& frame, float zoom, sf::Vector2f pan, sf::Vector2f mouse) const
{
    Position* positions = new Position[frame.nodeCount];
    int count = 0;
    int order = 0;
    place(frame, frame.rootId, 0, order, positions, count, zoom, pan);
    int selected = -1;
    for (int i = 0; i < count; i++)
    {
        float x = mouse.x - positions[i].x;
        float y = mouse.y - positions[i].y;
        if (x * x + y * y <= 26 * zoom * 26 * zoom)
        {
            selected = positions[i].id;
        }
    }
    delete[] positions;
    return selected;
}

void TreeRenderer::place(const BackendFrame& frame, int id, int depth, int& order,
                          Position* positions, int& count, float zoom, sf::Vector2f pan) const
{
    if (id == -1)
    {
        return;
    }
    for (int i = 0; i < frame.nodeCount; i++)
    {
        const BackendNodeView& node = frame.nodes[i];
        if (node.id == id)
        {
            place(frame, node.leftId, depth + 1, order, positions, count, zoom, pan);
            Position& position = positions[count++];
            position.id = id;
            float spacing = 850.0F / (frame.nodeCount + 1);
            if (spacing < 52)
            {
                spacing = 52;
            }
            position.x = 640 + ((order + 1) * spacing - (frame.nodeCount + 1) * spacing / 2) * zoom + pan.x;
            position.y = 200 + depth * 84 * zoom + pan.y;
            order++;
            place(frame, node.rightId, depth + 1, order, positions, count, zoom, pan);
            return;
        }
    }
}

sf::Vector2f TreeRenderer::locate(const Position* positions, int count, int id, sf::Vector2f fallback) const
{
    for (int i = 0; i < count; i++)
    {
        if (positions[i].id == id)
        {
            return sf::Vector2f(positions[i].x, positions[i].y);
        }
    }
    return fallback;
}

void TreeRenderer::draw(sf::RenderWindow& window, const sf::Font& font,
                         const AnimationController& animation, float zoom, sf::Vector2f pan,
                         sf::Vector2f mouse, int selectedId, const IntBuffer* selected) const
{
    const BackendFrame& before = animation.getFrom().model;
    const BackendFrame& after = animation.getTo().model;
    Position* oldPositions = new Position[before.nodeCount];
    Position* newPositions = new Position[after.nodeCount];
    int count = 0;
    int order = 0;
    place(before, before.rootId, 0, order, oldPositions, count, zoom, pan);
    int newCount = 0;
    order = 0;
    place(after, after.rootId, 0, order, newPositions, newCount, zoom, pan);
    float progress = animation.isFinished() ? 1 : static_cast<float>(animation.getProgress());
    float ease = progress * progress * (3 - 2 * progress);
    const GameEvent* event = animation.getEvent();
    for (int pass = 0; pass < 2; pass++)
    {
        for (int i = 0; i < after.nodeCount; i++)
        {
            const BackendNodeView& node = after.nodes[i];
            sf::Vector2f destination = locate(newPositions, newCount, node.id, sf::Vector2f(640, 200));
            sf::Vector2f original = locate(oldPositions, count, node.id, destination + sf::Vector2f(0, 24));
            sf::Vector2f position = original + (destination - original) * ease;
            if (pass == 0 && node.parentId != -1)
            {
                sf::Vector2f parentTo = locate(newPositions, newCount, node.parentId, position);
                sf::Vector2f parentFrom = locate(oldPositions, count, node.parentId, parentTo);
                sf::Vertex edge[2] = {sf::Vertex(position, sf::Color(207, 128, 143)),
                    sf::Vertex(parentFrom + (parentTo - parentFrom) * ease, sf::Color(207, 128, 143))};
                window.draw(edge, 2, sf::Lines);
            }
            if (pass == 1)
            {
                bool active = event != NULL && (event->nodeId == node.id || event->pivotId == node.id || event->otherNodeId == node.id);
                bool chosen = node.id == selectedId;
                if (selected != NULL)
                {
                    for (int j = 0; j < selected->getCount(); j++)
                    {
                        chosen = chosen || selected->get(j) == node.id;
                    }
                }
                float radius = 22 * zoom;
                bool existed = false;
                for (int j = 0; j < before.nodeCount; j++)
                {
                    existed = existed || before.nodes[j].id == node.id;
                }
                if (!existed)
                {
                    radius *= 0.6F + 0.4F * ease + 0.12F * std::sin(ease * 3.14159265F);
                }
                if (active)
                {
                    radius *= 1 + 0.12F * std::sin(progress * 3.14159265F);
                }
                sf::CircleShape circle(radius);
                circle.setOrigin(radius, radius);
                circle.setPosition(position);
                bool skipped = active && event->type == "HIGHLIGHT_SUBTREE" && event->detail == "SKIPPED";
                bool visiting = active && (event->type == "VISIT" || event->type == "COMPARE");
                int leftHeight = 0;
                int rightHeight = 0;
                for (int j = 0; j < after.nodeCount; j++)
                {
                    if (after.nodes[j].id == node.leftId)
                    {
                        leftHeight = after.nodes[j].height;
                    }
                    if (after.nodes[j].id == node.rightId)
                    {
                        rightHeight = after.nodes[j].height;
                    }
                }
                int balance = leftHeight - rightHeight;
                sf::Color base(255, 139, 174);
                if (animation.getTo().structure == "AVLTree")
                {
                    base = std::abs(balance) > 1 ? sf::Color(255, 128, 128) :
                           (balance == 0 ? sf::Color(160, 220, 169) : sf::Color(255, 220, 131));
                }
                circle.setFillColor(skipped ? sf::Color(197, 197, 202) :
                    (visiting ? sf::Color(132, 202, 246) : (active ? sf::Color(255, 196, 91) : base)));
                circle.setOutlineColor(sf::Color(125, 63, 97));
                circle.setOutlineThickness(active || chosen ? 4 : 2);
                if (chosen)
                {
                    circle.setOutlineColor(sf::Color(41, 111, 116));
                    Button::text(window, font, "SELECTED", position.x - 28, position.y - radius - 18, 11);
                }
                window.draw(circle);
                if (skipped)
                {
                    Button::text(window, font, "SKIPPED", position.x - 29, position.y + radius + 2, 11);
                }
                int labelValue = node.value;
                sf::Vector2f labelPosition = position;
                if (event != NULL && event->type == "SWAP_VALUE" &&
                    (node.id == event->nodeId || node.id == event->otherNodeId))
                {
                    int destinationId = node.id == event->nodeId ? event->otherNodeId : event->nodeId;
                    sf::Vector2f other = locate(newPositions, newCount, destinationId, position);
                    labelPosition += (other - position) * ease;
                    for (int j = 0; j < before.nodeCount; j++)
                    {
                        if (before.nodes[j].id == node.id)
                        {
                            labelValue = before.nodes[j].value;
                        }
                    }
                }
                std::string key = std::to_string(labelValue);
                Button::text(window, font, key, labelPosition.x - key.size() * 5, labelPosition.y - 13, 18);
                if (animation.getTo().structure == "AVLTree")
                {
                    Button::text(window, font, "bf " + std::to_string(balance) + " / sz " + std::to_string(node.subtreeSize), position.x - 38, position.y + radius + 2, 11);
                }
                if (std::abs(mouse.x - position.x) < radius && std::abs(mouse.y - position.y) < radius)
                {
                    int leftHeight = 0;
                    int rightHeight = 0;
                    for (int j = 0; j < after.nodeCount; j++)
                    {
                        if (after.nodes[j].id == node.leftId)
                        {
                            leftHeight = after.nodes[j].height;
                        }
                        if (after.nodes[j].id == node.rightId)
                        {
                            rightHeight = after.nodes[j].height;
                        }
                    }
                    std::ostringstream tip;
                    int depth = static_cast<int>((destination.y - 200 - pan.y) / (84 * zoom) + 0.5F);
                    tip << "id " << node.id << " | h " << node.height << " | bf " << leftHeight - rightHeight << " | size " << node.subtreeSize << " | depth " << depth;
                    Button::panel(window, sf::FloatRect(mouse.x + 12, mouse.y - 38, 370, 32), sf::Color(255, 249, 222), 10);
                    Button::text(window, font, tip.str(), mouse.x + 20, mouse.y - 34, 15);
                }
            }
        }
    }
    // Deleted IDs remain visible only in the playback layer while fading out.
    for (int i = 0; i < before.nodeCount; i++)
    {
        bool retained = false;
        for (int j = 0; j < after.nodeCount; j++)
        {
            retained = retained || before.nodes[i].id == after.nodes[j].id;
        }
        if (!retained && progress < 1)
        {
            sf::Vector2f position = locate(oldPositions, count, before.nodes[i].id, sf::Vector2f(640, 200));
            float radius = 22 * zoom * (1 - ease);
            sf::CircleShape circle(radius);
            circle.setOrigin(radius, radius);
            circle.setPosition(position);
            circle.setFillColor(sf::Color(255, 139, 174, static_cast<sf::Uint8>(255 * (1 - ease))));
            window.draw(circle);
        }
    }
    delete[] oldPositions;
    delete[] newPositions;
}
