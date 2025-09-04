#include "util.h"

sf::Vector2f util::toIso(sf::Vector2f cart)
{
    // cart is a position on the screen, lets strip it to cell space
    cart.x /= 128;
    cart.y /= 64;

    float xIso = (cart.x - cart.y) * (128.f / 2.f);
    float yIso = (cart.x + cart.y) * (64.f / 2.f);
    return { xIso,yIso };
}

sf::Vector2f util::toCart(float screenX, float screenY, float tileSize) {
    float mapx = (screenX / tileSize + screenY / (tileSize * 0.5f)) * 0.5f;
    float mapy = (screenY / (tileSize * 0.5f) - (screenX / tileSize)) * 0.5f;
    return sf::Vector2f{ mapx, mapy };
}


