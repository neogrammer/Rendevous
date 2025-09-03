#ifndef BOXCOLLIDER_H__
#define BOXCOLLIDER_H__
#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <algorithm>
#include "Collider.h"


class BoxCollider : public Collider
{
    sf::Sprite& spr;
    sf::Vector2f center{ 30.f, 30.f };
    std::vector<sf::Vector2f> vertex = {
        { -30, -30 },
        { -30, +30 },
        { +30, +30 },
        { +30, -30 }
    };	// "Model"

public:

    BoxCollider() = delete;
    explicit BoxCollider(sf::Sprite& spr_, float w_ = 60.f, float h_ = 60.f);
    ~BoxCollider();

    BoxCollider(const BoxCollider&) = delete;
    BoxCollider& operator=(const BoxCollider&) = delete;
    BoxCollider(BoxCollider&&) = delete;
    BoxCollider& operator=(BoxCollider&&) = delete;

    sf::Sprite& getSprite();

    sf::Vector2f getPos();

    sf::Vector2f getPoint(int idx_);

    std::array<sf::Vector2f, 4> getPoints();

    sf::Vector2f getCenter() override;

};


#endif
