#include "frontend/Button.h"
#include <cmath>
#include <sstream>

void Button::wrapped(sf::RenderTarget& target, const sf::Font& font, const std::string& label,
                      float x, float y, float width, unsigned int size)
{
    std::istringstream paragraphs(label);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph))
    {
        std::istringstream words(paragraph);
        std::string word;
        std::string line;
        while (words >> word)
        {
            std::string candidate = line.empty() ? word : line + " " + word;
            sf::Text measure(sf::String::fromUtf8(candidate.begin(), candidate.end()), font, size);
            if (!line.empty() && measure.getLocalBounds().width > width)
            {
                text(target, font, line, x, y, size);
                y += size * 1.45F;
                line = word;
            }
            else
            {
                line = candidate;
            }
        }
        text(target, font, line, x, y, size);
        y += size * 1.45F;
    }
}

void Button::panel(sf::RenderTarget& target, const sf::FloatRect& bounds, sf::Color color, float radius)
{
    sf::ConvexShape shape;
    shape.setPointCount(32);
    for (int corner = 0; corner < 4; corner++)
    {
        float x = bounds.left + (corner == 0 || corner == 3 ? radius : bounds.width - radius);
        float y = bounds.top + (corner < 2 ? radius : bounds.height - radius);
        for (int step = 0; step < 8; step++)
        {
            float angle = (180.0F + corner * 90.0F + step * 90.0F / 7.0F) * 3.14159265F / 180.0F;
            shape.setPoint(corner * 8 + step, sf::Vector2f(x + std::cos(angle) * radius, y + std::sin(angle) * radius));
        }
    }
    shape.setFillColor(color);
    target.draw(shape);
}

void Button::text(sf::RenderTarget& target, const sf::Font& font, const std::string& label,
                   float x, float y, unsigned int size, sf::Color color)
{
    sf::Text text(sf::String::fromUtf8(label.begin(), label.end()), font, size);
    text.setFillColor(color);
    text.setPosition(x, y);
    target.draw(text);
}

bool Button::draw(sf::RenderWindow& window, const sf::Font& font, const std::string& label,
                   const sf::FloatRect& bounds, sf::Vector2f mouse, bool clicked, bool enabled)
{
    bool hover = bounds.contains(mouse);
    sf::FloatRect shadow = bounds;
    shadow.top += 5;
    panel(window, shadow, sf::Color(205, 133, 149));
    sf::Color fill = !enabled ? sf::Color(218, 208, 215) :
                     (hover ? sf::Color(255, 196, 91) : sf::Color(255, 226, 123));
    panel(window, bounds, fill);
    sf::Text caption(sf::String::fromUtf8(label.begin(), label.end()), font, 19);
    caption.setFillColor(sf::Color(73, 43, 74));
    sf::FloatRect measure = caption.getLocalBounds();
    caption.setPosition(bounds.left + (bounds.width - measure.width) / 2 - measure.left,
                        bounds.top + (bounds.height - measure.height) / 2 - measure.top);
    window.draw(caption);
    return enabled && hover && clicked;
}
