
#include "Obstacle.h"
#include <cmath>
#include <iostream>

static constexpr float GROUND_Y    = 500.f;
static constexpr float BIRD_Y      = 370.f;   // bird flies at Tanjiro head level (duck to dodge)
static constexpr float PLATFORM_Y  = 420.f;   // platform floats mid-air (jumpable from ground)

Obstacle::Obstacle(float yPos,
                   const sf::Texture& tex,
                   Type t,
                   int cols,
                   float desiredHeight)
    : type(t), hp(1), speed(-190.f), flyTimer(0.f),
      baseY(yPos), frame(0), animTimer(0.f), animSpeed(0.12f),
      collected(false),
      hasEnergie(false), energieTex(nullptr),
      energieAnimTimer(0.f), energieFrame(0), energieCollected(false)
{
    sprite.setTexture(tex);

    int texW = (int)tex.getSize().x;
    int texH = (int)tex.getSize().y;

    frameW    = texW / cols;
    frameH    = texH;
    numFrames = cols;

    sprite.setTextureRect(sf::IntRect(0, 0, frameW, frameH));

    targetScale = desiredHeight / (float)frameH;
    sprite.setScale(targetScale, targetScale);
    // Origin bottom-centre = feet/base on the yPos
    sprite.setOrigin(frameW * 0.5f, (float)frameH);

    switch (type) {
        case GROUND:
            // demon2 on ground — 1 frame, full image
            sprite.setPosition(920.f, GROUND_Y);
            speed = -190.f;
            hp    = 1;
            animSpeed = 0.15f;
            break;

        case AIR:
            // demon1 crow — 4 animated frames (148px each), flies at head level
            baseY = BIRD_Y;
            sprite.setPosition(920.f, BIRD_Y);
            speed = -220.f;
            hp    = 1;
            animSpeed = 0.13f;  // wing flap speed
            break;

        case PLATFORM:
            // Floating platform — obstacle.png frame 1 (middle col) = complete platform
            baseY = PLATFORM_Y;
            sprite.setPosition(920.f, PLATFORM_Y);
            speed = -180.f;
            hp    = 1;
            frame = 1;  // col 1 = the full clean platform sprite
            sprite.setTextureRect(sf::IntRect(1 * frameW, 0, frameW, frameH));
            animSpeed = 999.f; // no animation for platform
            break;

        case ROCK:
            sprite.setPosition(920.f, GROUND_Y);
            speed = -320.f;
            hp    = 1;
            animSpeed = 999.f;
            break;

        case COLLECTIBLE:
            baseY = GROUND_Y - 50.f;  // above ground so it's clearly visible
            sprite.setPosition(920.f, baseY);
            speed = -200.f;
            hp    = 1;
            animSpeed = 999.f;  // no animation — energie is a single picture
            // Use frame 2 which has the orb content in energie.png
            frame = 0;
            sprite.setTextureRect(sf::IntRect(0, 0, frameW, frameH));
            break;

        case BOSS:
            sprite.setPosition(750.f, GROUND_Y);
            speed = -80.f;
            hp    = 10;
            break;
    }
}

// ── Attach energie on top of platform ───────────────────────────────────────
void Obstacle::attachEnergie(const sf::Texture& tex) {
    hasEnergie       = true;
    energieTex       = &tex;
    energieCollected = false;
    energieFrame     = 0;   // single static frame
    energieAnimTimer = 0.f;

    energieSprite.setTexture(tex);
    int ew = (int)tex.getSize().x;   // full width (single frame)
    int eh = (int)tex.getSize().y;
    energieSprite.setTextureRect(sf::IntRect(0, 0, ew, eh));

    float energieH = 70.f;               // visible height in world pixels
    float sc = energieH / (float)eh;
    energieSprite.setScale(sc, sc);
    energieSprite.setOrigin(ew * 0.5f, (float)eh); // origin at bottom → sits ON platform surface
}

// ── Top surface Y of platform ────────────────────────────────────────────────
float Obstacle::getPlatformTopY() const {
    // obstacle.png frame 1: surface top is at pixel row 48 out of 353
    // That means the surface is 305/353 = 0.864 of total height from the bottom
    // Origin is at bottom of sprite, so: surfaceY = pos.y - scaledHeight * 0.864
    float scaledH = (float)frameH * targetScale;
    return sprite.getPosition().y - scaledH * 0.864f;
}

sf::FloatRect Obstacle::getEnergieBounds() const {
    if (!hasEnergie || energieCollected) return sf::FloatRect();
    return energieSprite.getGlobalBounds();
}

// ── Update ───────────────────────────────────────────────────────────────────
void Obstacle::update(float dt) {
    if (collected) return;

    sprite.move(speed * dt, 0.f);

    if (type == BOSS) {
        float x = sprite.getPosition().x;
        if (x < 320.f) speed =  std::abs(speed);
        if (x > 750.f) speed = -std::abs(speed);
    }

    // Bird: gentle sine wave while flying high
    if (type == AIR) {
        flyTimer += dt;
        float wave = std::sin(flyTimer * 3.5f) * 18.f;
        sprite.setPosition(sprite.getPosition().x, baseY + wave);
    }

    // Ground demon: subtle bounce/sway to look alive
    if (type == GROUND) {
        flyTimer += dt;
        float bob = std::abs(std::sin(flyTimer * 4.f)) * 8.f;  // bounces up
        sprite.setPosition(sprite.getPosition().x, baseY - bob);
    }

    // Collectible: gentle bob
    if (type == COLLECTIBLE) {
        flyTimer += dt;
        float bob = std::sin(flyTimer * 5.f) * 10.f;
        sprite.setPosition(sprite.getPosition().x, baseY + bob);
    }

    // Frame animation (only if multi-frame, not platform, not collectible)
    if (numFrames > 1 && type != PLATFORM && type != COLLECTIBLE) {
        animTimer += dt;
        if (animTimer >= animSpeed) {
            animTimer = 0.f;
            frame = (frame + 1) % numFrames;
            sprite.setTextureRect(sf::IntRect(frame * frameW, 0, frameW, frameH));
        }
    }

    // Update energie position (sits on top of platform) — static, no animation
    if (hasEnergie && !energieCollected && type == PLATFORM) {
        float topY = getPlatformTopY();
        float px   = sprite.getPosition().x;
        energieSprite.setPosition(px, topY);
    }
}

// ── Render ───────────────────────────────────────────────────────────────────
void Obstacle::render(sf::RenderWindow& window) {
    if (collected) return;
    window.draw(sprite);
    if (hasEnergie && !energieCollected)
        window.draw(energieSprite);
}

bool Obstacle::isOffScreen() const {
    return sprite.getPosition().x < -300.f;
}
