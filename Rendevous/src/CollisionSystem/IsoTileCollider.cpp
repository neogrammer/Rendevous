#include "IsoTileCollider.h"

IsoTileCollider::IsoTileCollider(sf::Sprite& spr_)
    : spr{ spr_ } {
}

IsoTileCollider::~IsoTileCollider() {}


sf::Sprite& IsoTileCollider::getSprite()
{

    return spr;
}

sf::Vector2f IsoTileCollider::getPos()
{
    return spr.getPosition() + center;
}

sf::Vector2f IsoTileCollider::getPoint(int idx_)
{
    if (idx_ >= 4 || idx_ < 0)
        return this->getPos();
    else
        return this->getPos() + vertex[idx_];
}

std::array<sf::Vector2f, 4> IsoTileCollider::getPoints()
{
    std::array<sf::Vector2f, 4> arr = { sf::Vector2f{0.f,0.f},  sf::Vector2f{0.f,0.f},  sf::Vector2f{0.f,0.f},  sf::Vector2f{0.f,0.f} };

    for (int i = 0; i < 4; i++)
    {
        arr[i] = getPoint(i);
    }
    return arr;
}