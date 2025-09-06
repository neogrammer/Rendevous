#include "util.h"
#include "../../../SolutionHelpers.h"

sf::Vector2f util::toIso(sf::Vector2f cart)
{
    // cart is a position on the screen, lets strip it to cell space
    cart.x /= 128;
    cart.y /= 64;

    float xIso = (cart.x - cart.y) * (128.f / 2.f);
    float yIso = (cart.x + cart.y) * (64.f / 2.f);
    return { xIso,yIso };
}


sf::Vector2f util::toCart(sf::Vector2f iso) {
    const float TW = (float)hlp::tileSize.first; // or tileSize.first
    const float TH = (float)hlp::tileSize.second;  // or tileSize.second
    const float x = iso.x, y = iso.y;

    const float cartX = x + (TW / TH) * y;
    const float cartY = y - (TH / TW) * x;
    return { cartX, cartY };
}
