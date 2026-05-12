#pragma once
#include <SFML/Graphics.hpp>

// Base class for all game entities (Player, Obstacle, etc.)
class Entity {
protected:
    sf::Sprite  sprite;
    sf::Texture texture;

public:
    Entity();
    virtual ~Entity() = default;

    virtual void update(float dt) = 0;
    virtual void render(sf::RenderWindow& window);

    sf::FloatRect getBounds() const;
    void          move(float dx, float dy);
};
