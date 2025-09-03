#include "BoxCollider.h"

BoxCollider::BoxCollider(sf::Sprite& spr_, float w_, float h_)
    : spr{ spr_ } {
    float hw = w_ / 2.f;
    float hh = h_ / 2.f;
    center.x = hw;
    center.y = hh;

    vertex[0].x = -hw;
    vertex[0].y = -hh;
    vertex[1].x = -hw;
    vertex[1].y = +hh;
    vertex[2].x = +hw;
    vertex[2].y = +hh;
    vertex[3].x = +hw;
    vertex[3].y = -hh;
}

BoxCollider::~BoxCollider() {}


sf::Sprite& BoxCollider::getSprite()
{

    return spr;
}

sf::Vector2f BoxCollider::getPos()
{
    return spr.getPosition() + center;
}

sf::Vector2f BoxCollider::getPoint(int idx_)
{
    if (idx_ >= 4 || idx_ < 0)
        return this->getPos();
    else
        return this->getPos() + vertex[idx_];
}

std::array<sf::Vector2f, 4> BoxCollider::getPoints()
{
    std::array<sf::Vector2f, 4> arr = { sf::Vector2f{0.f,0.f},  sf::Vector2f{0.f,0.f},  sf::Vector2f{0.f,0.f},  sf::Vector2f{0.f,0.f} };

    for (int i = 0; i < 4; i++)
    {
        arr[i] = getPoint(i);
    }
    return arr;
}