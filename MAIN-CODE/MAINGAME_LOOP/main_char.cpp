#include <iostream>
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>
#include <cmath>

class AnimatedPlayer
{
public:
    float speed = 120.f;             // pixels per second — character movement speed
    float walkFrameDuration = 0.35f; // seconds per frame while walking — higher = slower cycle
    float idleFrameDuration = 0.5f;  // seconds per frame while idle — usually slower than walking

    // === ADJUST THESE if your folder names differ ===
    std::string walkAnimationName = "Walking";
    std::string idleAnimationName = "breathing-idle";
   
    bool loadAllAnimations(const std::string& basePath, sf::Vector2f startPos, sf::Vector2f displaySize)
    {
        // animation folders you have — add more names here if you add more later
        std::vector<std::string> animationNames = { "breathing-idle", "picking-up", "Walking" };

        // must match your actual folder names exactly
        std::vector<std::string> directionNames = {
            "north", "north-east", "east", "south-east",
            "south", "south-west", "west", "north-west"
        };

        for (const auto& anim : animationNames)
        {
            for (const auto& dir : directionNames)
            {
                std::vector<sf::Texture> frames;
                bool loadedAny = false;

                for (int i = 0; i < 4; ++i)
                {
                    char frameFile[16];
                    snprintf(frameFile, sizeof(frameFile), "frame_%03d.png", i);

                    std::string fullPath = basePath + "/" + anim + "/" + dir + "/" + frameFile;

                    sf::Texture tex;
                    if (tex.loadFromFile(fullPath))
                    {
                        frames.push_back(tex);
                        loadedAny = true;
                    }
                    else
                    {
                        std::cerr << "Warning: could not load " << fullPath << std::endl;
                    }
                }

                if (loadedAny)
                    m_animations[anim][dir] = frames;
            }
        }

        if (m_animations.empty())
        {
            std::cerr << "Error: no player animation frames loaded at all. Check basePath." << std::endl;
            return false;
        }

        m_sprite.setPosition(startPos);
        m_displaySize = displaySize;
        updateFrameTexture();

        return true;
    }

    // Size of the player's collision box (not the whole sprite — just the feet area).
    // Tune these two values until walking near walls/furniture feels right.
    sf::Vector2f collisionBoxSize = sf::Vector2f(20.f, 12.f);

    void handleMovement(float deltaTime, const std::vector<sf::FloatRect>& collisionRects)
    {
        sf::Vector2f movement(0.f, 0.f);

        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right);

        if (up) movement.y -= 1.f;
        if (down) movement.y += 1.f;
        if (left) movement.x -= 1.f;
        if (right) movement.x += 1.f;

        m_isMoving = (movement.x != 0.f || movement.y != 0.f);

        // pick direction folder name — diagonals take priority when two keys held
        if (up && left) m_currentDirection = "north-west";
        else if (up && right) m_currentDirection = "north-east";
        else if (down && left) m_currentDirection = "south-west";
        else if (down && right) m_currentDirection = "south-east";
        else if (up) m_currentDirection = "north";
        else if (down) m_currentDirection = "south";
        else if (left) m_currentDirection = "west";
        else if (right) m_currentDirection = "east";

        float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
        if (length != 0.f)
        {
            movement /= length;
            sf::Vector2f delta = movement * speed * deltaTime;

            // move on X, revert only the X move if it caused a collision (lets the
            // player keep sliding along a wall on the Y axis, and vice versa)
            m_sprite.move(delta.x, 0.f);
            if (isColliding(collisionRects))
                m_sprite.move(-delta.x, 0.f);

            m_sprite.move(0.f, delta.y);
            if (isColliding(collisionRects))
                m_sprite.move(0.f, -delta.y);
        }
    }

    void updateAnimation(float deltaTime)
    {
        std::string targetAnim = m_isMoving ? walkAnimationName : idleAnimationName;

        if (targetAnim != m_currentAnimation)
        {
            m_currentAnimation = targetAnim;
            m_currentFrame = 0;
            m_frameTimer = 0.f;
        }

        float currentFrameDuration = m_isMoving ? walkFrameDuration : idleFrameDuration;

        // cycle frames for BOTH walking and idle
        m_frameTimer += deltaTime;
        if (m_frameTimer >= currentFrameDuration)
        {
            m_frameTimer = 0.f;
            int frameCount = getFrameCount();
            if (frameCount > 0)
                m_currentFrame = (m_currentFrame + 1) % frameCount;
        }

        updateFrameTexture();
    }

    void draw(sf::RenderWindow& window)
    {
        window.draw(m_sprite);
    }

    sf::Vector2f getPosition() const
    {
        return m_sprite.getPosition();
    }
    void setPosition(sf::Vector2f pos)
    {
        m_sprite.setPosition(pos);
    }

    const sf::Texture& getPortraitTexture() const
    {
        return m_animations.at(idleAnimationName).at("south")[0];
    }

    sf::FloatRect getCollisionBox() const
    {
        sf::Vector2f pos = m_sprite.getPosition();
        return sf::FloatRect(
            pos.x - collisionBoxSize.x / 2.f,
            pos.y + m_displaySize.y / 2.f - collisionBoxSize.y, // sits at the sprite's feet
            collisionBoxSize.x,
            collisionBoxSize.y
        );
    }

private:
    bool isColliding(const std::vector<sf::FloatRect>& collisionRects) const
    {
        sf::FloatRect box = getCollisionBox();
        for (const auto& rect : collisionRects)
        {
            if (box.intersects(rect))
                return true;
        }
        return false;
    }

    std::map<std::string, std::map<std::string, std::vector<sf::Texture>>> m_animations;
    sf::Sprite m_sprite;
    sf::Vector2f m_displaySize;

    std::string m_currentAnimation = "breathing-idle";
    std::string m_currentDirection = "south";
    int m_currentFrame = 0;
    float m_frameTimer = 0.f;
    bool m_isMoving = false;

    int getFrameCount() const
    {
        auto animIt = m_animations.find(m_currentAnimation);
        if (animIt == m_animations.end()) return 0;

        auto dirIt = animIt->second.find(m_currentDirection);
        if (dirIt == animIt->second.end()) return 0;

        return static_cast<int>(dirIt->second.size());
    }

    void updateFrameTexture()
    {
        auto animIt = m_animations.find(m_currentAnimation);
        if (animIt == m_animations.end()) return;

        auto dirIt = animIt->second.find(m_currentDirection);
        if (dirIt == animIt->second.end()) return;

        const auto& frames = dirIt->second;
        if (frames.empty()) return;

        int frameIndex = m_currentFrame % static_cast<int>(frames.size());
        const sf::Texture& tex = frames[frameIndex];

        m_sprite.setTexture(tex, true);

        sf::Vector2u texSize = tex.getSize();
        m_sprite.setOrigin(texSize.x / 2.f, texSize.y / 2.f);

        sf::Vector2f scale(m_displaySize.x / texSize.x, m_displaySize.y / texSize.y);
        m_sprite.setScale(scale);
    }
};