#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>

struct AnimClip {
    std::vector<sf::IntRect> rects;
    int   frameW    = 0;
    int   frameH    = 0;
    int   numFrames = 1;
    float speed     = 0.12f;
    bool  loop      = true;
};

class Player {
public:
    enum State { RUN, WALK, JUMP, ATTACK, DUCK, HIT, DEAD };

private:
    sf::Texture texRun, texAttack, texAttackL2, texJump, texHit, texWalk;
    sf::Sprite  sprite;

    float groundY, gravity, velY;
    bool  onGround, isDucking;
    bool  forcedWalk;
    bool  useLevel2Attack;

    State curState, prevState;

    std::unordered_map<State, AnimClip> clips;
    AnimClip clipsAttackL2;
    int   frame;
    float animTimer;
    float attackTimer, hitTimer;

    int   lives;
    float invTimer;
    bool  visible;

    float scaleX, scaleY;

    AnimClip buildClip(sf::Texture& tex, int numFrames, float speed, bool loop);
    const AnimClip& getActiveClip(State s) const;
    float scaleForState(State s) const;

public:
    Player();
    void update(float dt);
    void render(sf::RenderWindow& window);
    void reset();
    void takeHit();
    void die();
    void stompBounce();
    void forceWalk(bool w)           { forcedWalk = w; }
    void setUseLevel2Attack(bool v);

    int   getLives()     const { return lives; }
    State getState()     const { return curState; }
    bool  isDead()       const { return curState == DEAD; }
    bool  isAttacking()  const { return curState == ATTACK; }
    bool  getIsDucking() const { return isDucking; }
    sf::FloatRect getBounds()   const { return sprite.getGlobalBounds(); }
    sf::Vector2f  getPosition() const { return sprite.getPosition(); }
    void          setPosition(float x, float y) { sprite.setPosition(x, y); }

    void landOnPlatform(float surfaceY) {
        sprite.setPosition(sprite.getPosition().x, surfaceY);
        velY     = 0.f;
        onGround = true;
    }
    void  setGroundY(float y) { groundY = y; }
    float getGroundY()  const { return groundY; }
    bool  isOnGround()  const { return onGround; }
    float getVelY()     const { return velY; }
    void  setOnGround(bool v) { onGround = v; }
    void setScale(float x, float y) {
    scaleX = x;
    scaleY = y;
    sprite.setScale(x, y);
}
};
