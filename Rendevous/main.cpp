#include "./src/core/GameServer.h"

void handleWindowEvents(sf::RenderWindow& window);
int main(int argc, char* argv[])
{
    GameServer server(60000);
    server.Start();

    server.initAssets();

    sf::RenderWindow window(sf::VideoMode({ 1600, 900 }), "GameServer!");
    if (!window.isOpen()) return 420;
    window.setPosition(sf::Vector2i(1800, 500));

    // Hide console window
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    // Fixed timestep
    const float dt = 1.0f / 60.0f;
    float accumulator = 0.f;
    sf::Clock clock;

    while (window.isOpen())
    {
        server.Update(10);
        handleWindowEvents(window);
       server.updateState();
        server.BroadcastSnapshot();
        // Render the server window, able to debug remotely
        server.renderServerDisplay(window);
    }
    return 0;
}

void handleWindowEvents(sf::RenderWindow& window)
{
    while (const std::optional ev = window.pollEvent())
    {
        if (ev->is<sf::Event::Closed>()) { window.close(); }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
    {
        window.close();
    }
}