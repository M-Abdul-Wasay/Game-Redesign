#include <iostream>
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cstdlib>

class NPCCharacter
{
public:
    float speed = 60.f; // usually slower than the player, tweak to taste
    float walkFrameDuration = 0.35f;
    float idleFrameDuration = 0.5f;
    sf::FloatRect getCollisionBox() const
        {
            sf::Vector2f pos = m_sprite.getPosition();
            return sf::FloatRect(
                pos.x - collisionBoxSize.x / 2.f,
                pos.y + m_displaySize.y / 2.f - collisionBoxSize.y,
                collisionBoxSize.x,
                collisionBoxSize.y
            );
        }
    std::string walkAnimationName = "Walking";
    std::string idleAnimationName = "breathing-idle";

    bool loadAllAnimations(const std::string& basePath, sf::Vector2f startPos, sf::Vector2f displaySize)
    {
        std::vector<std::string> animationNames = { "breathing-idle", "Walking" };
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
                        std::cerr << "Warning: NPC could not load " << fullPath << std::endl;
                    }
                }

                if (loadedAny)
                    m_animations[anim][dir] = frames;
            }
        }

        if (m_animations.empty())
        {
            std::cerr << "Error: no NPC animation frames loaded. Check basePath." << std::endl;
            return false;
        }

        m_sprite.setPosition(startPos);
        m_displaySize = displaySize;
        pickNewWanderTarget();
        updateFrameTexture();
        return true;
    }
    bool loadIdleFromRotation(const std::string& rotationPath)
    {
        std::vector<std::string> directionNames = {
            "north", "north-east", "east", "south-east",
            "south", "south-west", "west", "north-west"
        };

        bool loadedAny = false;
        for (const auto& dir : directionNames)
        {
            std::string fullPath = rotationPath + "/" + dir + ".png"; // <-- adjust filename pattern if yours differ

            sf::Texture tex;
            if (tex.loadFromFile(fullPath))
            {
                m_animations[idleAnimationName][dir] = { tex }; // single-frame "animation"
                loadedAny = true;
            }
            else
            {
                std::cerr << "Warning: NPC could not load rotation frame " << fullPath << std::endl;
            }
        }
        return loadedAny;
    }
    void update(float deltaTime, const std::vector<sf::FloatRect>& collisionRects)
    {
        m_stateTimer -= deltaTime;

        if (m_stateTimer <= 0.f)
        {
            if (m_isMoving)
                enterIdleState();
            else
                pickNewWanderTarget();
        }

        if (m_isMoving)
        {
            sf::Vector2f delta = m_moveDir * speed * deltaTime;

            m_sprite.move(delta.x, 0.f);
            if (isColliding(collisionRects))
            {
                m_sprite.move(-delta.x, 0.f);
                enterIdleState(); // hit something — stop and pick a new direction next
            }

            m_sprite.move(0.f, delta.y);
            if (isColliding(collisionRects))
            {
                m_sprite.move(0.f, -delta.y);
                enterIdleState();
            }
        }

        updateAnimation(deltaTime);
    }

    void draw(sf::RenderWindow& window)
    {
        window.draw(m_sprite);
    }

    sf::Vector2f getPosition() const
    {
        return m_sprite.getPosition();
    }

private:
    std::map<std::string, std::map<std::string, std::vector<sf::Texture>>> m_animations;
    sf::Sprite m_sprite;
    sf::Vector2f m_displaySize;
    sf::Vector2f collisionBoxSize = sf::Vector2f(20.f, 12.f);

    std::string m_currentAnimation = "breathing-idle";
    std::string m_currentDirection = "south";
    int m_currentFrame = 0;
    float m_frameTimer = 0.f;
    bool m_isMoving = false;

    sf::Vector2f m_moveDir = sf::Vector2f(0.f, 0.f);
    float m_stateTimer = 0.f;

    void pickNewWanderTarget()
    {
        static const std::vector<std::pair<sf::Vector2f, std::string>> directions = {
            { sf::Vector2f(0, -1), "north" },
            { sf::Vector2f(0.7f, -0.7f), "north-east" },
            { sf::Vector2f(1, 0), "east" },
            { sf::Vector2f(0.7f, 0.7f), "south-east" },
            { sf::Vector2f(0, 1), "south" },
            { sf::Vector2f(-0.7f, 0.7f), "south-west" },
            { sf::Vector2f(-1, 0), "west" },
            { sf::Vector2f(-0.7f, -0.7f), "north-west" }
        };

        int idx = std::rand() % directions.size();
        m_moveDir = directions[idx].first;
        m_currentDirection = directions[idx].second;
        m_isMoving = true;

        // walk for a random duration between 1 and 3 seconds
        m_stateTimer = 1.f + static_cast<float>(std::rand() % 200) / 100.f;
    }

    void enterIdleState()
    {
        m_isMoving = false;
        // idle for a random duration between 1 and 2.5 seconds
        m_stateTimer = 1.f + static_cast<float>(std::rand() % 150) / 100.f;
    }

   void updateAnimation(float deltaTime)
    {
        // fall back to the walk animation if this NPC has no idle set loaded at all —
        // prevents freezing on a stale mid-walk frame when idle frames are missing
        bool hasIdle = m_animations.find(idleAnimationName) != m_animations.end();
        std::string targetAnim = (m_isMoving || !hasIdle) ? walkAnimationName : idleAnimationName;

        if (targetAnim != m_currentAnimation)
        {
            m_currentAnimation = targetAnim;
            m_currentFrame = 0;
            m_frameTimer = 0.f;
        }

        if (m_isMoving)
        {
            m_frameTimer += deltaTime;
            if (m_frameTimer >= walkFrameDuration)
            {
                m_frameTimer = 0.f;
                int frameCount = getFrameCount();
                if (frameCount > 0)
                    m_currentFrame = (m_currentFrame + 1) % frameCount;
            }
        }
        else if (hasIdle)
        {
            m_frameTimer += deltaTime;
            if (m_frameTimer >= idleFrameDuration)
            {
                m_frameTimer = 0.f;
                int frameCount = getFrameCount();
                if (frameCount > 0)
                    m_currentFrame = (m_currentFrame + 1) % frameCount;
            }
        }
        else
        {
            // no idle frames for this NPC — freeze on the walk cycle's first frame
            // (a clean standing pose) instead of cycling or freezing mid-step
            m_currentFrame = 0;
        }

        updateFrameTexture();
    } 
    
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
};