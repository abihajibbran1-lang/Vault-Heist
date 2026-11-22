#ifndef VAULT_BUTTON_H
#define VAULT_BUTTON_H

#include <SFML/Graphics.hpp>
#include <string>

class Button
{
public:
    static void wrapped(sf::RenderTarget& target, const sf::Font& font, const std::string& label,
                        float x, float y, float width, unsigned int size = 19);
    static void panel(sf::RenderTarget& target, const sf::FloatRect& bounds, sf::Color color, float radius = 16);
    static void text(sf::RenderTarget& target, const sf::Font& font, const std::string& label,
                     float x, float y, unsigned int size = 20, sf::Color color = sf::Color(73, 43, 74));
    static bool draw(sf::RenderWindow& window, const sf::Font& font, const std::string& label,
                     const sf::FloatRect& bounds, sf::Vector2f mouse, bool clicked, bool enabled = true);
};

#endif
