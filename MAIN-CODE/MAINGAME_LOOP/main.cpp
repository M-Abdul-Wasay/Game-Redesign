#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <vector>
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

void drawCenteredText(sf::RenderWindow& window, sf::Text& text, float x, float y, bool shadow = false) 
{
    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    
    if (shadow) {
        sf::Text shadowText = text;
        shadowText.setFillColor(sf::Color(20, 10, 5, 200));
        shadowText.setPosition(x + 3.f, y + 3.f);
        window.draw(shadowText);
    }
    
    text.setPosition(x, y);
    window.draw(text);
}

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

void drawInteractionBubble(sf::RenderWindow& window, sf::Vector2f pos, sf::Font& font, float time) 
{
    float bobOffset = std::sin(time * 3.f) * 3.f; 
    
    RoundedRectangleShape bubble(sf::Vector2f(36.f, 24.f), 8.f);
    bubble.setOrigin(18.f, 12.f);
    bubble.setPosition(pos.x, pos.y - 45.f + bobOffset);
    bubble.setFillColor(sf::Color(255, 255, 255, 230));
    
    sf::ConvexShape tail;
    tail.setPointCount(3);
    tail.setPoint(0, sf::Vector2f(pos.x - 4.f, pos.y - 35.f + bobOffset));
    tail.setPoint(1, sf::Vector2f(pos.x + 4.f, pos.y - 35.f + bobOffset));
    tail.setPoint(2, sf::Vector2f(pos.x, pos.y - 28.f + bobOffset));
    tail.setFillColor(sf::Color(255, 255, 255, 230));

    sf::Text dots("...", font, 20);
    dots.setFillColor(sf::Color(40, 40, 40));
    drawCenteredText(window, dots, pos.x, pos.y - 48.f + bobOffset);

    window.draw(tail);
    window.draw(bubble);
    window.draw(dots);
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

enum GameState
{
    Menu,
    Setting,
    CharacterIntro,
    playing,
    Minigame,
    Cutscene,
    GameOver
};
GameState currentState = GameState::Menu;

enum class ActiveMap { Hostel, C2, Seecs };

enum MinigameID
{
    None = -1,
    UniBoyQuiz = 1,
    Student1Typing = 2,
    FemStd1MathQuiz = 3,
    UniProfProgQuiz = 4
};

struct MinigameTrigger
{
    NPCCharacter* npc;
    MinigameID id;
    ActiveMap mapLoc;
};

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Lost In NUST");

    sf::Texture texture;
    if (!texture.loadFromFile("../../BACKGROUND-IMAGE/NUST Background.png"))
        return -1;

    sf::Sprite sprite;
    sprite.setTexture(texture);
    sf::Vector2u bgSize = texture.getSize();
    sprite.setScale(
        static_cast<float>(window.getSize().x) / bgSize.x,
        static_cast<float>(window.getSize().y) / bgSize.y
    );

    sf::Clock clock;
    sf::Clock globalClock;

    Button startButton, exitButton, settingButton, backButton, fullScreen, gameInstruction, masterVolume;
    startButton.loadFromFile("../../Buttons/Start button.png", sf::Vector2f(960, 400), sf::Vector2f(430, 160));
    exitButton.loadFromFile("../../Buttons/Exit button.png", sf::Vector2f(960, 600), sf::Vector2f(430, 160));
    settingButton.loadFromFile("../../Buttons/Setting Button.png", sf::Vector2f(960, 800), sf::Vector2f(160, 160));
    backButton.loadFromFile("../../Buttons/Back Button.png", sf::Vector2f(120, 50), sf::Vector2f(200, 80));
    fullScreen.loadFromFile("../../Buttons/FullScreen Button.png", sf::Vector2f(750, 350), sf::Vector2f(290, 130));
    masterVolume.loadFromFile("../../Buttons/Master Volume Button.png", sf::Vector2f(1150, 350), sf::Vector2f(290, 130));
    gameInstruction.loadFromFile("../../Buttons/Game Instructions .png", sf::Vector2f(960, 650), sf::Vector2f(530, 290));

    TileMap hostel_map; hostel_map.load("../../Maps/hostel_map.tmx");
    TileMap C2_map;     C2_map.load("../../Maps/C2.tmx");
    TileMap Seecs_map;  Seecs_map.load("../../Maps/Seecs.tmx");

    std::vector<sf::FloatRect> Seecs_furnitureCollision=Seecs_map.getCollisionRects("class room thing");
    std::vector<sf::FloatRect> Seecs_boundaries=Seecs_map.getCollisionRects("boundries");
    Seecs_furnitureCollision.insert(Seecs_furnitureCollision.end(),Seecs_boundaries.begin(),Seecs_boundaries.end());

    std::vector<sf::FloatRect> C2_furniture_collision = C2_map.getCollisionRects("things above c2");
    std::vector<sf::FloatRect> C2_tree_collision = C2_map.getCollisionRects("trees");
    C2_furniture_collision.insert(C2_furniture_collision.end(),C2_tree_collision.begin(),C2_tree_collision.end());

    std::vector<sf::FloatRect> collisionRects = hostel_map.getCollisionRects("Boundaries");
    std::vector<sf::FloatRect> furnitureRects = hostel_map.getCollisionRects("Beds and stuff");
    collisionRects.insert(collisionRects.end(), furnitureRects.begin(), furnitureRects.end());

    AnimatedPlayer player;
    player.loadAllAnimations("../../Main-Char", sf::Vector2f(350, 350), sf::Vector2f(50, 50));
    
    NPCCharacter senior;
    senior.loadAllAnimations("../../NPC/senior_uni", sf::Vector2f(350, 600), sf::Vector2f(50, 50));
    
    // --- C2 Map NPCs ---
    NPCCharacter student2;
    student2.loadAllAnimations("../../NPC/student2", sf::Vector2f(390,600), sf::Vector2f(50,50));
    student2.loadIdleFromRotation("../../NPC/student2/rotations");

    NPCCharacter student1;
    student1.loadAllAnimations("../../NPC/student1", sf::Vector2f(570,600), sf::Vector2f(50,50));
    student1.loadIdleFromRotation("../../NPC/student1/rotations");

    NPCCharacter fem_std1;
    fem_std1.loadAllAnimations("../../NPC/fem_std1", sf::Vector2f(540,600), sf::Vector2f(50,50));
    fem_std1.loadIdleFromRotation("../../NPC/fem_std1/rotations");

    NPCCharacter fem_student;
    fem_student.loadAllAnimations("../../NPC/fem_student", sf::Vector2f(600,800), sf::Vector2f(50,50));
    fem_student.loadIdleFromRotation("../../NPC/fem_student/rotations");

    NPCCharacter student3;
    student3.loadAllAnimations("../../NPC/student3", sf::Vector2f(300,600), sf::Vector2f(50,50));
    student3.loadIdleFromRotation("../../NPC/student3/rotations");

    NPCCharacter uni_boy;
    uni_boy.loadAllAnimations("../../NPC/uni_boy", sf::Vector2f(450,600), sf::Vector2f(50,50));
    uni_boy.loadIdleFromRotation("../../NPC/uni_boy/rotations");

    NPCCharacter uni_student;
    uni_student.loadAllAnimations("../../NPC/uni_student", sf::Vector2f(800,600), sf::Vector2f(50,50));

    // --- SEECS Map NPCs ---
    NPCCharacter uni_prof;
    uni_prof.loadStationaryIdle("../../NPC/uni-professor/", sf::Vector2f(610,200), sf::Vector2f(50,50));

    NPCCharacter seecs_student1;
    seecs_student1.loadAllAnimations("../../NPC/student2", sf::Vector2f(250, 250), sf::Vector2f(50, 50));
    seecs_student1.loadIdleFromRotation("../../NPC/student2/rotations");

    NPCCharacter seecs_student2;
    seecs_student2.loadAllAnimations("../../NPC/student3", sf::Vector2f(750, 350), sf::Vector2f(50, 50));
    seecs_student2.loadIdleFromRotation("../../NPC/student3/rotations");

    NPCCharacter seecs_fem1;
    seecs_fem1.loadAllAnimations("../../NPC/fem_std1", sf::Vector2f(420, 480), sf::Vector2f(50, 50));
    seecs_fem1.loadIdleFromRotation("../../NPC/fem_std1/rotations");

    NPCCharacter seecs_boy1;
    seecs_boy1.loadAllAnimations("../../NPC/uni_boy", sf::Vector2f(820, 220), sf::Vector2f(50, 50));
    seecs_boy1.loadIdleFromRotation("../../NPC/uni_boy/rotations");

    ActiveMap currentMap = ActiveMap::Hostel;

    // --- Cutscene Data ---
    ActiveMap targetMapAfterCutscene = ActiveMap::Hostel;
    sf::Vector2f targetSpawnPosAfterCutscene(0.f, 0.f);
    float cutsceneTimer = 0.f;
    const float cutsceneDuration = 2.0f;
    std::string cutsceneText = "";

    // --- Game Over Data ---
    bool isVictory = false;
    std::string gameOverReason = "You failed to survive NUST!";

    int activeMinigameId = MinigameID::None;
    sf::Vector2f minigameReturnPos;
    ActiveMap minigameReturnMap = ActiveMap::Hostel;

    std::vector<MinigameTrigger> minigameTriggers = {
        { &uni_boy,   MinigameID::UniBoyQuiz,      ActiveMap::C2 },
        { &student1,  MinigameID::Student1Typing,  ActiveMap::C2 },
        { &fem_std1,  MinigameID::FemStd1MathQuiz, ActiveMap::C2 },
        { &uni_prof,  MinigameID::UniProfProgQuiz, ActiveMap::Seecs }
    };

    // --- Typing Minigame Data ---
    std::vector<std::string> typingPrompts = {
        "sudo apt-get install nust-survival-kit",
        "Code fast, but debug slowly and carefully.",
        "System.out.println(\"Hello SEECS!\");"
    };
    std::string typingTarget = "";
    std::string typingInput = "";
    bool typingComplete = false;
    sf::Clock typingClock;
    float typingCompletionTime = 0.f;

    // --- Quiz Minigame Data ---
    struct QuizQuestion {
        std::string question;
        std::string options[3];
        int correctIndex;
    };
    std::vector<QuizQuestion> quizQuestions = {
        {"What does NUST stand for?", {"National Univ of Sciences & Tech", "New Urban Science & Tech", "National United Student Team"}, 0},
        {"Which feature handles scope safely in modern C++?", {"enum class", "magic numbers", "goto statement"}, 0},
        {"Where is the main campus located?", {"H-12 Islamabad", "Rawalpindi Saddar", "Lahore Cantt"}, 0}
    };
    int currentQuizIndex = 0;
    bool quizFinished = false;
    bool quizWon = false;

    // --- Programming Quiz Minigame Data ---
    std::vector<QuizQuestion> progQuizQuestions = {
        {"Which C++ operator performs a bitwise XOR?", {"& (AND)", "^ (XOR)", "| (OR)"}, 1},
        {"What principle does a Stack data structure follow?", {"LIFO (Last-In First-Out)", "FIFO (First-In First-Out)", "FILO (First-In Last-Out)"}, 0},
        {"Which Java framework is typically used for Desktop GUIs?", {"Spring Boot", "Hibernate", "JavaFX"}, 2}
    };
    int currentProgQuizIndex = 0;
    bool progQuizFinished = false;
    bool progQuizWon = false;

    // --- Math Quiz Minigame Data ---
    int mathOp1 = 0;
    int mathOp2 = 0;
    char mathOperator = '+';
    int mathTargetAnswer = 0;
    std::string mathInput = "";
    bool mathComplete = false;
    bool mathFailed = false;
    bool mathAnswered = false;

    float introElapsed = 0.f;
    std::vector<std::string> introLines = {
        "Yenna: Hello guys, my name is Yenna.",
        "Yenna: Please help me survive a day in NUST!"
    };
    int introLineIndex = 0;
    float charsPerSecond = 15.f;

    sf::Font introFont;
    introFont.loadFromFile("../../Fonts/Inter/Your Font.ttf");
    introFont.setSmooth(false); 

    sf::Text bubbleText;
    bubbleText.setFont(introFont);
    bubbleText.setCharacterSize(10);
    bubbleText.setFillColor(sf::Color::Black);

    RoundedRectangleShape speechBubble(sf::Vector2f(100.f, 40.f), 8.f);
    speechBubble.setFillColor(sf::Color(255, 255, 255));
    speechBubble.setOutlineThickness(2.f);
    speechBubble.setOutlineColor(sf::Color(60, 40, 20));

    std::vector<std::string> seniorDialogue1 = {"Senior: Oh hey, you're new here?", "Senior: Can you help me find my notes ??"};
    std::vector<std::string> seniorDialogue2 = {"Senior: Thanks alot !", "Senior: I have to go now see u later ;)"};
    
    int dialogueIndex = 0;
    float dialogueElapsed = 0.f;
    bool showingDialogue = false;
    float interactionRange = 60.f;

    const std::string bookLayerName = "Beds and stuff";
    const int bookTileX = 38;
    const int bookTileY = 35;
    const int bookWidth = 2;
    const int bookHeight = 2;
    bool bookPickedUp = false;

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

    // Shared Warm UI box for Minigames
    RoundedRectangleShape minigameBox(sf::Vector2f(860.f, 560.f), 24.f);
    minigameBox.setFillColor(sf::Color(101, 67, 33));
    minigameBox.setOrigin(430.f, 280.f);
    minigameBox.setPosition(960.f, 540.f);
    minigameBox.setOutlineThickness(4);
    minigameBox.setOutlineColor(sf::Color(60, 40, 20));

    bool wasChecked = fullscreenCheckbox.checked;

    sf::View uiView(sf::FloatRect(0, 0, 1920, 1080));
    sf::View gameView(sf::FloatRect(0, 0, 512, 288));
    window.setView(uiView);

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        float totalTime = globalClock.getElapsedTime().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) window.close();
            
            if (currentState == GameState::Menu)
            {
                if (startButton.isClicked(window, event))
                {
                    currentState = GameState::CharacterIntro;
                    introElapsed = 0.f;
                }
                if (settingButton.isClicked(window, event)) currentState = GameState::Setting;
                if (exitButton.isClicked(window, event)) window.close();
            }
            else if (currentState == GameState::Setting)
            {
                if (backButton.isClicked(window, event)) currentState = GameState::Menu;

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

                    if (!doneTyping) {
                        introElapsed = introLines[introLineIndex].size() / charsPerSecond + 0.01f;
                    } else {
                        introLineIndex++;
                        introElapsed = 0.f;
                        if (introLineIndex >= (int)introLines.size())
                            currentState = GameState::playing;
                    }
                }
            }
            else if (currentState == GameState::GameOver)
            {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                {
                    currentState = GameState::Menu;
                    currentMap = ActiveMap::Hostel;
                    player.setPosition(sf::Vector2f(350, 350));
                    bookPickedUp = false;
                }
            }
            else if (currentState == GameState::Minigame)
            {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
                {
                    currentState = GameState::playing;
                    player.setPosition(minigameReturnPos);
                    currentMap = minigameReturnMap;
                    activeMinigameId = MinigameID::None;
                }

                if (activeMinigameId == MinigameID::Student1Typing)
                {
                    if (typingComplete && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                    {
                        currentState = GameState::playing;
                        player.setPosition(minigameReturnPos);
                        currentMap = minigameReturnMap;
                        activeMinigameId = MinigameID::None;
                    }
                    else if (!typingComplete && event.type == sf::Event::TextEntered)
                    {
                        if (event.text.unicode == 8) {
                            if (!typingInput.empty()) typingInput.pop_back();
                        } else if (event.text.unicode >= 32 && event.text.unicode < 127) {
                            if (typingInput.size() < typingTarget.size())
                                typingInput += static_cast<char>(event.text.unicode);
                        }
                        if (typingInput == typingTarget) {
                            typingComplete = true;
                            typingCompletionTime = typingClock.getElapsedTime().asSeconds();
                        }
                    }
                }
                else if (activeMinigameId == MinigameID::UniBoyQuiz)
                {
                    if (quizFinished && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                    {
                        currentState = GameState::playing;
                        player.setPosition(minigameReturnPos);
                        currentMap = minigameReturnMap;
                        activeMinigameId = MinigameID::None;
                    }
                    else if (!quizFinished && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                    {
                        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                        for (int i = 0; i < 3; ++i) {
                            sf::FloatRect optBounds(600.f, 480.f + (i * 75.f), 720.f, 55.f);
                            if (optBounds.contains(mousePos)) {
                                if (i == quizQuestions[currentQuizIndex].correctIndex) {
                                    currentQuizIndex++;
                                    if (currentQuizIndex >= (int)quizQuestions.size()) {
                                        quizFinished = true;
                                        quizWon = true;
                                    }
                                } else {
                                    quizFinished = true;
                                    quizWon = false;
                                }
                            }
                        }
                    }
                }
                else if (activeMinigameId == MinigameID::UniProfProgQuiz)
                {
                    if (progQuizFinished && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                    {
                        if (progQuizWon) {
                            isVictory = true;
                            gameOverReason = "A+ SECURED! You survived your OOP project and conquered NUST!";
                            currentState = GameState::GameOver;
                        } else {
                            currentState = GameState::playing;
                            player.setPosition(minigameReturnPos);
                            currentMap = minigameReturnMap;
                            activeMinigameId = MinigameID::None;
                        }
                    }
                    else if (!progQuizFinished && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                    {
                        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                        for (int i = 0; i < 3; ++i) {
                            sf::FloatRect optBounds(600.f, 480.f + (i * 75.f), 720.f, 55.f);
                            if (optBounds.contains(mousePos)) {
                                if (i == progQuizQuestions[currentProgQuizIndex].correctIndex) {
                                    currentProgQuizIndex++;
                                    if (currentProgQuizIndex >= (int)progQuizQuestions.size()) {
                                        progQuizFinished = true;
                                        progQuizWon = true;
                                    }
                                } else {
                                    progQuizFinished = true;
                                    progQuizWon = false;
                                }
                            }
                        }
                    }
                }
                else if (activeMinigameId == MinigameID::FemStd1MathQuiz)
                {
                    if (mathAnswered && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter)
                    {
                        currentState = GameState::playing;
                        player.setPosition(minigameReturnPos);
                        currentMap = minigameReturnMap;
                        activeMinigameId = MinigameID::None;
                    }
                    else if (!mathAnswered && event.type == sf::Event::TextEntered)
                    {
                        if (event.text.unicode == 8) {
                            if (!mathInput.empty()) mathInput.pop_back();
                        } else if ((event.text.unicode >= '0' && event.text.unicode <= '9') || event.text.unicode == '-') {
                            if (mathInput.size() < 10) mathInput += static_cast<char>(event.text.unicode);
                        } else if (event.text.unicode == 13) {
                            if (!mathInput.empty()) {
                                mathAnswered = true;
                                try {
                                    if (std::stoi(mathInput) == mathTargetAnswer) mathComplete = true;
                                    else mathFailed = true;
                                } catch (...) { mathFailed = true; }
                            }
                        }
                    }
                }
            }
            else if (currentState == GameState::playing)
            {
                if (currentMap == ActiveMap::Hostel)
                {
                    sf::Vector2f playerPos = player.getPosition();
                    if (playerPos.y >= 920.f)
                    {
                        targetMapAfterCutscene = ActiveMap::C2;
                        targetSpawnPosAfterCutscene = sf::Vector2f(25.f, 950.f);
                        cutsceneText = "Entering C2 Building...";
                        cutsceneTimer = 0.f;
                        currentState = GameState::Cutscene;
                    }
                }
                else if (currentMap == ActiveMap::C2)
                {
                    sf::Vector2f playerPos = player.getPosition();
                    if (playerPos.x >= 1000.f)
                    {
                        targetMapAfterCutscene = ActiveMap::Seecs;
                        targetSpawnPosAfterCutscene = sf::Vector2f(25.f, 500.f);
                        cutsceneText = "Entering SEECS Building...";
                        cutsceneTimer = 0.f;
                        currentState = GameState::Cutscene;
                    }
                }

                if (event.type == sf::Event::KeyReleased&& event.key.code == sf::Keyboard::E)
                {
                    if (currentMap == ActiveMap::Hostel)
                    {
                        sf::Vector2i playerTile = hostel_map.worldToTile(player.getPosition());
                        int bookDx = std::abs(playerTile.x - bookTileX);
                        int bookDy = std::abs(playerTile.y - bookTileY);
                        bool nearBook = (bookDx <= 1 && bookDy <= 1) && !bookPickedUp;
                        if (!bookPickedUp)
                        {
                            for (int x = bookTileX; x < bookTileX + bookWidth; x++) {
                                for (int y = bookTileY; y < bookTileY + bookHeight; y++) {
                                    int dx = std::abs(playerTile.x - x);
                                    int dy = std::abs(playerTile.y - y);
                                    if (dx <= 1 && dy <= 1) { nearBook = true; break; }
                                }
                                if (nearBook) break;
                            }
                        }

                        if (nearBook)
                        {
                            for (int x = bookTileX; x < bookTileX + bookWidth; x++) {
                                for (int y = bookTileY; y < bookTileY + bookHeight; y++) {
                                    hostel_map.hideTile("Beds and stuff", x, y);
                                }
                            }
                            bookPickedUp = true;
                        }
                        else
                        {
                            float dist = std::hypot(player.getPosition().x - senior.getPosition().x, player.getPosition().y - senior.getPosition().y);
                            if (!showingDialogue && dist < interactionRange) {
                                showingDialogue = true; dialogueIndex = 0; dialogueElapsed = 0.f;
                            }
                            else if (showingDialogue) {
                                const std::vector<std::string>& activeDialogue = bookPickedUp ? seniorDialogue2 : seniorDialogue1;
                                int visibleChars = static_cast<int>(dialogueElapsed * charsPerSecond);
                                bool doneTyping = visibleChars >= (int)activeDialogue[dialogueIndex].size();

                                if (!doneTyping) dialogueElapsed = activeDialogue[dialogueIndex].size() / charsPerSecond + 0.01f;
                                else {
                                    dialogueIndex++; dialogueElapsed = 0.f;
                                    if (dialogueIndex >= (int)activeDialogue.size()) showingDialogue = false;
                                }
                            }
                        }
                    }
                    else
                    {
                        for (auto& trig : minigameTriggers)
                        {
                            if (trig.mapLoc != currentMap) continue;

                            float dist = std::hypot(
                                player.getPosition().x - trig.npc->getPosition().x,
                                player.getPosition().y - trig.npc->getPosition().y
                            );

                            if (dist < interactionRange)
                            {
                                activeMinigameId = trig.id;
                                minigameReturnPos = player.getPosition();
                                minigameReturnMap = currentMap;
                                currentState = GameState::Minigame;

                                if (trig.id == MinigameID::Student1Typing) {
                                    typingTarget = typingPrompts[std::rand() % typingPrompts.size()];
                                    typingInput = ""; typingComplete = false; typingClock.restart();
                                }
                                else if (trig.id == MinigameID::UniBoyQuiz) {
                                    currentQuizIndex = 0; quizFinished = false; quizWon = false;
                                }
                                else if (trig.id == MinigameID::UniProfProgQuiz) {
                                    currentProgQuizIndex = 0; progQuizFinished = false; progQuizWon = false;
                                }
                                else if (trig.id == MinigameID::FemStd1MathQuiz) {
                                    mathOp1 = std::rand() % 50 + 10; mathOp2 = std::rand() % 20 + 1;
                                    int opType = std::rand() % 3;
                                    if (opType == 0) { mathOperator = '+'; mathTargetAnswer = mathOp1 + mathOp2; } 
                                    else if (opType == 1) { mathOperator = '-'; mathTargetAnswer = mathOp1 - mathOp2; } 
                                    else { mathOperator = '*'; mathTargetAnswer = mathOp1 * mathOp2; }
                                    mathInput = ""; mathAnswered = false; mathComplete = false; mathFailed = false;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        window.clear(sf::Color::Black);

        if (currentState == GameState::Menu || currentState == GameState::Setting || currentState == GameState::CharacterIntro || currentState == GameState::GameOver)
            window.setView(uiView);

        if (currentState == GameState::Menu)
        {
            window.draw(sprite);
            startButton.updateHover(window, deltaTime); exitButton.updateHover(window, deltaTime);
            settingButton.updateHover(window, deltaTime); backButton.updateHover(window, deltaTime);
            startButton.drawSprite(window); exitButton.drawSprite(window); settingButton.drawSprite(window);
        }
        else if (currentState == GameState::Setting)
        {
            window.draw(sprite); window.draw(settingsBox);
            backButton.drawSprite(window); backButton.updateHover(window, deltaTime);
            fullScreen.drawSprite(window); masterVolume.drawSprite(window); gameInstruction.drawSprite(window);
            masterVolumeSlider.draw(window); fullscreenCheckbox.draw(window);
        }
        else if (currentState == GameState::GameOver)
        {
            sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
            overlay.setFillColor(sf::Color(10, 0, 0, 230));
            window.draw(overlay);

            sf::Text goTitle(isVictory ? "VICTORY!" : "GAME OVER", introFont, 64);
            goTitle.setFillColor(isVictory ? sf::Color(210, 180, 140) : sf::Color(255, 100, 100));
            goTitle.setStyle(sf::Text::Bold);
            drawCenteredText(window, goTitle, 960.f, 350.f, true);

            sf::Text goReason(gameOverReason, introFont, 24);
            goReason.setFillColor(sf::Color(220, 220, 220));
            drawCenteredText(window, goReason, 960.f, 470.f, true);

            sf::Text goPrompt(">> PRESS ENTER TO RETURN TO MAIN MENU <<", introFont, 20);
            goPrompt.setFillColor(sf::Color::White);
            drawCenteredText(window, goPrompt, 960.f, 600.f, true);
        }
        else if (currentState == GameState::Cutscene)
        {
            cutsceneTimer += deltaTime;
            window.setView(uiView);

            float progress = cutsceneTimer / cutsceneDuration;
            float alpha = (progress < 0.5f) ? (progress / 0.5f) * 255.f : ((1.0f - progress) / 0.5f) * 255.f;
            alpha = std::max(0.f, std::min(alpha, 255.f));

            if (cutsceneTimer >= cutsceneDuration * 0.5f && currentMap != targetMapAfterCutscene) {
                currentMap = targetMapAfterCutscene;
                player.setPosition(targetSpawnPosAfterCutscene);
            }

            sf::RectangleShape letterboxTop(sf::Vector2f(1920.f, 250.f));
            letterboxTop.setFillColor(sf::Color::Black); window.draw(letterboxTop);
            sf::RectangleShape letterboxBottom(sf::Vector2f(1920.f, 250.f));
            letterboxBottom.setPosition(0.f, 1080.f - 250.f);
            letterboxBottom.setFillColor(sf::Color::Black); window.draw(letterboxBottom);

            sf::Text transitionText(cutsceneText, introFont, 32);
            transitionText.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
            drawCenteredText(window, transitionText, 960.f, 540.f, true);

            if (cutsceneTimer >= cutsceneDuration) currentState = GameState::playing;
        }
        else if (currentState == GameState::CharacterIntro)
        {
            introElapsed += deltaTime;

            sf::Vector2f viewSize = gameView.getSize();
            sf::Vector2f center = player.getPosition();
            center.x = std::max(viewSize.x / 2.f, std::min(center.x, 1024.f - viewSize.x / 2.f));
            center.y = std::max(viewSize.y / 2.f, std::min(center.y, 1024.f - viewSize.y / 2.f));
            gameView.setCenter(center); window.setView(gameView);

            player.updateAnimation(deltaTime);

            window.draw(hostel_map); player.draw(window);

            int visibleChars = std::min(static_cast<int>(introElapsed * charsPerSecond), static_cast<int>(introLines[introLineIndex].size()));
            bubbleText.setString(introLines[introLineIndex].substr(0, visibleChars));
            sf::FloatRect textBounds = bubbleText.getLocalBounds();
            bubbleText.setOrigin(textBounds.width / 2.f, textBounds.height / 2.f);

            float bubbleWidth = std::max(textBounds.width + 20.f, 60.f); float bubbleHeight = textBounds.height + 20.f;
            sf::Vector2f bubbleCenter(player.getPosition().x, player.getPosition().y - 40.f);

            speechBubble.setSize(sf::Vector2f(bubbleWidth, bubbleHeight)); speechBubble.setOrigin(bubbleWidth / 2.f, bubbleHeight / 2.f);
            speechBubble.setPosition(bubbleCenter); bubbleText.setPosition(bubbleCenter.x, bubbleCenter.y - 2.f);

            window.draw(speechBubble); window.draw(bubbleText);
        }
        else if (currentState == GameState::Minigame)
        {
            sf::RectangleShape dimOverlay(sf::Vector2f(1920.f, 1080.f));
            dimOverlay.setFillColor(sf::Color(0, 0, 0, 160));
            window.draw(dimOverlay);

            window.setView(uiView);
            window.draw(minigameBox);

            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            sf::Color tanColor(210, 180, 140);

            // --- 1. Typing Minigame (Student 1 - C2) ---
            if (activeMinigameId == MinigameID::Student1Typing)
            {
                sf::Text titleText("SPEED TYPING TEST", introFont, 36);
                titleText.setFillColor(tanColor);
                titleText.setStyle(sf::Text::Bold);
                drawCenteredText(window, titleText, 960.f, 320.f, true);

                if (!typingComplete) {
                    sf::Text promptLabel("Type the exact text below:", introFont, 20);
                    promptLabel.setFillColor(sf::Color(220, 220, 220));
                    drawCenteredText(window, promptLabel, 960.f, 390.f, true);

                    sf::Text targetText(typingTarget, introFont, 24);
                    targetText.setFillColor(sf::Color::White);
                    drawCenteredText(window, targetText, 960.f, 450.f, true);

                    sf::Text inputText(typingInput + "_", introFont, 24);
                    inputText.setFillColor(tanColor);
                    drawCenteredText(window, inputText, 960.f, 520.f, true);
                } else {
                    sf::Text resText("TEST PASSED!", introFont, 36);
                    resText.setFillColor(tanColor);
                    resText.setStyle(sf::Text::Bold);
                    drawCenteredText(window, resText, 960.f, 440.f, true);

                    sf::Text timeText("Time: " + std::to_string(typingCompletionTime).substr(0, 4) + "s", introFont, 20);
                    timeText.setFillColor(sf::Color::White);
                    drawCenteredText(window, timeText, 960.f, 500.f, true);

                    sf::Text enterTxt(">> PRESS ENTER TO CONTINUE <<", introFont, 20);
                    enterTxt.setFillColor(sf::Color::White);
                    drawCenteredText(window, enterTxt, 960.f, 580.f, true);
                }
            }
            // --- 2. Campus Trivia Quiz (Uni Boy - C2) ---
            else if (activeMinigameId == MinigameID::UniBoyQuiz)
            {
                sf::Text titleText("CAMPUS TRIVIA QUIZ", introFont, 36);
                titleText.setFillColor(tanColor);
                titleText.setStyle(sf::Text::Bold);
                drawCenteredText(window, titleText, 960.f, 320.f, true);

                sf::Text progressText("Question " + std::to_string(currentQuizIndex + 1) + " of " + std::to_string((int)quizQuestions.size()), introFont, 18);
                progressText.setFillColor(sf::Color(200, 200, 200));
                drawCenteredText(window, progressText, 960.f, 365.f, true);

                if (!quizFinished) {
                    sf::Text qText(quizQuestions[currentQuizIndex].question, introFont, 22);
                    qText.setFillColor(sf::Color::White);
                    drawCenteredText(window, qText, 960.f, 420.f, true);

                    for (int i = 0; i < 3; ++i) {
                        sf::FloatRect optBounds(600.f, 480.f + (i * 75.f), 720.f, 55.f);
                        bool isHovered = optBounds.contains(mousePos);

                        RoundedRectangleShape optBox(sf::Vector2f(720.f, 55.f), 12.f);
                        optBox.setFillColor(isHovered ? tanColor : sf::Color(60, 40, 20));
                        optBox.setPosition(600.f, 480.f + (i * 75.f));
                        window.draw(optBox);

                        sf::Text optText(quizQuestions[currentQuizIndex].options[i], introFont, 20);
                        optText.setFillColor(isHovered ? sf::Color(60, 40, 20) : sf::Color::White);
                        drawCenteredText(window, optText, 960.f, 507.f + (i * 75.f));
                    }
                }
                else {
                    sf::Text resText(quizWon ? "TRIVIA PASSED!" : "TRIVIA FAILED!", introFont, 36);
                    resText.setFillColor(quizWon ? tanColor : sf::Color(255, 100, 100));
                    resText.setStyle(sf::Text::Bold);
                    drawCenteredText(window, resText, 960.f, 480.f, true);

                    sf::Text enterTxt(">> PRESS ENTER TO RETURN <<", introFont, 20);
                    enterTxt.setFillColor(sf::Color::White);
                    drawCenteredText(window, enterTxt, 960.f, 560.f, true);
                }
            }
            // --- 3. Quick Math Challenge (Fem Std 1 - C2) ---
            else if (activeMinigameId == MinigameID::FemStd1MathQuiz)
            {
                sf::Text titleText("QUICK MATH QUIZ", introFont, 36);
                titleText.setFillColor(tanColor);
                titleText.setStyle(sf::Text::Bold);
                drawCenteredText(window, titleText, 960.f, 320.f, true);

                if (!mathAnswered) {
                    sf::Text promptLabel("Solve the equation:", introFont, 20);
                    promptLabel.setFillColor(sf::Color(220, 220, 220));
                    drawCenteredText(window, promptLabel, 960.f, 390.f, true);

                    std::string eqStr = std::to_string(mathOp1) + " " + mathOperator + " " + std::to_string(mathOp2) + " = ?";
                    sf::Text eqText(eqStr, introFont, 32);
                    eqText.setFillColor(sf::Color::White);
                    drawCenteredText(window, eqText, 960.f, 450.f, true);

                    sf::Text inputText(mathInput + "_", introFont, 28);
                    inputText.setFillColor(tanColor);
                    drawCenteredText(window, inputText, 960.f, 520.f, true);
                } else {
                    sf::Text resText(mathComplete ? "CORRECT ANSWER!" : "INCORRECT ANSWER!", introFont, 36);
                    resText.setFillColor(mathComplete ? tanColor : sf::Color(255, 100, 100));
                    resText.setStyle(sf::Text::Bold);
                    drawCenteredText(window, resText, 960.f, 480.f, true);

                    sf::Text enterTxt(">> PRESS ENTER TO CONTINUE <<", introFont, 20);
                    enterTxt.setFillColor(sf::Color::White);
                    drawCenteredText(window, enterTxt, 960.f, 560.f, true);
                }
            }
            // --- 4. OOP Exam (Uni Prof - SEECS) ---
            else if (activeMinigameId == MinigameID::UniProfProgQuiz)
            {
                sf::Text titleText("CS PROFESSOR'S OOP QUIZ", introFont, 36);
                titleText.setFillColor(tanColor);
                titleText.setStyle(sf::Text::Bold);
                drawCenteredText(window, titleText, 960.f, 320.f, true);

                sf::Text progressText("Question " + std::to_string(currentProgQuizIndex + 1) + " of " + std::to_string((int)progQuizQuestions.size()), introFont, 18);
                progressText.setFillColor(sf::Color(200, 200, 200));
                drawCenteredText(window, progressText, 960.f, 365.f, true);

                if (!progQuizFinished) {
                    sf::Text qText(progQuizQuestions[currentProgQuizIndex].question, introFont, 22);
                    qText.setFillColor(sf::Color::White);
                    drawCenteredText(window, qText, 960.f, 420.f, true);

                    for (int i = 0; i < 3; ++i) {
                        sf::FloatRect optBounds(600.f, 480.f + (i * 75.f), 720.f, 55.f);
                        bool isHovered = optBounds.contains(mousePos);

                        RoundedRectangleShape optBox(sf::Vector2f(720.f, 55.f), 12.f);
                        optBox.setFillColor(isHovered ? tanColor : sf::Color(60, 40, 20));
                        optBox.setPosition(600.f, 480.f + (i * 75.f));
                        window.draw(optBox);

                        sf::Text optText(progQuizQuestions[currentProgQuizIndex].options[i], introFont, 20);
                        optText.setFillColor(isHovered ? sf::Color(60, 40, 20) : sf::Color::White);
                        drawCenteredText(window, optText, 960.f, 507.f + (i * 75.f));
                    }
                }
                else {
                    sf::Text resText(progQuizWon ? "EXAM PASSED!" : "EXAM FAILED!", introFont, 36);
                    resText.setFillColor(progQuizWon ? tanColor : sf::Color(255, 100, 100));
                    resText.setStyle(sf::Text::Bold);
                    drawCenteredText(window, resText, 960.f, 480.f, true);

                    sf::Text enterTxt(progQuizWon ? ">> PRESS ENTER TO CLAIM VICTORY <<" : ">> PRESS ENTER TO RETRY <<", introFont, 20);
                    enterTxt.setFillColor(sf::Color::White);
                    drawCenteredText(window, enterTxt, 960.f, 560.f, true);
                }
            }
        }
        else
        {
            std::vector<sf::FloatRect> active_collision =
                (currentMap == ActiveMap::Hostel) ? collisionRects :
                (currentMap == ActiveMap::C2)     ? C2_furniture_collision :
                                                      Seecs_furnitureCollision;
            
            if (!showingDialogue)
            {
                if (currentMap == ActiveMap::Hostel) {
                    senior.update(deltaTime, active_collision);
                } else if (currentMap == ActiveMap::C2) {
                    student2.update(deltaTime, active_collision); student1.update(deltaTime, active_collision);
                    student3.update(deltaTime, active_collision); fem_std1.update(deltaTime, active_collision);
                    fem_student.update(deltaTime, active_collision); uni_boy.update(deltaTime, active_collision);
                    uni_student.update(deltaTime, active_collision);
                } else if (currentMap == ActiveMap::Seecs) {
                    uni_prof.update(deltaTime, active_collision);
                    seecs_student1.update(deltaTime, active_collision);
                    seecs_student2.update(deltaTime, active_collision);
                    seecs_fem1.update(deltaTime, active_collision);
                    seecs_boy1.update(deltaTime, active_collision);
                }
            }

            std::vector<sf::FloatRect> playerCollisionRects = active_collision;
            if (currentMap == ActiveMap::Hostel) {
                playerCollisionRects.push_back(senior.getCollisionBox());
            } else if (currentMap == ActiveMap::C2) {
                playerCollisionRects.push_back(student2.getCollisionBox()); playerCollisionRects.push_back(student1.getCollisionBox());
                playerCollisionRects.push_back(student3.getCollisionBox()); playerCollisionRects.push_back(fem_std1.getCollisionBox());
                playerCollisionRects.push_back(fem_student.getCollisionBox()); playerCollisionRects.push_back(uni_boy.getCollisionBox());
                playerCollisionRects.push_back(uni_student.getCollisionBox());
            } else if (currentMap == ActiveMap::Seecs) {
                playerCollisionRects.push_back(uni_prof.getCollisionBox());
                playerCollisionRects.push_back(seecs_student1.getCollisionBox());
                playerCollisionRects.push_back(seecs_student2.getCollisionBox());
                playerCollisionRects.push_back(seecs_fem1.getCollisionBox());
                playerCollisionRects.push_back(seecs_boy1.getCollisionBox());
            }

            if (!showingDialogue) player.handleMovement(deltaTime, playerCollisionRects);
            player.updateAnimation(deltaTime);

            sf::Vector2f viewSize = gameView.getSize(); sf::Vector2f center = player.getPosition();
            center.x = std::max(viewSize.x / 2.f, std::min(center.x, 1024.f - viewSize.x / 2.f));
            center.y = std::max(viewSize.y / 2.f, std::min(center.y, 1024.f - viewSize.y / 2.f));
            gameView.setCenter(center); window.setView(gameView);

            if (currentMap == ActiveMap::Hostel)
            {
                window.draw(hostel_map); player.draw(window); senior.draw(window);
            }
            else if (currentMap == ActiveMap::C2)
            {
                window.draw(C2_map); player.draw(window);
                student2.draw(window); student1.draw(window); student3.draw(window);
                fem_std1.draw(window); fem_student.draw(window); uni_boy.draw(window); uni_student.draw(window);

                for (auto& trig : minigameTriggers) {
                    if (trig.mapLoc == currentMap) drawInteractionBubble(window, trig.npc->getPosition(), introFont, totalTime);
                }
            }
            else // Seecs Map
            {
                window.draw(Seecs_map); player.draw(window); 
                uni_prof.draw(window);
                seecs_student1.draw(window);
                seecs_student2.draw(window);
                seecs_fem1.draw(window);
                seecs_boy1.draw(window);

                for (auto& trig : minigameTriggers) {
                    if (trig.mapLoc == currentMap) drawInteractionBubble(window, trig.npc->getPosition(), introFont, totalTime);
                }
            }

            if (showingDialogue)
            {
                dialogueElapsed += deltaTime;
                const std::vector<std::string>& activeDialogue = bookPickedUp ? seniorDialogue2 : seniorDialogue1;
                int visibleChars = std::min(static_cast<int>(dialogueElapsed * charsPerSecond), (int)activeDialogue[dialogueIndex].size());
                bubbleText.setString(activeDialogue[dialogueIndex].substr(0, visibleChars));
                sf::FloatRect textBounds = bubbleText.getLocalBounds(); bubbleText.setOrigin(textBounds.width / 2.f, textBounds.height / 2.f);

                float bubbleWidth = std::max(textBounds.width + 20.f, 60.f); float bubbleHeight = textBounds.height + 20.f;
                sf::Vector2f bubbleCenter(senior.getPosition().x, senior.getPosition().y - 40.f);
                speechBubble.setSize(sf::Vector2f(bubbleWidth, bubbleHeight)); speechBubble.setOrigin(bubbleWidth / 2.f, bubbleHeight / 2.f);
                speechBubble.setPosition(bubbleCenter); bubbleText.setPosition(bubbleCenter.x, bubbleCenter.y - 2.f);

                window.draw(speechBubble); window.draw(bubbleText);
            }
        }
        window.display();
    }
    return 0;
}