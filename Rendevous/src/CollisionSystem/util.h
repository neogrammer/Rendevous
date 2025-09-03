#ifndef UTIL_H__
#define UTIL_H__
#include <SFML/Graphics.hpp>
namespace util {
    extern sf::Vector2f toIso(sf::Vector2f cart);
    extern sf::Vector2f toCart(float screenX, float screenY, float tileSize);
};

#endif
