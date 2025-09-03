#ifndef COLLIDER_H__
#define COLLIDER_H__

#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <algorithm>

class Collider
{
public:

    virtual ~Collider();

    virtual sf::Sprite& getSprite() = 0;
    virtual sf::Vector2f getPos() = 0;
    virtual sf::Vector2f getPoint(int idx_) = 0;
    virtual std::array<sf::Vector2f, 4> getPoints() = 0;
    virtual sf::Vector2f getCenter() = 0;
};


#endif
