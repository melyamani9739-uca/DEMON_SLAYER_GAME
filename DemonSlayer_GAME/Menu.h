#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct MenuItem {
    sf::Text label;
    sf::Text sub;
    bool     enabled = true;
};

enum class MenuAction { NONE, PLAY, CONTINUE, SETTINGS, CONTROLS, QUIT, BACK };
enum class MenuScreen { MAIN, SETTINGS, CONTROLS };

class Menu {
private:
    sf::Font       font;
    sf::Texture    bgTex;
    sf::Sprite     bgSprite;
    sf::Texture    logoTex;
    sf::Sprite     logoSprite;

    sf::RectangleShape overlay;

    struct Particle { sf::CircleShape shape; sf::Vector2f vel; float life; };
    std::vector<Particle> particles;

    std::vector<MenuItem> items;
    int    selectedIdx;
    float  hoverAnim, hoverTarget;

    MenuScreen screen;
    bool  sfxEnabled, musicEnabled;
    float volume;

    sf::Text controlsText, backHint;
    float time, titlePulse;

    void buildMainItems();
    void buildSettingsItems();
    void spawnParticle();
    void updateParticles(float dt);
    sf::Color lerpColor(sf::Color a, sf::Color b, float t) const;
    MenuAction activateSelected();

public:
    Menu();
    MenuAction handleEvent(const sf::Event& e);
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool  isSfxEnabled()   const { return sfxEnabled; }
    bool  isMusicEnabled() const { return musicEnabled; }
    float getVolume()      const { return volume; }
    MenuScreen getScreen() const { return screen; }
};
