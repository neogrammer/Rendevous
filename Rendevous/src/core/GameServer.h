#ifndef GAMESERVER_H__
#define GAMESERVER_H__


#include "net_server.h"
#include <iostream>
#include "cid_net.h"
#include <SFML/Graphics.hpp>
#include "net_common.h"
#include <unordered_map>
#include "../Network_Common.h"
#include <vector>
#include <map>
#include <cmath>
#include <cstdlib>
#include "../SnapshotTiles.h"
#include "./src/CollisionSystem/Colliders.h"
#include "./src/CollisionSystem/util.h"
#include "../SolutionHelpers.h"

#include "./src/CollisionSystem/Physics.h"




class GameServer : public cnet::server_interface<Msg>
{


public:
    sf::Vector2f getPlayerColliderPos(sf::Sprite& player_);
    sf::Vector2f getTileColliderPos(sf::Sprite& spr_);

    bool updateState();
    void renderServerDisplay(sf::RenderWindow& window);
    void handleInput(PlayerIO& pIO, PlayerData& pData);
    void animate(PlayerIO& pIO, PlayerData& pData, float dt);
    bool initAssets(); 
    bool loadMap(uint32_t** data, int numElems, const std::string& filename);
public:
    bool hasTilesToCheck{ false };
public:
    std::string myMsg{ "" };
    std::vector<std::string> myStrings{};
    // Sim state
    std::unordered_map<uint32_t, PlayerCollisionTiles> playerCollideTiles;
    std::unordered_map<uint32_t, PlayerData> playerDataMap;
    sf::Texture dummyTex{ "assets/textures/isometric_demo.png" };
    float zHeightOffset = 156.f;
    std::unordered_map<uint32_t, PlayerIO>   playerIOMap;
    std::vector<sf::Sprite> tilemap;
    std::vector<int> currTSetDetails = {
        1, 1, 0, 0, 0, 2, 3, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3,
        0, 1, 0, 0, 0, 2, 3, 0, 0, 0, 0,
        0, 2, 3, 0, 0, 2, 3, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0,
        0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0,
        0, 0, 2, 3, 0, 2, 3, 0, 2, 3, 0,
        0, 2, 3, 0, 2, 3, 0, 0, 2, 3, 0,
        0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0

    };
    uint32_t mapData[1800] = { /* (kept as your existing data) */ };
    int currTSetCols{0};
    int currTSetRows{ 0 };
public:

    // Debug
    // Text
    std::map<uint32_t, std::vector<TEXT>> m_messagesToDisplay;
    std::unordered_map<TEXT, std::string> m_msgStrLUT = {
        {TEXT::POOPSIE, ""},
    };
    std::unordered_map<uint32_t, sPlayerDescription> m_mapPlayerRoster;
    std::vector<uint32_t> m_vGarbageIDs;
public:
    GameServer(uint16_t nPort);
protected:
    bool OnClientConnect(std::shared_ptr<cnet::connection<Msg>> client);
    void OnClientValidated(std::shared_ptr<cnet::connection<Msg>> client);
    void OnClientDisconnect(std::shared_ptr<cnet::connection<Msg>> client);
    void handleGarbageRemovals();
    void OnMessage(std::shared_ptr<cnet::connection<Msg>> client, cnet::message<Msg>& msg);
public:
    void Simulate(float dt);
    void BroadcastSnapshot();
};

#endif