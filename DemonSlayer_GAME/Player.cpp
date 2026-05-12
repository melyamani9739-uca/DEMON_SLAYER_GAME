// Player.cpp – Tanjiro controller with correct scaling for all sprites
#include "Player.h"
#include <iostream>

static constexpr float GROUND_Y        = 500.f;
static constexpr float GRAVITY         = 1900.f;
static constexpr float JUMP_VEL        = -720.f;
static constexpr float ATTACK_DURATION = 0.55f;
static constexpr float HIT_DURATION    = 0.4f;
static constexpr float INV_DURATION    = 1.4f;
static constexpr float TARGET_HEIGHT   = 190.f;   // match CHAR_HEIGHT in Game.cpp
static constexpr float DUCK_HEIGHT     = 85.f;
static constexpr float WALK_SPEED      = 160.f;

AnimClip Player::buildClip(sf::Texture& tex, int numFrames, float speed, bool loop) {
    AnimClip clip;
    clip.numFrames = numFrames;
    clip.speed     = speed;
    clip.loop      = loop;
    clip.frameW    = static_cast<int>(tex.getSize().x) / numFrames;
    clip.frameH    = static_cast<int>(tex.getSize().y);
    clip.rects.reserve(numFrames);
    for (int i = 0; i < numFrames; ++i)
        clip.rects.push_back(sf::IntRect(i * clip.frameW, 0, clip.frameW, clip.frameH));
    return clip;
}

Player::Player()
    : groundY(GROUND_Y), gravity(GRAVITY), velY(0.f),
      onGround(true), isDucking(false), forcedWalk(false),
      curState(RUN), prevState(RUN),
      frame(0), animTimer(0.f),
      attackTimer(0.f), hitTimer(0.f),
      lives(3), invTimer(0.f), visible(true),
      useLevel2Attack(false)
{
    auto load = [](sf::Texture& t, const char* path) {
        if (!t.loadFromFile(path))
            std::cerr << "[Player] Failed to load: " << path << "\n";
        t.setSmooth(true);
    };

    // Level 1 textures (original proportions, correct height)
    load(texRun,      "asseste/tanjiroRun.png");               // 669x140, 6 frames
    load(texAttack,   "asseste/tanjiro_attack.png");           // 669x152, 6 frames
    load(texJump,     "asseste/tanjiro_jump.png");             // 669x163, 6 frames
    load(texHit,      "asseste/tanjiroRun.png");               // fallback to run

    // Level 2 / Boss textures (cropped — real character height, no blank rows)
    load(texAttackL2, "asseste/tanjiro_attack_level2_cropped.png"); // ~669x144
    load(texWalk,     "asseste/tanjiro_walk_cropped.png");          // ~669x291

    // Base scale from RUN texture height (140px)
    float runH = static_cast<float>(texRun.getSize().y);
    scaleY = TARGET_HEIGHT / runH;
    scaleX = scaleY;

    clips[RUN]    = buildClip(texRun,    6, 0.10f, true);
    clips[ATTACK] = buildClip(texAttack, 6, 0.08f, false);
    clips[JUMP]   = buildClip(texJump,   6, 0.10f, false);
    clips[DUCK]   = buildClip(texRun,    6, 0.12f, true);
    clips[HIT]    = buildClip(texHit,    6, 0.08f, false);
    clips[DEAD]   = buildClip(texHit,    6, 0.12f, false);

    // Walk clip (cropped ~291px tall)
    {
        AnimClip wc;
        wc.numFrames = 6;
        wc.speed     = 0.10f;
        wc.loop      = true;
        wc.frameW    = static_cast<int>(texWalk.getSize().x) / 6;
        wc.frameH    = static_cast<int>(texWalk.getSize().y);
        for (int i = 0; i < 6; ++i)
            wc.rects.push_back(sf::IntRect(i * wc.frameW, 0, wc.frameW, wc.frameH));
        clips[WALK] = wc;
    }

    // Level2 attack clip (cropped ~144px tall)
    {
        AnimClip ac;
        ac.numFrames = 6;
        ac.speed     = 0.08f;
        ac.loop      = false;
        ac.frameW    = static_cast<int>(texAttackL2.getSize().x) / 6;
        ac.frameH    = static_cast<int>(texAttackL2.getSize().y);
        for (int i = 0; i < 6; ++i)
            ac.rects.push_back(sf::IntRect(i * ac.frameW, 0, ac.frameW, ac.frameH));
        clipsAttackL2 = ac;
    }

    sprite.setTexture(texRun);
    sprite.setTextureRect(clips[RUN].rects[0]);
    sprite.setScale(scaleX, scaleY);
    float fw = static_cast<float>(clips[RUN].frameW);
    float fh = static_cast<float>(clips[RUN].frameH);
    sprite.setOrigin(fw * 0.5f, fh);
    sprite.setPosition(180.f, groundY);
}

void Player::setUseLevel2Attack(bool v) {
    useLevel2Attack = v;
}

const AnimClip& Player::getActiveClip(State s) const {
    if (s == ATTACK && useLevel2Attack)
        return clipsAttackL2;
    auto it = clips.find(s);
    if (it != clips.end()) return it->second;
    return clips.at(RUN);
}

// Returns the scale so this state's sprite renders at TARGET_HEIGHT pixels tall
float Player::scaleForState(State s) const {
    const AnimClip& cl = getActiveClip(s);
    return TARGET_HEIGHT / static_cast<float>(cl.frameH);
}

void Player::update(float dt) {
    if (curState == DEAD) return;

    prevState = curState;

    bool upHeld    = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
    bool downHeld  = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
    bool attackKey = sf::Keyboard::isKeyPressed(sf::Keyboard::Z) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::X);

    if (forcedWalk) {
        sprite.move(WALK_SPEED * dt, 0.f);
        if (attackKey && curState != ATTACK && curState != HIT) {
            curState    = ATTACK;
            attackTimer = 0.f;
            frame       = 0;
            animTimer   = 0.f;
        } else if (curState != ATTACK && curState != HIT) {
            curState = WALK;
        }
        goto animate;
    }

    isDucking = downHeld && onGround && curState != ATTACK && curState != HIT;

    if (upHeld && onGround && curState != ATTACK && curState != HIT) {
        velY      = JUMP_VEL;
        onGround  = false;
        isDucking = false;
        curState  = JUMP;
    }

    if (attackKey && curState != ATTACK && curState != HIT) {
        curState    = ATTACK;
        attackTimer = 0.f;
        frame       = 0;
        animTimer   = 0.f;
    }

    switch (curState) {
        case ATTACK:
            attackTimer += dt;
            if (attackTimer >= ATTACK_DURATION)
                curState = onGround ? RUN : JUMP;
            break;
        case HIT:
            hitTimer += dt;
            if (hitTimer >= HIT_DURATION)
                curState = onGround ? RUN : JUMP;
            break;
        case JUMP:
            if (onGround) curState = RUN;
            break;
        case DUCK:
            if (!isDucking) curState = RUN;
            break;
        case RUN:
            if (isDucking) curState = DUCK;
            break;
        default: break;
    }

    if (!onGround) {
        velY += gravity * dt;
        sprite.move(0.f, velY * dt);
        if (sprite.getPosition().y >= groundY) {
            sprite.setPosition(sprite.getPosition().x, groundY);
            velY     = 0.f;
            onGround = true;
        }
    }

animate:
    if (curState != prevState) {
        frame     = 0;
        animTimer = 0.f;
        switch (curState) {
            case RUN:    sprite.setTexture(texRun);    break;
            case WALK:   sprite.setTexture(texWalk);   break;
            case ATTACK: sprite.setTexture(useLevel2Attack ? texAttackL2 : texAttack); break;
            case JUMP:   sprite.setTexture(texJump);   break;
            case DUCK:   sprite.setTexture(texRun);    break;
            case HIT:    sprite.setTexture(texHit);    break;
            case DEAD:   sprite.setTexture(texHit);    break;
        }
        const AnimClip& cl = getActiveClip(curState);
        sprite.setOrigin(cl.frameW * 0.5f, static_cast<float>(cl.frameH));
    }

    // Scale so every state renders at TARGET_HEIGHT tall
    float sc = scaleForState(curState);
    if (curState == DUCK) {
        sprite.setScale(sc * 1.2f, DUCK_HEIGHT / static_cast<float>(getActiveClip(DUCK).frameH));
    } else {
        sprite.setScale(sc, sc);
    }

    const AnimClip& clip = getActiveClip(curState);
    animTimer += dt;
    if (animTimer >= clip.speed) {
        animTimer = 0.f;
        frame++;
        if (frame >= clip.numFrames)
            frame = clip.loop ? 0 : clip.numFrames - 1;
    }
    if (frame < (int)clip.rects.size())
        sprite.setTextureRect(clip.rects[frame]);

    if (invTimer > 0.f) {
        invTimer -= dt;
        visible = (int)(invTimer * 10.f) % 2 == 0;
    } else {
        visible  = true;
        invTimer = 0.f;
    }
}

void Player::render(sf::RenderWindow& window) {
    if (!visible && invTimer > 0.f) return;
    window.draw(sprite);
}

void Player::reset() {
    curState    = RUN;
    prevState   = RUN;
    lives       = 3;
    frame       = 0;
    animTimer   = 0.f;
    attackTimer = 0.f;
    hitTimer    = 0.f;
    invTimer    = 0.f;
    visible     = true;
    velY        = 0.f;
    onGround    = true;
    isDucking   = false;
    forcedWalk  = false;
    useLevel2Attack = false;

    sprite.setTexture(texRun);
    sprite.setTextureRect(clips[RUN].rects[0]);
    const AnimClip& cl = clips[RUN];
    sprite.setOrigin(cl.frameW * 0.5f, static_cast<float>(cl.frameH));
    sprite.setScale(scaleX, scaleY);
    sprite.setPosition(180.f, 500.f);
    sprite.setColor(sf::Color::White);
}

void Player::stompBounce() {
    velY     = JUMP_VEL * 0.45f;
    onGround = false;
    curState = JUMP;
}

void Player::takeHit() {
    if (curState == DEAD || invTimer > 0.f) return;
    lives--;
    invTimer  = INV_DURATION;
    hitTimer  = 0.f;
    curState  = HIT;
    if (lives <= 0) die();
}

void Player::die() {
    lives    = 0;
    curState = DEAD;
    sprite.setColor(sf::Color(255, 60, 60, 200));
}
