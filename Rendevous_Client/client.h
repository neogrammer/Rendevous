#ifndef CLIENT_H__
#define CLIENT_H__
#include <cid_net.h>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include "../Network_Common.h"
#include <vector>
#include <array>
#include <SFML/Audio.hpp>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <assert.h>
#include <cstdint>
#include "../SnapshotTiles.h"
#include <../Macros.h>
#include "../SolutionHelpers.h"



constexpr int playerWidth = 299;
constexpr int playerHeight = 240;
constexpr float playerZHeightOffset = 153.f;
struct TileLayer
{
    int width, height;        // in tiles
    int tileSize;             // pixels
    const uint32_t* data;     // map[t] = tileset index at that map cell
};


class Client : public cnet::client_interface<Msg>
{
    sf::RectangleShape cellShp;
    uint32_t cellShpX{};
    uint32_t cellShpY{};
    uint32_t cellOffsetX{};
    uint32_t cellOffsetY{};


public:
    sf::Vector2f mpos{0,0};
    const unsigned SCRW = 1980;
    const unsigned SCRH = 1024;
    typedef std::array<std::vector<sf::IntRect>, 8> AnimSheet;
    using AnimSheet = std::array<std::vector<sf::IntRect>, 8>;
    Client();
    ~Client() override;

    bool OnUserCreate();
    void run(); 

private:
    std::unordered_map<uint32_t, sf::Vector2f> tileCollidePos{};
    std::unordered_map < uint32_t, bool> tileCollided{};

    void handleWindowEvents(sf::RenderWindow& wnd_);
    void processInput();
    bool loadMap(uint32_t** data, int numElems, const std::string& filename);
    void displayText(sf::RenderWindow& wnd_, const sf::Vector2f& mouseStr_, const sf::Vector2i& cellStr_, const sf::Vector2f& selectedStr_, const sf::Vector2i offsetStr_);
    void initAssets();
    bool Update(float fElapsedTime);
    void Render();
    void sendMessageMsg(const TEXT t);
    uint32_t animIndex(AnimID a);
    uint32_t dirIndex(Dir d);
    sf::Texture& getPlayerTex(AnimID id_);
    float getPlayerZHeight(uint32_t playerID);

    sf::RenderWindow* wnd;
    sf::ContextSettings settings;
    sf::Clock timer{};

    sf::Texture tilesetTex{ "assets/textures/blocksTileSheet128x128.png" };
    sf::Font font1{ "assets/font/font1.ttf" };
    std::array<sf::Texture, 3> playerTexArr = {
        sf::Texture{"assets/textures/idle_sheet.png"},
        sf::Texture{"assets/textures/run_sheet.png"},
        sf::Texture{"assets/textures/attack_sheet.png"}
    };
    uint32_t mapData[1800] = { /* (kept as your existing data) */ };

    Dir    playerDir = Dir::D;
    AnimID playerAnimID = AnimID::Idle;
    sf::Vector2f playerPos{ 100.f,200.f };
    uint32_t playerAnimFrameIdx = 0;
    sPlayerDescription descPlayer;
    uint32_t nPlayerID = 0;
    PlayerIO myIO{};

    sf::Sprite selected{ tilesetTex };

    std::unordered_map<uint32_t, sPlayerDescription> mapObjects; // who exists
    std::unordered_map<uint32_t, PlayerDrawData>     drawObjects; // last snapshot data
    std::array<AnimSheet, 3> playerAnimFrames; // [0]=Idle, [1]=Run
    std::deque<sMessageFromClient> messages;
    std::vector<sf::Sprite> tilemap;
    std::vector<sf::Sprite> tileset;
    
    std::string txtStr = "";
    bool moving{ false };
    bool inEditMode = false;
    bool bWaitingForConnection = true;
    bool isAttacking = false;
    int currTSetRows = 0;
    int currTSetCols = 0;
    bool startingAttack = false;

};

#endif