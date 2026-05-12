#include "Entity.h"

Entity::Entity() {}

void Entity::render(sf::RenderWindow& window) {
    window.draw(sprite);
}

sf::FloatRect Entity::getBounds() const {
    return sprite.getGlobalBounds();
}

void Entity::move(float dx, float dy) {
    sprite.move(dx, dy);
}
