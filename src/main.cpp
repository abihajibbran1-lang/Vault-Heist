#include "frontend/UIManager.h"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char* argv[])
{
    static_assert(SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR == 6 && SFML_VERSION_PATCH == 1,
                  "Vault Heist requires SFML 2.6.1");
    bool smokeBriefing = argc >= 5 && std::string(argv[1]) == "--smoke-briefing";
    bool smokeEvidence = argc >= 6 && std::string(argv[1]) == "--capture-evidence";
    bool smokeCampaign = smokeEvidence || smokeBriefing || (argc >= 5 && std::string(argv[1]) == "--smoke-campaign");
    bool smokeEndless = argc > 1 && std::string(argv[1]) == "--smoke-endless";
    bool smokeMenu = argc > 1 && std::string(argv[1]) == "--smoke-test";
    UIManager ui(smokeCampaign ? argv[4] : (smokeEndless || smokeMenu ?
                 (argc >= 3 ? argv[2] : "build/smoke-saves") : "data/saves"));
    if (!ui.initialize())
    {
        std::cerr << "No readable font found. Place DejaVuSans.ttf in assets/fonts/.\n";
        return 1;
    }
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Vault Heist - The Living Vault");
    window.setFramerateLimit(60);
    sf::Clock frameClock;
    sf::Clock smokeClock;
    bool smokeTest = smokeCampaign || smokeEndless || (argc > 1 && std::string(argv[1]) == "--smoke-test");
    if (smokeCampaign && !ui.openCampaign(std::atoi(argv[2]), std::atoi(argv[3]), smokeBriefing))
    {
        std::cerr << "Campaign smoke-test level could not be loaded.\n";
        return 1;
    }
    if (smokeEndless)
    {
        ui.openEndless();
    }
    if (smokeEvidence && !ui.prepareEvidence(argv[5]))
    {
        std::cerr << "Evidence capture workflow failed.\n";
        return 1;
    }
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            ui.handle(event, window);
        }
        if (!window.isOpen())
        {
            break;
        }
        ui.update(frameClock.restart().asSeconds());
        ui.draw(window);
        if (smokeTest && smokeClock.getElapsedTime().asSeconds() >= 2)
        {
            sf::Texture capture;
            if (capture.create(window.getSize().x, window.getSize().y))
            {
                capture.update(window);
                std::string capturePath = smokeCampaign ? std::string(smokeBriefing ? "build/briefing-" : "build/campaign-") + argv[2] + "-" + argv[3] + ".png" :
                                          (smokeEndless ? "build/endless-smoke.png" : "build/menu-smoke.png");
                if (smokeEvidence)
                {
                    capturePath = "build/evidence-" + std::string(argv[5]) + ".png";
                }
                if (!capture.copyToImage().saveToFile(capturePath))
                {
                    std::cerr << "Could not save smoke-test capture.\n";
                    return 1;
                }
            }
            window.close();
        }
    }
    std::cout << "Vault Heist closed normally.\n";
    return 0;
}
