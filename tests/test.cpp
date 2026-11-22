#include <SFML/Graphics.hpp>

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    static_assert(SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR == 6 && SFML_VERSION_PATCH == 1,
                  "Vault Heist requires the approved SFML 2.6.1 installation");
    sf::RenderWindow window(sf::VideoMode(640, 360), "Vault Heist - SFML Test");
    window.setFramerateLimit(60);

    sf::CircleShape marker(60.0F);
    marker.setFillColor(sf::Color(77, 166, 255));
    marker.setPosition(260.0F, 130.0F);

    const bool autoClose = argc > 1 && std::string(argv[1]) == "--auto-close";
    sf::Clock testClock;

    std::cout << "SFML window opened successfully. Close the window to finish the test.\n";

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        if (autoClose && testClock.getElapsedTime().asSeconds() >= 2.0F)
        {
            window.close();
        }

        if (!window.isOpen())
        {
            break;
        }

        window.clear(sf::Color(20, 24, 35));
        window.draw(marker);
        window.display();
    }

    std::cout << "SFML test passed.\n";
    return 0;
}
