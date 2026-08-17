#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <math.h>
#include <algorithm>
#include "../MAPS/Tiled.cpp"
#include "main_char.cpp"
#include "npc.cpp"

bool isFullScreen = false;

void toggleFullScreen(sf::RenderWindow& window, bool fullscreen, sf::View& uiView)
{
    if (fullscreen)
    {
        window.create(sf::VideoMode::getDesktopMode(), "Lost In Nust", sf::Style::Fullscreen);
    }
    else
    {
        window.create(sf::VideoMode(1920, 1080), "Lost in Nust", sf::Style::Default);
    }
    window.setVerticalSyncEnabled(true);
    window.setView(uiView);
}

struct Checkbox
{
    sf::RectangleShape box;
    sf::RectangleShape checkMark;
    bool checked = false;

    void setup(sf::Vector2f centerPos, float size)
    {
        box.setSize(sf::Vector2f(size, size));
        box.setOrigin(size / 2.f, size / 2.f);
        box.setPosition(centerPos);
        box.setFillColor(sf::Color(210, 180, 140));
        box.setOutlineThickness(3.f);
        box.setOutlineColor(sf::Color(60, 40, 20));

        float inner = size * 0.6f;
        checkMark.setSize(sf::Vector2f(inner, inner));
        checkMark.setOrigin(inner / 2.f, inner / 2.f);
        checkMark.setPosition(centerPos);
        checkMark.setFillColor(sf::Color(60, 40, 20));
    }

    void handleEvent(sf::RenderWindow& window, sf::Event& event)
    {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            if (box.getGlobalBounds().contains(mousePos))
                checked = !checked;
        }
    }

    void draw(sf::RenderWindow& window)
    {
        window.draw(box);
        if (checked)
            window.draw(checkMark);
    }
};

struct Slider
{
    sf::RectangleShape track;
    sf::CircleShape handle;
    float minValue = 0.f, maxValue = 100.f, value = 50.f;
    float width = 0.f;
    bool dragging = false;

    void setup(sf::Vector2f centerPos, float trackWidth)
    {
        width = trackWidth;

        track.setSize(sf::Vector2f(width, 6.f));
        track.setOrigin(0.f, 3.f);
        track.setPosition(centerPos.x - width / 2.f, centerPos.y);
        track.setFillColor(sf::Color(60, 40, 20));

        handle.setRadius(12.f);
        handle.setOrigin(12.f, 12.f);
        handle.setFillColor(sf::Color(210, 180, 140));

        updateHandlePosition();
    }

    void updateHandlePosition()
    {
        float t = (value - minValue) / (maxValue - minValue);
        float x = track.getPosition().x + t * width;
        handle.setPosition(x, track.getPosition().y);
    }

    void handleEvent(sf::RenderWindow& window, sf::Event& event)
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            if (handle.getGlobalBounds().contains(mousePos))
                dragging = true;
        }
        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
        {
            dragging = false;
        }
        if (event.type == sf::Event::MouseMoved && dragging)
        {
            float left = track.getPosition().x;
            float right = left + width;
            float x = std::max(left, std::min(right, mousePos.x));

            float t = (x - left) / width;
            value = minValue + t * (maxValue - minValue);
            updateHandlePosition();
        }
    }

    void draw(sf::RenderWindow& window)
    {
        window.draw(track);
        window.draw(handle);
    }
};

class RoundedRectangleShape : public sf::Shape
{
public:
    RoundedRectangleShape(const sf::Vector2f& size = sf::Vector2f(0, 0), float radius = 0, unsigned int cornerPointCount = 12)
        : m_size(size), m_radius(radius), m_cornerPointCount(cornerPointCount)
    {
        update();
    }

    void setSize(const sf::Vector2f& size)
    {
        m_size = size;
        update();
    }

    void setCornerRadius(float radius)
    {
        m_radius = radius;
        update();
    }

    void setCornerPointCount(unsigned int count)
    {
        m_cornerPointCount = count;
        update();
    }

    sf::Vector2f getSize() const { return m_size; }

    virtual std::size_t getPointCount() const override
    {
        return m_cornerPointCount * 4;
    }

    virtual sf::Vector2f getPoint(std::size_t index) const override
    {
        static const float pi = 3.141592654f;

        unsigned int cornerIndex = static_cast<unsigned int>(index) / m_cornerPointCount;
        unsigned int localIndex = static_cast<unsigned int>(index) % m_cornerPointCount;

        float deltaAngle = 90.0f / (m_cornerPointCount - 1);

        float angleDeg = localIndex * deltaAngle + cornerIndex * 90.0f;
        float angleRad = angleDeg * pi / 180.0f;

        sf::Vector2f center;
        switch (cornerIndex)
        {
            case 0: center = sf::Vector2f(m_size.x - m_radius, m_radius); break;
            case 1: center = sf::Vector2f(m_radius, m_radius); break;
            case 2: center = sf::Vector2f(m_radius, m_size.y - m_radius); break;
            case 3: center = sf::Vector2f(m_size.x - m_radius, m_size.y - m_radius); break;
        }

        return sf::Vector2f(
            center.x + m_radius * std::cos(angleRad),
            center.y - m_radius * std::sin(angleRad)
        );
    }

private:
    sf::Vector2f m_size;
    float m_radius;
    unsigned int m_cornerPointCount;
};

enum GameState
{
    Menu,
    Setting,
    CharacterIntro,
    playing
};
GameState currentState = GameState::Menu;

struct Button
{
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Vector2f baseScale;
    float currentScaleMultiplier = 1.0f;
    float targetScaleMultiplier = 1.0f;

    bool loadFromFile(const std::string& path, sf::Vector2f position, sf::Vector2f size)
    {
        if (!texture.loadFromFile(path))
        {
            std::cerr << "Error : The file failed to load " << std::endl;
            return false;
        }
        sprite.setTexture(texture);

        sf::Vector2u texSize(texture.getSize());
        sprite.setOrigin(texSize.x / 2.f, texSize.y / 2.f);
        sprite.setPosition(position);

        baseScale = sf::Vector2f(size.x / texSize.x, size.y / texSize.y);
        sprite.setScale(baseScale);
        return true;
    }

    bool isClicked(sf::RenderWindow& window, sf::Event& event)
    {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            return sprite.getGlobalBounds().contains(mousePos);
        }
        return false;
    }

    void updateHover(sf::RenderWindow& window, float deltaTime)
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        bool hovering = sprite.getGlobalBounds().contains(mousePos);

        targetScaleMultiplier = hovering ? 1.15f : 1.0f;

        float speed = 3.0f;
        currentScaleMultiplier += (targetScaleMultiplier - currentScaleMultiplier) * speed * deltaTime;

        sprite.setScale(baseScale.x * currentScaleMultiplier, baseScale.y * currentScaleMultiplier);
    }

    void drawSprite(sf::RenderWindow& window)
    {
        window.draw(sprite);
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Lost In NUST");
    std::cout << "Window created successfully!" << std::endl;

    sf::Texture texture;
    if (!texture.loadFromFile("/home/wasay/Myprojects/GAME/BACKGROUND-IMAGE/NUST Background.png"))
    {
        std::cerr << "Error: Failed to load image" << std::endl;
        return -1;
    }

    sf::Sprite sprite;
    sprite.setTexture(texture);
    sf::Vector2u bgSize = texture.getSize();
    sprite.setScale(
        static_cast<float>(window.getSize().x) / bgSize.x,
        static_cast<float>(window.getSize().y) / bgSize.y
    );

    sf::Clock clock;

    Button startButton, exitButton, settingButton, backButton, fullScreen, gameInstruction, masterVolume;
    startButton.loadFromFile(
        "/home/wasay/Myprojects/GAME/Buttons/Start button.png",
        sf::Vector2f(960, 400), sf::Vector2f(430, 160)
    );
    exitButton.loadFromFile(
        "/home/wasay/Myprojects/GAME/Buttons/Exit button.png",
        sf::Vector2f(960, 600), sf::Vector2f(430, 160)
    );
    settingButton.loadFromFile(
        "/home/wasay/Myprojects/GAME/Buttons/Setting Button.png",
        sf::Vector2f(960, 800), sf::Vector2f(160, 160)
    );
    backButton.loadFromFile(
        "/home/wasay/Myprojects/GAME/Buttons/Back Button.png",
        sf::Vector2f(120, 50), sf::Vector2f(200, 80)
    );
    fullScreen.loadFromFile(
        "/home/wasay/Myprojects/GAME/Buttons/FullScreen Button.png",
        sf::Vector2f(750, 350), sf::Vector2f(290, 130)
    );
    masterVolume.loadFromFile(
        "/home/wasay/Myprojects/GAME/Buttons/Master Volume Button.png",
        sf::Vector2f(1150, 350), sf::Vector2f(290, 130)
    );
    gameInstruction.loadFromFile(
        "/home/wasay/Myprojects/GAME/Buttons/Game Instructions .png",
        sf::Vector2f(960, 650), sf::Vector2f(530, 290)
    );

    TileMap hostel_map;
    if (!hostel_map.load("/home/wasay/Myprojects/GAME/Maps/hostel_map.tmx"))
    {
        std::cerr << "Hostel map aint loading help me please" << std::endl;
    }

    // Build collision rectangles from the "Boundaries" and "Beds and stuff" layers.
    // Add more layer names here the same way if you want other furniture layers to be solid too.
    std::vector<sf::FloatRect> collisionRects = hostel_map.getCollisionRects("Boundaries");
    std::vector<sf::FloatRect> furnitureRects = hostel_map.getCollisionRects("Beds and stuff");
    collisionRects.insert(collisionRects.end(), furnitureRects.begin(), furnitureRects.end());

    AnimatedPlayer player;
    player.loadAllAnimations(
        "/home/wasay/Myprojects/GAME/Main-Char",
        sf::Vector2f(350, 350),
        sf::Vector2f(50, 50)
    );
    NPCCharacter senior;
    senior.loadAllAnimations(
    "/home/wasay/Myprojects/GAME/NPC/senior_uni", // reusing your existing character assets for now — point at a different folder once you have a second character's frames
    sf::Vector2f(350, 600),                   // starting position, somewhere else on the map
    sf::Vector2f(50, 50)
);

    float introElapsed = 0.f;

    std::vector<std::string> introLines = {
    "Hello guys, my name is Yenna.",
    "Please help me survive a day in NUST!"
    };
    int introLineIndex = 0;
    float charsPerSecond = 15.f; // typing speed inside the bubble

    sf::Font introFont;
    introFont.loadFromFile("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf");

    sf::Text bubbleText;
    bubbleText.setFont(introFont);
    bubbleText.setCharacterSize(10); // small — this renders inside the zoomed-in game view, not the UI view
    bubbleText.setFillColor(sf::Color::Black);

    RoundedRectangleShape speechBubble(sf::Vector2f(100.f, 40.f), 8.f);
    speechBubble.setFillColor(sf::Color(255, 255, 255));
    speechBubble.setOutlineThickness(2.f);
    speechBubble.setOutlineColor(sf::Color(60, 40, 20));

    std::vector<std::string> seniorDialogue = {
        "Senior: Oh hey, you're new here?",
        "Senior: Welcome to NUST, good luck surviving!"
    };
    int dialogueIndex = 0;
    float dialogueElapsed = 0.f;
    bool showingDialogue = false;
    float interactionRange = 60.f; // pixels — tune based on how close feels right

    Slider masterVolumeSlider;
    masterVolumeSlider.setup(sf::Vector2f(1150, 440), 175.f);

    Checkbox fullscreenCheckbox;
    fullscreenCheckbox.setup(sf::Vector2f(750, 440), 20.f);

    RoundedRectangleShape settingsBox(sf::Vector2f(800, 600), 30.f);
    settingsBox.setFillColor(sf::Color(101, 67, 33));
    settingsBox.setOrigin(settingsBox.getSize().x / 2.f, settingsBox.getSize().y / 2.f);
    settingsBox.setPosition(window.getSize().x / 2.f, window.getSize().y / 2.f);
    settingsBox.setOutlineThickness(4);
    settingsBox.setOutlineColor(sf::Color(60, 40, 20));

    bool wasChecked = fullscreenCheckbox.checked;

    sf::View uiView(sf::FloatRect(0, 0, 1920, 1080));
    sf::View gameView(sf::FloatRect(0, 0, 512, 288));
    window.setView(uiView);

    while (window.isOpen())
    {
        // recompute every frame, and clamp so a stall (e.g. window recreation) can't
        // cause a huge deltaTime spike that teleports the player / speeds up animation
        float deltaTime = clock.restart().asSeconds();
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            if (currentState == GameState::Menu)
            {
                if (startButton.isClicked(window, event))
                {
                    currentState = GameState::CharacterIntro;
                    introElapsed = 0.f;
                }
                if (settingButton.isClicked(window, event))
                    currentState = GameState::Setting;
                if (exitButton.isClicked(window, event))
                    window.close();
            }
            else if (currentState == GameState::Setting)
            {
                if (backButton.isClicked(window, event))
                    currentState = GameState::Menu;

                masterVolumeSlider.handleEvent(window, event);
                fullscreenCheckbox.handleEvent(window, event);
                if (fullscreenCheckbox.checked != wasChecked)
                {
                    isFullScreen = fullscreenCheckbox.checked;
                    toggleFullScreen(window, isFullScreen, uiView);

                    wasChecked = fullscreenCheckbox.checked;
                }
            }
            else if (currentState == GameState::CharacterIntro)
            {
                if (event.type == sf::Event::KeyPressed || event.type == sf::Event::MouseButtonPressed)
                {
                    int visibleChars = static_cast<int>(introElapsed * charsPerSecond);
                    bool doneTyping = visibleChars >= static_cast<int>(introLines[introLineIndex].size());

                    if (!doneTyping)
                    {
                        introElapsed = introLines[introLineIndex].size() / charsPerSecond + 0.01f;
                    }
                    else
                    {
                        introLineIndex++;
                        introElapsed = 0.f;
                        if (introLineIndex >= (int)introLines.size())
                            currentState = GameState::playing;
                    }
                }
            }
            else if (currentState == GameState::playing)
            {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::E)
                {
                    float dist = std::hypot(
                        player.getPosition().x - senior.getPosition().x,
                        player.getPosition().y - senior.getPosition().y
                    );

                    if (!showingDialogue && dist < interactionRange)
                    {
                        showingDialogue = true;
                        dialogueIndex = 0;
                        dialogueElapsed = 0.f;
                    }
                    else if (showingDialogue)
                    {
                        int visibleChars = static_cast<int>(dialogueElapsed * charsPerSecond);
                        bool doneTyping = visibleChars >= (int)seniorDialogue[dialogueIndex].size();

                        if (!doneTyping)
                            dialogueElapsed = seniorDialogue[dialogueIndex].size() / charsPerSecond + 0.01f;
                        else
                        {
                            dialogueIndex++;
                            dialogueElapsed = 0.f;
                            if (dialogueIndex >= (int)seniorDialogue.size())
                                showingDialogue = false;
                        }
                    }
                }
            }
        }

        window.clear(sf::Color::Black);

        if (currentState == GameState::Menu || currentState == GameState::Setting || currentState == GameState::CharacterIntro)
        {
            window.setView(uiView);
        }

        if (currentState == GameState::Menu)
        {
            window.draw(sprite);
            startButton.updateHover(window, deltaTime); 
            exitButton.updateHover(window, deltaTime);
            settingButton.updateHover(window, deltaTime);
            backButton.updateHover(window, deltaTime);
            startButton.drawSprite(window);
            exitButton.drawSprite(window);
            settingButton.drawSprite(window);
        }
        else if (currentState == GameState::Setting)
        {
            window.draw(sprite);
            window.draw(settingsBox);
            backButton.drawSprite(window);
            backButton.updateHover(window, deltaTime);
            fullScreen.drawSprite(window);
            masterVolume.drawSprite(window);
            gameInstruction.drawSprite(window);

            masterVolumeSlider.draw(window);
            fullscreenCheckbox.draw(window);
        }
        else if (currentState == GameState::CharacterIntro)
        {
            introElapsed += deltaTime;

            // camera follows the player, same clamping as gameplay
            sf::Vector2f viewSize = gameView.getSize();
            sf::Vector2f center = player.getPosition();
            float mapWidth = 1024.f;
            float mapHeight = 1024.f;
            center.x = std::max(viewSize.x / 2.f, std::min(center.x, mapWidth - viewSize.x / 2.f));
            center.y = std::max(viewSize.y / 2.f, std::min(center.y, mapHeight - viewSize.y / 2.f));
            gameView.setCenter(center);
            window.setView(gameView);

            player.updateAnimation(deltaTime); // idle only, no movement during intro

            window.draw(hostel_map);
            player.draw(window);

            // typewriter reveal of the current line
            int visibleChars = static_cast<int>(introElapsed * charsPerSecond);
            visibleChars = std::min(visibleChars, static_cast<int>(introLines[introLineIndex].size()));
            std::string revealed = introLines[introLineIndex].substr(0, visibleChars);

            bubbleText.setString(revealed);
            sf::FloatRect textBounds = bubbleText.getLocalBounds();
            bubbleText.setOrigin(textBounds.width / 2.f, textBounds.height / 2.f);

            sf::Vector2f playerPos = player.getPosition();
            float bubbleWidth = std::max(textBounds.width + 20.f, 60.f);
            float bubbleHeight = textBounds.height + 20.f;
            sf::Vector2f bubbleCenter(playerPos.x, playerPos.y - 40.f); // above her head

            speechBubble.setSize(sf::Vector2f(bubbleWidth, bubbleHeight));
            speechBubble.setOrigin(bubbleWidth / 2.f, bubbleHeight / 2.f);
            speechBubble.setPosition(bubbleCenter);

            bubbleText.setPosition(bubbleCenter.x, bubbleCenter.y - 2.f);

            window.draw(speechBubble);
            window.draw(bubbleText);
        }
       else // Playing state
        {
            if(!showingDialogue)
            {
                senior.update(deltaTime,collisionRects);
            }
            std::vector<sf::FloatRect> playerCollisionRects = collisionRects;
            playerCollisionRects.push_back(senior.getCollisionBox());

            if (!showingDialogue)
                player.handleMovement(deltaTime, playerCollisionRects);
            player.updateAnimation(deltaTime);

            sf::Vector2f viewSize = gameView.getSize();
            sf::Vector2f center = player.getPosition();
            float mapWidth = 1024.f;
            float mapHeight = 1024.f;
            center.x = std::max(viewSize.x / 2.f, std::min(center.x, mapWidth - viewSize.x / 2.f));
            center.y = std::max(viewSize.y / 2.f, std::min(center.y, mapHeight - viewSize.y / 2.f));
            gameView.setCenter(center);
            window.setView(gameView);

            window.draw(hostel_map);
            player.draw(window);
            senior.draw(window);

            if (showingDialogue)
            {
                dialogueElapsed += deltaTime;

                int visibleChars = static_cast<int>(dialogueElapsed * charsPerSecond);
                visibleChars = std::min(visibleChars, (int)seniorDialogue[dialogueIndex].size());
                std::string revealed = seniorDialogue[dialogueIndex].substr(0, visibleChars);

                bubbleText.setString(revealed);
                sf::FloatRect textBounds = bubbleText.getLocalBounds();
                bubbleText.setOrigin(textBounds.width / 2.f, textBounds.height / 2.f);

                sf::Vector2f seniorPos = senior.getPosition();
                float bubbleWidth = std::max(textBounds.width + 20.f, 60.f);
                float bubbleHeight = textBounds.height + 20.f;
                sf::Vector2f bubbleCenter(seniorPos.x, seniorPos.y - 40.f);

                speechBubble.setSize(sf::Vector2f(bubbleWidth, bubbleHeight));
                speechBubble.setOrigin(bubbleWidth / 2.f, bubbleHeight / 2.f);
                speechBubble.setPosition(bubbleCenter);
                bubbleText.setPosition(bubbleCenter.x, bubbleCenter.y - 2.f);

                window.draw(speechBubble);
                window.draw(bubbleText);
            }
        }

        window.display();
    }
    return 0;
}