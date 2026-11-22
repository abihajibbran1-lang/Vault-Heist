#include "frontend/UIManager.h"
#include "frontend/Button.h"
#include <sstream>
#include <ctime>
#include <cmath>
#include <fstream>

UIManager::UIManager(const std::string& saveDirectory)
    : controller(saveDirectory), zoom(1), clicked(false), dragging(false), editingInput(false), muted(true), speed(1), sandboxOperation(0),
      nextSeed(static_cast<std::uint32_t>(std::time(NULL)))
{
}

bool UIManager::openCampaign(int act, int level, bool briefing)
{
    if (!controller.loadLevel(act, level))
    {
        return false;
    }
    screen.name = briefing ? "BRIEFING" : "CAMPAIGN";
    fitTree();
    return true;
}

bool UIManager::initialize()
{
    const char* candidates[4] = {"assets/fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/mnt/c/Windows/Fonts/arial.ttf"};
    for (int i = 0; i < 4; i++)
    {
        std::ifstream available(candidates[i], std::ios::binary);
        if (available && font.loadFromFile(candidates[i]))
        {
            return true;
        }
    }
    return false;
}

void UIManager::startEndless()
{
    nextSeed = nextSeed * 1664525u + 1013904223u;
    if (controller.startEndless(nextSeed))
    {
        screen.name = "ENDLESS";
        zoom = 1;
        pan = sf::Vector2f();
        controller.animation().setSpeed(speed);
        notice.clear();
    }
    else
    {
        notice = controller.getState().message;
    }
}

void UIManager::openEndless()
{
    startEndless();
}

void UIManager::operation(const std::string& name, const std::string& detail)
{
    if (controller.getState().mode == "CAMPAIGN")
    {
        PlayerAction action;
        action.name = name;
        action.text = detail;
        std::istringstream numeric(input);
        if (controller.getCampaignKind().find("HEAP_") == 0 &&
            (name == "HEAPIFY" || name == "SIFT_UP" || name == "REPAIR_SWAP") && !input.empty())
        {
            if (!(numeric >> action.value) || (name == "REPAIR_SWAP" && !(numeric >> action.otherValue)))
            {
                notice = "Enter the index, or two indices for Swap";
                return;
            }
            action.text = "INPUT";
            action.indexA = action.value;
            action.indexB = action.otherValue;
        }
        if (name == "SUBMIT" && !(numeric >> action.value))
        {
            notice = "Click Input and enter a number first";
            return;
        }
        if (name == "SUBMIT" && controller.getCampaignKind() == "RANGE" && !(numeric >> action.otherValue))
        {
            notice = "Enter both lower and upper bounds";
            return;
        }
        notice = controller.performAction(action).message;
        return;
    }
    PlayerAction action;
    action.name = name;
    action.text = detail;
    std::istringstream values(input);
    values >> action.value >> action.otherValue >> action.nodeId;
    action.indexA = action.value;
    action.indexB = action.otherValue;
    if (name == "BUILD_PRE_IN" || name == "BUILD_POST_IN" || name == "EQUAL")
    {
        action.text = input;
    }
    if (name == "PREORDER" || name == "INORDER" || name == "POSTORDER" || name == "LEVELORDER")
    {
        action.name = "TRAVERSE";
        action.text = name;
    }
    if (name == "ADD_RIGHT")
    {
        action.name = "INSERT";
        action.text = "RIGHT";
    }
    if (name == "ROTATE_LL" || name == "ROTATE_RR" || name == "ROTATE_LR" || name == "ROTATE_RL")
    {
        action.name = "ROTATE";
        action.text = name.substr(7);
        action.nodeId = action.value;
    }
    if (name == "SELECT_ROTATION" || name == "TRAVERSE" || name == "MIRROR" || name == "VALIDATE" ||
        name == "HEIGHT" || name == "DIAMETER" || name == "SYMMETRY" || name == "HEAP_SORT" || name == "EXTRACT_MAX" ||
        name == "GENERATE" || name == "BUILD_HEAP" || name == "SORT_STEP" || name == "MIN" || name == "MAX" || action.name == "TRAVERSE")
    {
        notice = controller.performAction(action).message;
        return;
    }
    std::istringstream check(input);
    int required = 0;
    if (!(check >> required))
    {
        notice = "Enter an integer first";
        return;
    }
    notice = controller.performAction(action).message;
}

void UIManager::handle(const sf::Event& event, sf::RenderWindow& window)
{
    if (event.type == sf::Event::Closed)
    {
        if (controller.getState().mode == "ENDLESS")
        {
            controller.pause();
            controller.saveEndless();
        }
        window.close();
    }
    else if (event.type == sf::Event::LostFocus)
    {
        controller.setFocused(false);
    }
    else if (event.type == sf::Event::GainedFocus)
    {
        controller.setFocused(true);
    }
    else if (event.type == sf::Event::Resized)
    {
        window.setView(sf::View(sf::FloatRect(0, 0, 1280, 720)));
    }
    else if (event.type == sf::Event::MouseButtonPressed)
    {
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            clicked = true;
            sf::Vector2f point = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
            editingInput = (screen.name == "SANDBOX" || screen.name == "CAMPAIGN") && sf::FloatRect(45, 570, 1190, 42).contains(point);
            if (screen.name == "CAMPAIGN" && controller.getState().status == "PLAYING")
            {
                if (controller.getCampaignKind().find("HEAP_") == 0)
                {
                    int index = heap.pick(controller.getSnapshot().model, point);
                    if (index != -1)
                    {
                        input.clear();
                        PlayerAction action;
                        action.name = "SELECT_HEAP_INDEX";
                        action.nodeId = index;
                        notice = controller.performAction(action).message;
                    }
                    return;
                }
                if (controller.getCampaignKind() == "AVL_SINGLE" || controller.getCampaignKind() == "AVL_DOUBLE")
                {
                    draggedSubtree = tree.pick(controller.getSnapshot().model, zoom, pan, point);
                    return;
                }
                if (controller.getCampaignKind() == "RECONSTRUCT")
                {
                    GameSnapshot target = controller.blueprint();
                    for (int i = 0; i < target.model.nodeCount; i++)
                    {
                        bool placed = false;
                        for (int j = 0; j < controller.getState().answers.getCount(); j++)
                        {
                            placed = placed || controller.getState().answers.get(j) == target.model.nodes[i].id;
                        }
                        if (!placed && sf::FloatRect(100.0F + i * 140, 500, 112, 44).contains(point))
                        {
                            draggedToken = i;
                        }
                    }
                    return;
                }
                int id = tree.pick(controller.getSnapshot().model, zoom, pan, point);
                if (id != -1)
                {
                    PlayerAction action;
                    action.name = "SELECT_NODE";
                    action.nodeId = id;
                    notice = controller.performAction(action).message;
                }
            }
        }
        if (event.mouseButton.button == sf::Mouse::Middle)
        {
            dragging = true;
            lastMouse = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && draggedSubtree != -1)
    {
        sf::Vector2f point = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
        int pivot = tree.pick(controller.getSnapshot().model, zoom, pan, point);
        if (pivot != -1)
        {
            PlayerAction action;
            action.name = "SELECT_PIVOT";
            action.nodeId = pivot;
            notice = controller.performAction(action).message;
        }
        draggedSubtree = -1;
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && draggedToken != -1)
    {
        GameSnapshot target = controller.blueprint();
        sf::Vector2f point = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
        PlayerAction action;
        action.name = "PLACE_TOKEN";
        action.nodeId = tree.pick(target.model, zoom, pan, point);
        action.value = target.model.nodes[draggedToken].value;
        if (action.nodeId != -1)
        {
            notice = controller.performAction(action).message;
        }
        draggedToken = -1;
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Middle)
    {
        dragging = false;
    }
    else if (event.type == sf::Event::MouseMoved && dragging)
    {
        sf::Vector2f now = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
        pan += now - lastMouse;
        lastMouse = now;
    }
    else if (event.type == sf::Event::MouseWheelScrolled)
    {
        zoom *= event.mouseWheelScroll.delta > 0 ? 1.12F : 0.89F;
        if (zoom < 0.15F)
        {
            zoom = 0.15F;
        }
        if (zoom > 2)
        {
            zoom = 2;
        }
    }
    else if (event.type == sf::Event::TextEntered && (screen.name == "SANDBOX" || screen.name == "CAMPAIGN") && editingInput)
    {
        unsigned int letter = event.text.unicode;
        if (letter == 8 && !input.empty())
        {
            input.erase(input.size() - 1);
        }
        else if ((letter == ' ' || letter == '-' || letter == '/' || (letter >= '0' && letter <= '9')) && input.size() < 300)
        {
            input += static_cast<char>(letter);
        }
    }
    else if (event.type == sf::Event::KeyPressed)
    {
        if (editingInput && event.key.code != sf::Keyboard::Escape)
        {
            if (event.key.code == sf::Keyboard::Enter)
            {
                editingInput = false;
            }
            return;
        }
        if (event.key.code == sf::Keyboard::Escape)
        {
            if (screen.name == "ENDLESS")
            {
                controller.pause();
            }
            else
            {
                screen.name = "MENU";
            }
        }
        else if (event.key.code == sf::Keyboard::Space)
        {
            if (controller.animation().isPlaying())
            {
                controller.animation().pause();
            }
            else
            {
                controller.animation().play();
            }
        }
        else if (event.key.code == sf::Keyboard::Right)
        {
            controller.animation().next();
        }
        else if (event.key.code == sf::Keyboard::Left)
        {
            controller.animation().previous();
        }
        else if (event.key.code == sf::Keyboard::F)
        {
            fitTree();
        }
        else if (event.key.code == sf::Keyboard::Z && (screen.name == "SANDBOX" || screen.name == "CAMPAIGN"))
        {
            controller.undo();
        }
        else if (event.key.code == sf::Keyboard::H && screen.name == "CAMPAIGN")
        {
            controller.requestHint(notice);
        }
    }
}

void UIManager::update(double seconds)
{
    controller.update(seconds);
}

void UIManager::fitTree()
{
    GameSnapshot current = controller.getSnapshot();
    zoom = 1;
    if (current.model.nodeCount > 0 && current.model.nodes[0].height > 4)
    {
        zoom = 250.0F / ((current.model.nodes[0].height - 1) * 84);
    }
    pan = sf::Vector2f();
}

bool UIManager::button(sf::RenderWindow& window, const std::string& text, float x, float y,
                       float width, sf::Vector2f mouse, bool enabled)
{
    return Button::draw(window, font, text, sf::FloatRect(x, y, width, 42), mouse, clicked, enabled);
}

void UIManager::draw(sf::RenderWindow& window)
{
    window.clear(sf::Color(255, 239, 227));
    sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    Button::panel(window, sf::FloatRect(28, 20, 1224, 92), sf::Color(255, 131, 160), 25);
    Button::text(window, font, "VAULT HEIST", 63, 31, 35);
    Button::text(window, font, "THE LIVING VAULT", 65, 78, 16);
    if (screen.name == "MENU")
    {
        Button::text(window, font, "A little prediction. A lot of possibility.", 405, 139, 23);
        if (button(window, "Campaign", 470, 204, 340, mouse))
        {
            screen.name = "CAMPAIGN_MAP";
        }
        if (button(window, "Resume Campaign", 470, 617, 340, mouse))
        {
            if (controller.resumeCampaign())
            {
                screen.name = "BRIEFING";
                fitTree();
            }
            notice = controller.getState().message;
        }
        if (button(window, "Endless AVL", 470, 263, 340, mouse))
        {
            startEndless();
        }
        if (button(window, "Resume Endless", 470, 322, 340, mouse))
        {
            if (controller.resumeEndless())
            {
                screen.name = "ENDLESS";
            }
            notice = controller.getState().message;
        }
        if (button(window, "Sandbox", 470, 381, 340, mouse))
        {
            controller.selectMode("SANDBOX");
            screen.name = "SANDBOX";
            notice = "Click Input to type values, choose an operation, then Run";
        }
        if (button(window, "Leaderboard", 470, 440, 340, mouse))
        {
            screen.name = "LEADERBOARD";
        }
        if (button(window, "Settings", 470, 499, 340, mouse))
        {
            screen.name = "SETTINGS";
        }
        if (button(window, "Quit", 470, 558, 340, mouse))
        {
            window.close();
        }
    }
    else if (screen.name == "CAMPAIGN_MAP")
    {
        Button::text(window, font, "CHOOSE YOUR WING", 400, 135, 28);
        const char* names[4] = {"Blueprint / Binary Tree", "Sorted / BST", "Architect / AVL", "Heap Fortress"};
        for (int act = 1; act <= 4; act++)
        {
            Button::text(window, font, names[act - 1], 55, 185.0F + (act - 1) * 117, 20);
            for (int level = 1; level <= 5; level++)
            {
                bool available = controller.levelAvailable(act, level);
                std::string label = std::to_string(act) + "." + std::to_string(level);
                int stars = controller.levelStars(act, level);
                label += stars > 0 ? " / " + std::to_string(stars) + " stars" : (available ? " / Enter" : " / Locked");
                if (button(window, label, 255.0F + (level - 1) * 192, 181.0F + (act - 1) * 117, 180, mouse, available))
                {
                    if (controller.loadLevel(act, level))
                    {
                        screen.name = "BRIEFING";
                        notice.clear();
                        input.clear();
                        fitTree();
                    }
                    else
                    {
                        notice = controller.getState().message;
                    }
                }
            }
        }
    }
    else if (screen.name == "BRIEFING")
    {
        const GameState& game = controller.getState();
        Button::panel(window, sf::FloatRect(100, 170, 1080, 490), sf::Color(255, 248, 230), 24);
        Button::text(window, font, std::to_string(game.act) + "." + std::to_string(game.level) + " / " + controller.getCampaignTitle(), 140, 191, 28);
        Button::text(window, font, "YOUR OBJECTIVE", 140, 248, 18);
        Button::wrapped(window, font, game.objective, 140, 280, 990, 19);
        Button::text(window, font, "HOW TO PLAY", 140, 345, 18);
        Button::wrapped(window, font, controller.briefingInstructions(), 140, 377, 990, 18);
        Button::text(window, font, "Undo: Z  |  Hint: H (caps stars at two)  |  Fit: F  |  Alarm limit: " + std::to_string(game.alarmCapacity), 140, 535, 16);
        if (button(window, "Okay, start level", 450, 590, 360, mouse))
        {
            screen.name = "CAMPAIGN";
            notice.clear();
        }
        if (button(window, "Back", 190, 590, 200, mouse))
        {
            screen.name = "CAMPAIGN_MAP";
        }
    }
    else if (screen.name == "CAMPAIGN")
    {
        const GameState& game = controller.getState();
        const std::string kind = controller.getCampaignKind();
        if (kind.find("HEAP_") == 0)
        {
            heap.draw(window, font, controller.animation(), &game.answers);
        }
        else if (kind == "RECONSTRUCT")
        {
            GameSnapshot target = controller.blueprint();
            tree.drawSlots(window, font, target.model, game.answers, zoom, pan);
            for (int i = 0; i < target.model.nodeCount; i++)
            {
                bool placed = false;
                for (int j = 0; j < game.answers.getCount(); j++)
                {
                    placed = placed || game.answers.get(j) == target.model.nodes[i].id;
                }
                if (!placed)
                {
                    sf::Vector2f position(100.0F + i * 140, 500);
                    if (i == draggedToken)
                    {
                        position = mouse - sf::Vector2f(56, 22);
                    }
                    Button::panel(window, sf::FloatRect(position.x, position.y, 112, 44), sf::Color(255, 218, 125), 12);
                    Button::text(window, font, std::to_string(target.model.nodes[i].value), position.x + 36, position.y + 8, 21);
                }
            }
        }
        else
        {
            tree.draw(window, font, controller.animation(), zoom, pan, mouse, game.selectedId, kind == "AVL_BADGES" ? &game.answers : NULL);
        }
        Button::text(window, font, std::to_string(game.act) + "." + std::to_string(game.level) + " " + controller.getCampaignTitle(), 60, 123, 23);
        Button::text(window, font, "Alarm " + std::to_string(game.alarmSteps) + "/" + std::to_string(game.alarmCapacity) +
                     "   Score " + std::to_string(game.score), 820, 125, 20);
        Button::text(window, font, controller.campaignInstruction(), 60, 158, 17);
        if (kind.find("HEAP_") == 0)
        {
            const char* operations[5] = {"BUILD_ACTIVE_HEAP", "HEAPIFY", "SIFT_UP", "REPAIR_SWAP", "EXTRACT_SORTED"};
            const char* labels[5] = {"Build Heap", "Heapify", "Sift Up", "Swap selected", "Extract Maximum"};
            for (int i = 0; i < 5; i++)
            {
                bool enabled = !(kind == "HEAP_REPAIR" && i == 0) && !((kind == "HEAP_BUILD" || kind == "HEAP_REPAIR") && i == 4);
                if (button(window, labels[i], 100.0F + i * 220, 448, 205, mouse, enabled))
                {
                    operation(operations[i]);
                }
            }
        }
        else if (kind == "AVL_SINGLE" || kind == "AVL_DOUBLE" || kind == "AVL_PREDICT")
        {
            if (game.choice.empty())
            {
                const char* cases[5] = {"NONE", "LL", "RR", "LR", "RL"};
                for (int i = 0; i < 5; i++)
                {
                    if (button(window, cases[i], 200.0F + i * 180, 500, 165, mouse))
                    {
                        operation("ROTATION_CASE", cases[i]);
                    }
                }
            }
            else
            {
                if (button(window, "Rotate Left", 390, 500, 230, mouse))
                {
                    operation("ROTATE_PRIMITIVE", "LEFT");
                }
                if (button(window, "Rotate Right", 660, 500, 230, mouse))
                {
                    operation("ROTATE_PRIMITIVE", "RIGHT");
                }
            }
        }
        else if (kind == "SEARCH" || kind == "INSERT_PATH")
        {
            if (button(window, "Left", 290, 500, 200, mouse))
            {
                operation("DIRECTION", "LEFT");
            }
            if (button(window, "Right", 530, 500, 200, mouse))
            {
                operation("DIRECTION", "RIGHT");
            }
            if (button(window, "Found", 770, 500, 200, mouse))
            {
                operation("DIRECTION", "FOUND");
            }
        }
        else if (kind == "DELETE_CASE")
        {
            const char* cases[3] = {"LEAF", "ONE_CHILD", "TWO_CHILDREN"};
            for (int i = 0; i < 3; i++)
            {
                if (button(window, cases[i], 280.0F + i * 250, 500, 230, mouse))
                {
                    operation("DELETE_CASE", cases[i]);
                }
            }
        }
        else if (kind == "MIRROR")
        {
            if (button(window, "Auto mirror", 510, 500, 260, mouse))
            {
                operation("AUTO");
            }
        }
        bool numeric = kind == "HEIGHTS" || kind == "LCA_PATH" || kind == "RANGE" || kind == "AVL_ORDER" || kind.find("HEAP_") == 0;
        if (numeric)
        {
            Button::panel(window, sf::FloatRect(45, 570, 1190, 42), sf::Color(255, 222, 202), 12);
            Button::text(window, font, "Input: " + input + (editingInput ? "_" : "  [click here to edit]"), 65, 578, 19);
            if (kind.find("HEAP_") != 0 && button(window, "Submit", 1010, 567, 185, mouse))
            {
                operation("SUBMIT");
            }
        }
        const GameEvent* event = controller.animation().getEvent();
        if (event != NULL)
        {
            Button::text(window, font, "Step " + std::to_string(controller.animation().getPosition() + 1) + ": " + event->type + " " + event->detail,
                         65, kind.find("HEAP_") == 0 ? 614.0F : 548.0F, kind.find("HEAP_") == 0 ? 12 : 15);
        }
        if (button(window, "Undo", 220, 630, 170, mouse))
        {
            controller.undo();
            notice = "Previous action restored";
        }
        if (button(window, "Replay", 420, 630, 170, mouse))
        {
            controller.replayLevel();
        }
        if (button(window, "Reset", 620, 630, 170, mouse))
        {
            controller.resetLevel();
            input.clear();
            notice.clear();
        }
        if (button(window, "Hint", 820, 630, 170, mouse))
        {
            controller.requestHint(notice);
        }
        if (button(window, "Save", 1030, 630, 170, mouse))
        {
            controller.saveCampaign();
            notice = controller.getState().message;
        }
        if ((game.completed || game.failed) && controller.animation().isFinished())
        {
            Button::panel(window, sf::FloatRect(260, 210, 760, 335), sf::Color(255, 248, 228), 24);
            Button::text(window, font, game.completed ? (game.act == 4 && game.level == 5 ? "FINAL VAULT UNLOCKED" : "WING CHALLENGE COMPLETE!") : "THE ALARM IS FULL", 360, 235, 28);
            Button::text(window, font, "Stars: " + std::to_string(game.stars) + "    Score: " + std::to_string(game.score) +
                         "    Penalties: " + std::to_string(game.penalties), 355, 290, 22);
            Button::text(window, font, "Steps " + std::to_string(game.alarmSteps) + " / optimal " + std::to_string(game.optimalSteps), 420, 335, 21);
            if (game.act == 2 && game.level == 5 && game.completed)
            {
                Button::text(window, font, "A sorted BST becomes a corridor. AVL rotations keep the next wing short.", 300, 375, 17);
            }
            if (game.act == 4 && game.completed)
            {
                std::string array = "Array: ";
                const BackendFrame& frame = controller.getSnapshot().model;
                for (int i = 0; i < frame.arraySize; i++)
                {
                    array += std::to_string(frame.values[i]) + " ";
                }
                Button::text(window, font, array, 330, 375, 18);
            }
            if (button(window, "Retry", 310, 438, 170, mouse))
            {
                controller.resetLevel();
                notice.clear();
                screen.name = "BRIEFING";
            }
            int nextAct = game.level == 5 ? game.act + 1 : game.act;
            int nextLevel = game.level == 5 ? 1 : game.level + 1;
            if (button(window, "Next Level", 505, 438, 190, mouse, game.completed && controller.levelAvailable(nextAct, nextLevel)))
            {
                if (controller.loadLevel(nextAct, nextLevel))
                {
                    screen.name = "BRIEFING";
                    input.clear();
                    notice.clear();
                    fitTree();
                }
            }
            if (button(window, "Campaign Map", 720, 438, 220, mouse))
            {
                screen.name = "CAMPAIGN_MAP";
                notice.clear();
            }
        }
    }
    else if (screen.name == "ENDLESS" || screen.name == "SANDBOX")
    {
        if (controller.getSnapshot().structure == "MaxHeap")
        {
            heap.draw(window, font, controller.animation());
        }
        else
        {
            tree.draw(window, font, controller.animation(), zoom, pan, mouse);
        }
        if (screen.name == "ENDLESS")
        {
            const EndlessState& run = controller.getEndless();
            double remaining = run.deadline - run.survivalSeconds;
            std::ostringstream hud;
            hud << "Survived " << static_cast<int>(run.survivalSeconds) << "s   |   Operations " << run.completedOperations;
            Button::text(window, font, hud.str(), 60, 126, 22);
            std::string request = run.phase == "WAITING_FOR_ANSWER" ? run.request + " " + std::to_string(run.key) : run.phase;
            Button::text(window, font, request, 940, 127, 21);
            Button::text(window, font, "Next arrival: " + std::to_string(static_cast<int>(std::ceil(remaining))) + "s", 975, 163, 18);
            Button::panel(window, sf::FloatRect(180, 579, 920, 71), sf::Color(255, 211, 202), 18);
            const std::string choices[5] = {"NONE", "LL", "RR", "LR", "RL"};
            for (int i = 0; i < 5; i++)
            {
                if (button(window, choices[i], 205.0F + i * 175, 592, 160, mouse, run.phase == "WAITING_FOR_ANSWER"))
                {
                    operation("SELECT_ROTATION", choices[i]);
                }
            }
            if (run.phase == "PAUSED")
            {
                Button::panel(window, sf::FloatRect(390, 222, 500, 290), sf::Color(255, 248, 228), 24);
                Button::text(window, font, "Take a breath. Time is frozen.", 430, 244, 23);
                if (button(window, "Resume", 470, 300, 340, mouse))
                {
                    controller.resume();
                }
                if (button(window, "Save and Exit", 470, 362, 340, mouse))
                {
                    if (controller.saveEndless())
                    {
                        controller.exitEndless();
                        screen.name = "MENU";
                    }
                    notice = controller.getState().message;
                }
                if (button(window, "Exit Run", 470, 424, 340, mouse))
                {
                    controller.exitEndless();
                    screen.name = "MENU";
                }
            }
            else if (run.phase == "FAILED" || run.phase == "ERROR")
            {
                Button::panel(window, sf::FloatRect(370, 213, 540, 294), sf::Color(255, 248, 228), 24);
                Button::text(window, font, "Survived: " + std::to_string(static_cast<int>(run.survivalSeconds)) + "s", 440, 236, 28);
                Button::text(window, font, "Operations completed: " + std::to_string(run.completedOperations), 440, 285, 22);
                Button::text(window, font, "Reason: " + run.reason, 440, 326, 20);
                if (controller.getLeaderboardCount() > 0)
                {
                    Button::text(window, font, "Local best: " + std::to_string(controller.getLeaderboardEntry(0).seconds) + "s", 440, 360, 18);
                }
                if (button(window, "Retry", 425, 392, 190, mouse))
                {
                    startEndless();
                }
                if (button(window, "Main Menu", 650, 392, 210, mouse))
                {
                    controller.exitEndless();
                    screen.name = "MENU";
                }
            }
        }
        else
        {
            const std::string structures[4] = {"BinaryTree", "BST", "AVLTree", "MaxHeap"};
            for (int i = 0; i < 4; i++)
            {
                if (button(window, structures[i], 50.0F + i * 200, 124, 185, mouse))
                {
                    controller.selectStructure(structures[i]);
                }
            }
            Button::panel(window, sf::FloatRect(45, 570, 1190, 112), sf::Color(255, 211, 202), 18);
            Button::text(window, font, "Input: " + input + (editingInput ? "_" : "  [click to edit]"), 65, 580, 20);
            const std::string operations[36] = {"INSERT", "DELETE", "SEARCH", "KTH", "RANK", "RANGE", "EXTRACT_MAX", "HEAP_SORT",
                "GENERATE", "PREORDER", "INORDER", "POSTORDER", "LEVELORDER", "HEIGHT", "DIAMETER", "MIRROR", "SYMMETRY",
                "MIN", "MAX", "SUCCESSOR", "PREDECESSOR", "COUNT_RANGE", "LCA", "PATH", "BUILD_PRE_IN", "BUILD_POST_IN", "EQUAL",
                "BUILD_HEAP", "HEAPIFY", "SIFT_UP", "HEAP_SWAP", "ADD_RIGHT", "ROTATE_LL", "ROTATE_RR", "ROTATE_LR", "ROTATE_RL"};
            if (button(window, "Previous", 55, 620, 130, mouse))
            {
                sandboxOperation = (sandboxOperation + 35) % 36;
            }
            if (button(window, operations[sandboxOperation] + "  >", 200, 620, 340, mouse))
            {
                sandboxOperation = (sandboxOperation + 1) % 36;
            }
            if (button(window, "Run", 555, 620, 125, mouse))
            {
                operation(operations[sandboxOperation]);
            }
            if (button(window, "Undo", 705, 620, 150, mouse))
            {
                notice = controller.undo() ? "Previous action restored" : "No previous action";
            }
            if (button(window, "Replay", 875, 620, 150, mouse))
            {
                controller.replayLevel();
            }
            if (button(window, "Reset", 1045, 620, 150, mouse))
            {
                controller.resetLevel();
            }
            if (button(window, "Inorder", 950, 125, 120, mouse))
            {
                operation("TRAVERSE", "INORDER");
            }
            if (button(window, "Validate", 1080, 125, 120, mouse))
            {
                operation("VALIDATE");
            }
        }
        const GameEvent* event = controller.animation().getEvent();
        if (event != NULL)
        {
            Button::text(window, font, "Event " + std::to_string(controller.animation().getPosition() + 1) + ": " + event->type + " " + event->detail, 65, 534, 16);
        }
    }
    else if (screen.name == "LEADERBOARD")
    {
        Button::text(window, font, "ENDLESS / LOCAL TOP 10", 365, 134, 28);
        for (int i = 0; i < controller.getLeaderboardCount(); i++)
        {
            const LeaderboardEntry& entry = controller.getLeaderboardEntry(i);
            std::string label = std::to_string(i + 1) + ".   " + std::to_string(entry.seconds) + "s     " + std::to_string(entry.operations) + " operations";
            Button::text(window, font, label, 440, 191.0F + i * 41, 22);
        }
    }
    else if (screen.name == "SETTINGS")
    {
        Button::text(window, font, "Make yourself comfortable", 404, 175, 28);
        if (button(window, muted ? "Sound: muted" : "Sound: on (no assets loaded)", 410, 258, 460, mouse))
        {
            muted = !muted;
        }
        if (button(window, "Animation speed: " + std::to_string(speed).substr(0, 4) + "x", 410, 327, 460, mouse))
        {
            speed = speed == 0.5 ? 1 : (speed == 1 ? 2 : 0.5);
            controller.animation().setSpeed(speed);
        }
        Button::text(window, font, "Space: playback pause | Arrows: step | Esc: pause/menu", 290, 420, 20);
        Button::text(window, font, "Wheel: zoom | Middle drag: pan | F: fit", 407, 462, 20);
    }
    if (screen.name != "MENU" && screen.name != "ENDLESS")
    {
        if (button(window, "Main Menu", 1065, 43, 155, mouse))
        {
            screen.name = "MENU";
        }
    }
    if (!notice.empty())
    {
        Button::text(window, font, notice.substr(0, 110), 45, 692, 16);
    }
    clicked = false;
    window.display();
}
