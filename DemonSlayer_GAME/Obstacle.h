#pragma once
#include "Entity.h"

class Obstacle : public Entity {
public:
    // GROUND  = demon on ground, attack to kill
    // AIR     = bird flying high, must duck
    // BOSS    = Akaza
    // PLATFORM= floating platform; energie sits on top
    // ROCK    = fast ground rock in level 2
    // COLLECTIBLE = floating pickup (level 2 money)
    enum Type { GROUND, AIR, BOSS, PLATFORM, ROCK, COLLECTIBLE };

private:
    Type  type;
    int   hp;
    float speed;
    float flyTimer;
    float baseY;

    int   numFrames;
    int   frame;
    float animTimer;
    float animSpeed;
    int   frameW;
    int   frameH;
    float targetScale;

    bool  collected;

    // For PLATFORM: a paired energie sprite drawn on top
    bool  hasEnergie;
    sf::Sprite  energieSprite;
    const sf::Texture* energieTex;
    float energieAnimTimer;
    int   energieFrame;
    bool  energieCollected;

public:
    Obstacle(float yPos,
             const sf::Texture& tex,
             Type type,
             int cols,
             float desiredHeight = 100.f);

    // Attach an energie collectible on top of this platform
    void attachEnergie(const sf::Texture& tex);

    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    Type  getType()           const { return type; }
    int   getHP()             const { return hp; }
    bool  isOffScreen()       const;
    bool  isCollected()       const { return collected; }
    void  collect()                 { collected = true; }
    void  takeDamage()              { if (hp > 0) hp--; }
    float getBaseY()          const { return baseY; }
    float getPlatformTopY()   const;
    float getSpeedX()         const { return speed; }  // for moving-with-platform
    bool  hasEnergiePickup()  const { return hasEnergie && !energieCollected; }
    bool  collectEnergie()          { if (hasEnergie && !energieCollected) { energieCollected = true; return true; } return false; }
    sf::FloatRect getEnergieBounds() const;
    sf::Vector2f  getPos()    const { return sprite.getPosition(); }
};
