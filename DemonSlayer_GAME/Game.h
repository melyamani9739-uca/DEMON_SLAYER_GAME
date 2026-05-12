#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include "Player.h"
#include "Obstacle.h"

enum class GameState {
    LEVEL1, LEVEL1_COMPLETE,
    LEVEL2,
    BOSS_APPROACH, BOSS_FIGHT,
    VICTORY, GAMEOVER
};

class Game {
public:
    enum BackgroundType { BG_LEVEL1, BG_LEVEL2, BG_BOSS, BG_FINALE };

private:
    Player player;

    // Level 1
    sf::Texture texDemon2, texDemon1, texObstacle, texEnergie;

    // Level 2
    sf::Texture texRock, texMoney, texDemon2Fly;

    // Boss — Akaza using actual asset images (669x373, 6 frames, 111px wide each)
    sf::Texture texAkazaWalk;      // Akaza_walk.png   – walk animation
    sf::Texture texAkazaAttack;    // Akaza_attack.png – attack animation

    // Tanjiro level2/boss attack texture
    sf::Texture texTanjiroAttackL2; // tanjiro_attack_level2.png

    // Finisher sprites
    sf::Texture texFinisherDead;   // tanjiro_dead.png

    // Backgrounds
    sf::Texture texBgL1a, texBgL1b, texBgL2a, texBgL2b, texBgFinale;
    sf::Texture texBgGameOver;     // gameover_bg.jpg
    sf::Sprite  bg1, bg2;
    float scrollSpeed;

    std::vector<Obstacle> obstacles;
    sf::Clock spawnClock;
    float spawnInterval;

    GameState state;
    float levelTimer;
    int   score, highScore, moneyCount;

    float bossApproachTimer, level1CompleteTimer;

    float demon5AnimT;
    int   demon5Frame, demon5NumFrames;

    // ── Akaza boss ────────────────────────────────────────────────────────────
    sf::Sprite akazaSprite;
    int   akazaHP;
    static constexpr int AKAZA_MAX_HP = 3;

    enum class AkazaState { WALK, ATTACK, HIT };
    AkazaState akazaState;

    int   akazaAnimFrame;
    float akazaAnimTimer;
    float akazaInvTimer;
    float akazaDeathTimer;
    float akazaSpeed;
    float playerHitCooldown;
    bool  akazaDead;

    // Finisher overlay
    sf::Sprite finSprite;
    int        finFrame;
    float      finAnimTimer;
    float      finDoneTimer;
    int        finNumFrames;
    bool       finisherActive;
    bool       finisherIsKill;
    bool       finisherZPressed;

    float charScale;

    sf::Font font;
    sf::Text txtScore, txtTimer, txtLives, txtLevel, txtBig, txtHint, txtMoney;
    sf::RectangleShape heartShapes[3];
    sf::RectangleShape bossBarBg, bossBarFill;
    sf::RectangleShape flashOverlay, killFlashOverlay;
    float flashTimer, killFlashTimer, victoryTextAlpha;

    // ── Sounds ────────────────────────────────────────────────────────────────
    sf::SoundBuffer sbWin, sbMoney, sbDemon, sbKillAkaza;
    sf::Sound       sndWin, sndMoney, sndDemon, sndKillAkaza;
    bool            winSoundPlayed;

    void loadSounds();
    void playSound(sf::Sound& snd);

    void spawnObstacle();
    void switchBackground(BackgroundType t);
    void initUI();
    void updateUI();
    void setupAkaza();
    void updateAkaza(float dt);
    void renderAkaza(sf::RenderWindow& w);
    void setAkazaTexture(sf::Texture& tex, int nf);

public:
    Game();
    void handleEvent(const sf::Event& e);
    void update(float dt);
    void render(sf::RenderWindow& window);
    void reset();
};
