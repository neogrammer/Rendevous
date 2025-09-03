#ifndef ISOTILECOLLIDER_H__
#define ISOTILECOLLIDER_H__
#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <algorithm>
#include "Collider.h"

class IsoTileCollider : public Collider
{
    sf::Sprite& spr;
    sf::Vector2f center{ 64.f, 32.f };
    std::vector<sf::Vector2f> vertex = {
        {64.f, 0.f},
        {0.f,-32.f},
        {-64.f,0.f},
        {0.f, 32.f}
    };	// "Model"

public:

    IsoTileCollider() = delete;

    explicit IsoTileCollider(sf::Sprite& spr_);

    ~IsoTileCollider();

    IsoTileCollider(const IsoTileCollider&) = delete;
    IsoTileCollider& operator=(const IsoTileCollider&) = delete;
    IsoTileCollider(IsoTileCollider&&) = delete;
    IsoTileCollider& operator=(IsoTileCollider&&) = delete;

    sf::Sprite& getSprite();
    sf::Vector2f getPos();
    sf::Vector2f getPoint(int idx_);

    std::array<sf::Vector2f, 4> getPoints();

    sf::Vector2f getCenter() override;

};

#endif
