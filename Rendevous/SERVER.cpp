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
#include "./src/CollisionSystem/Physics.h"



struct LatestNeighborhood {
    uint32_t seq = 0;
    int32_t  cx = 0, cy = 0;
    rect r{};
    std::array<uint32_t, 9> tiles{};
    bool valid = false;
};


std::unordered_map<uint32_t, LatestNeighborhood> m_neighborhoods; // by playerId


// Helpers
static inline bool aabbOverlap(float ax0, float ay0, float ax1, float ay1,
    float bx0, float by0, float bx1, float by1)
{
    return (ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0);
}

class GameServer : public cnet::server_interface<Msg>
{
    void handleWindowEvents(sf::RenderWindow& window);
public:
    sf::Vector2f getPlayerColliderPos( sf::Sprite& player_);
    sf::Vector2f getTileColliderPos(sf::Sprite& spr_);

    bool updateState();
    void renderServerDisplay(sf::RenderWindow& window);
    void handleInput(PlayerIO& pIO, PlayerData& pData);
    void animate(PlayerIO& pIO, PlayerData& pData, float dt);
    public:
    bool hasTilesToCheck{ false };
public:
    std::string myMsg{ "Not in collision" };
    // Sim state
    std::unordered_map<uint32_t, PlayerData> playerDataMap;
    sf::Texture dummyTex{ "assets/textures/isometric_demo.png" };
    float zHeightOffset = 156.f;
    std::unordered_map<uint32_t, PlayerIO>   playerIOMap;
    //void collidePlayerWithNeighborhood(PlayerData& p, float dt)
    //{

    //    auto it = m_neighborhoods.find(p.id);
    //    if (it == m_neighborhoods.end() || !it->second.valid)
    //    {
    //        // No neighborhood tiles yet -> allow movement (or clamp)
    //        p.xpos += p.xvel * dt;
    //        p.ypos += p.yvel * dt;
    //        return;
    //    }
    //    rect& pRect = it->second.r;




    //    static const float SPEED = 500.f;
    //    float sx = 0.f, sy = 0.f;
    //    if (this->playerIOMap[p.id].u) sy -= 1.f;   // Up
    //    if (this->playerIOMap[p.id].d) sy += 1.f;   // Down
    //    if (this->playerIOMap[p.id].l) sx -= 1.f;   // Left
    //    if (this->playerIOMap[p.id].r) sx += 1.f;   // Right



    //    float vx = 0.5f * sx + 0.5f * sy;
    //    float vy = -0.5f * sx + 0.5f * sy;
    //    const float len = std::sqrt(vx * vx + vy * vy);
    //    if (len > 0.0001f) { vx /= len; vy /= len; }


    //    if (!playerIOMap[p.id].space)
    //    {

    //        if (playerDataMap[p.id].animID != AnimID::Attack)
    //        {
    //            p.xvel = vx * SPEED;
    //            p.yvel = vy * SPEED;
    //        }
    //    }

    //    if (std::abs(sx) > 0.001f || std::abs(sy) > 0.001f)
    //    {
    //        if (sy > 0.5f && sx > 0.5f)      p.dir = Dir::DR; // Down+Right
    //        else if (sy < -0.5f && sx >  0.5f) p.dir = Dir::UR; // Up+Right
    //        else if (sy < -0.5f && sx < -0.5f) p.dir = Dir::UL; // Up+Left
    //        else if (sy > 0.5f && sx < -0.5f) p.dir = Dir::DL; // Down+Left
    //        else if (sx > 0.5f)               p.dir = Dir::R;  // Right
    //        else if (sx < -0.5f)               p.dir = Dir::L;  // Left
    //        else if (sy < -0.5f)               p.dir = Dir::U;  // Up
    //        else                                p.dir = Dir::D;  // Down
    //        if (playerDataMap[p.id].animID != AnimID::Run && playerDataMap[p.id].animID != AnimID::Attack) 
    //        {
    //            playerDataMap[p.id].animID = AnimID::Run; playerDataMap[p.id].frameIndex = 0;
    //        }
    //    }
    //    else
    //    {
    //        if (playerDataMap[p.id].animID != AnimID::Idle && playerDataMap[p.id].animID != AnimID::Attack) 
    //        {
    //            playerDataMap[p.id].animID = AnimID::Idle; playerDataMap[p.id].frameIndex = 0; 
    //        }
    //    }

    //   
    //   
    //    const LatestNeighborhood& N = it->second;
    //    p.xpos = N.r.left - 124.f;
    //    p.ypos = N.r.top - 200.f;


    //    // Proposed movement
    //    float nextX = p.xpos + p.xvel * dt;
    //    float nextY = p.ypos + p.yvel * dt;

    //    // Player feet box (tune these to your sprite)
    //    const float feetW = 150.f;
    //    const float feetH = 30.f;
    //    float pf_x0 = nextX - feetW * 0.5f;
    //    float pf_y0 = nextY - feetH;      // feet rectangle just above the feet point
    //    float pf_x1 = nextX + feetW * 0.5f;
    //    float pf_y1 = nextY;

    //    // Iterate 3×3 tiles and block if colliding with BlockFeet band
    //    const int T = 128;                 // tile size (server constant)
    //    int i = 0;
    //    bool collided = false;

    //    for (int dy = -1; dy <= 1; ++dy) {
    //        for (int dx = -1; dx <= 1; ++dx, ++i) {
    //            const uint32_t tilesetIndex = N.tiles[i];
    //            if (tilesetIndex >= currTSetDetails.size()) continue;

    //            TileCol type = (TileCol)currTSetDetails[tilesetIndex];
    //            if (type == TileCol::Walk) continue;

    //            
    //            auto mapIso = [&](const sf::Vector2f& pos) {
    //                float xIso = ((pos.x / (float)T) - (pos.y / (float)T)) * 0.5f * T;
    //                float yIso = ((pos.x / (float)T) + (pos.y / (float)T)) * 0.5f * (T / ((T == T) ? 2.f : 1.f));
    //                return sf::Vector2f{ xIso, yIso };
    //                };
    //           // const int tx = N.cx + dx;
    //           // const int ty = N.cy + dy;
    //            sf::Vector2f offset{ 124.f,200.f };
    //            auto playerIso = sf::Vector2f{ (N.r.left + N.r.width / 2.f) + p.xvel * dt, (N.r.top + N.r.height / 2.f) + p.yvel * dt };                
    //            //auto invertIso = util::toCart( playerIso.x, playerIso.y, (float)T);
    //            //invertIso *= (float)T;
    //            const int tx = ((int)playerIso.x / T) + dx;
    //            const int ty = ((int)playerIso.y / T) + dy;

    //            const float tileX0 = tx * float(T);
    //            const float tileY0 = ty * float(T);
    //            const float tileX1 = tileX0 + float(T);
    //            const float tileY1 = tileY0 + float(T);

    //            if (!N.valid) { continue; }
    //            if (tileX1 < 0.f || tileY1 < 0.f) { continue; }
    //        
    //            if (type == TileCol::BlockFeet) {

    //                

    //                sf::Vector2f tilePos_iso{ mapIso({tileX0, tileY0}) };
    //                
    //                sf::Sprite playerSpr(dummyTex);
    //                
    //                playerSpr.setPosition({N.r.left + p.xvel * dt, N.r.top + p.yvel * dt});
    //                std::unique_ptr<Collider> playerCollider = std::make_unique<BoxCollider>(playerSpr, 50.f, 30.f);

    //                sf::Sprite tileSpr(dummyTex);
    //                tileSpr.setPosition({ tileX0, tileY0 });
    //                std::unique_ptr<Collider> tileCollider = std::make_unique<IsoTileCollider>(tileSpr);

    //                if (Physics::DetectAndResolve(*playerCollider, *tileCollider))
    //                {
    //                    nextX = playerCollider->getSprite().getPosition().x - offset.x;
    //                    nextY = playerCollider->getSprite().getPosition().y - offset.y;

    //                }



    //                //switch (i % 3)
    //                //{
    //                //case (1):  // tile to left of tx in screenspace
    //                //{
    //                //    tilePos_world.x = tileX0 - float(T);
    //                //    
    //                //}
    //                //    break;
    //                //case (2):
    //                //{

    //                //    tilePos_world.x = tileX0;
    //                //    
    //                //}
    //                //break;
    //                //case (0): // tile to left of tx in screenspace
    //                //{
    //                //    tilePos_world.x = tileX0 + float(T);
    //                //}
    //                //break;
    //                //default:
    //                //{}
    //                //break;
    //                //}
    //                //if (i < 3) // top row
    //                //{

    //                //    tilePos_world.y = tileY0 - float(T);
    //                //}
    //                //else if (i > 5) // bottom row
    //                //{
    //                //    tilePos_world.y = tileY0 + float(T);
    //                //}
    //                //else if (i == 3 || i == 5)
    //                //{
    //                //    tilePos_world.y = tileY0;
    //                //}

    //                // Only collide with the bottom band (e.g., bottom 20%)
    //                //const float band = 0.20f;
    //                //const float by0 = tileY1 - band * float(T);
    //                //const float by1 = tileY1;
    //                ////if (N.r.left < 0 || N.r.top < 0) { continue; } // not a tile there at edge of map
    //                //if (aabbOverlap((float)N.r.left, (float)N.r.top, ((float)N.r.left) + (float)N.r.width, ((float)N.r.top) + (float)N.r.height,(tileX0 ), tileY0, tileX1, tileY1))
    //                //{
    //                //    collided = true;
    //                //    colX0 = (int)tileX0;
    //                //    colY0 = (int)tileY0;
    //                //    colX1 = (int)(tileX1 - tileX0);
    //                //    colY1 = (int)(tileY1 - tileY0);

    //                //    //aabbOverlap(pf_x0, pf_y0, pf_x1, pf_y1, tileX0, by0, tileX1, by1)) {
    //                //    // push player up to top of band
    //                //    //nextY = by0;  // feet touch band top
    //                //    // zero Y vel if moving into it
    //                //    //if (p.yvel > 0.f) p.yvel = 0.f;
    //                //    // recompute feet box after correction
    //                //    //pf_y0 = nextY - feetH;
    //                //   // pf_y1 = nextY;
    //                //}
    //               
    //            }
    //            else if (type == TileCol::RampR || type == TileCol::RampL) {
    //                // TODO: handle ramps (compute tile-local height at x; clamp feetY)
    //                // Keep as Walk for now or implement your ramp curve.
    //            }
    //            else {
    //                // If you later add full blocking, handle here
    //            }
    //        }
    //    }

    //    p.xpos = nextX;
    //    p.ypos = nextY;
    //}
   
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
    public:
    // Debug
    uint32_t ioU{}, ioD{}, ioL{}, ioR{}, ioSpace{};

    // Text
    std::map<uint32_t, std::vector<TEXT>> m_messagesToDisplay;
    std::unordered_map<TEXT, std::string> m_msgStrLUT = {
        {TEXT::POOPSIE, "Poopsie Daisy"},
    };

    std::unordered_map<uint32_t, sPlayerDescription> m_mapPlayerRoster;
    std::vector<uint32_t> m_vGarbageIDs;

public:
    GameServer(uint16_t nPort) : cnet::server_interface<Msg>(nPort) {
        playerDataMap.clear();
        playerIOMap.clear();
    }

protected:
    bool OnClientConnect(std::shared_ptr<cnet::connection<Msg>> client) override {
        return true;
    }

    void OnClientValidated(std::shared_ptr<cnet::connection<Msg>> client) override {
        cnet::message<Msg> msg;
        msg.header.id = Msg::Client_Accepted;
        client->Send(msg);
    }

    void OnClientDisconnect(std::shared_ptr<cnet::connection<Msg>> client) override {
        if (!client) return;

        auto it = m_mapPlayerRoster.find(client->GetID());
        if (it != m_mapPlayerRoster.end()) {
            const auto pid = it->second.nUniqueID;
            //std::cout << "[UNGRACEFUL REMOVAL]:" << pid << "\n";
            m_mapPlayerRoster.erase(it);
            m_vGarbageIDs.push_back(pid);

            playerDataMap.erase(pid);
            playerIOMap.erase(pid);
        }
    }

    void handleGarbageRemovals() {
        if (m_vGarbageIDs.empty()) return;
        for (auto pid : m_vGarbageIDs) {
            cnet::message<Msg> m;
            m.header.id = Msg::Game_RemovePlayer;
            m << pid;
            //std::cout << "Removing " << pid << "\n";
            MessageAllClients(m);
        }
        m_vGarbageIDs.clear();
    }

    void OnMessage(std::shared_ptr<cnet::connection<Msg>> client, cnet::message<Msg>& msg) override
    {
        handleGarbageRemovals();

        switch (msg.header.id)
        {
        case Msg::Client_RegisterWithServer:
        {


            sPlayerDescription desc;
            msg >> desc;
            desc.nUniqueID = client->GetID();
            m_mapPlayerRoster.insert_or_assign(desc.nUniqueID, desc);

            // Create sim state
            static int spawnIndex = 0;          // <— NEW
            const float baseX = 200.f;          // <— NEW
            const float baseY = 200.f;          // <— NEW
            const float gap = 96.f;           // <— NEW

            // Create sim state
            PlayerData p{};
            p.id = desc.nUniqueID;
            p.avatarID = 0;
            p.animID = AnimID::Idle;
            p.dir = Dir::D;
            p.frameIndex = 0;
            p.xpos = baseX + (spawnIndex % 3) * gap;   // <— NEW
            p.ypos = baseY + (spawnIndex / 3) * gap;   // <— NEW
            ++spawnIndex;
            p.xvel = 0.f;   p.yvel = 0.f;
            playerDataMap[p.id] = p;
            playerIOMap[p.id] = PlayerIO{};

            // Send their ID
            {
                cnet::message<Msg> msgSendID;
                msgSendID.header.id = Msg::Client_AssignID;
                msgSendID << desc.nUniqueID;
                MessageClient(client, msgSendID);
            }

            // Tell everyone a player exists
            {
                cnet::message<Msg> msgAddPlayer;
                msgAddPlayer.header.id = Msg::Game_AddPlayer;
                msgAddPlayer << desc;
                MessageAllClients(msgAddPlayer);
            }

            // Tell the new guy about existing players
            for (const auto& player : m_mapPlayerRoster)
            {
                cnet::message<Msg> msgAddOtherPlayers;
                msgAddOtherPlayers.header.id = Msg::Game_AddPlayer;
                msgAddOtherPlayers << player.second;
                MessageClient(client, msgAddOtherPlayers);
            }
            break;
        }

        case Msg::Client_UnregisterWithServer:
            // Optional: handle graceful leave
            break;

        case Msg::Client_SendText:
        {
            sMessageFromClient message;
            msg >> message;
            auto& vec = m_messagesToDisplay[message.nUniqueID];
            vec.push_back(message.message);
            break;
        }

        case Msg::Client_IO:
        {
            PlayerIO io{};
            msg >> io;

            ioU = io.u; ioD = io.d; ioL = io.l; ioR = io.r; ioSpace = io.space;
             
            auto& pio = playerIOMap[client->GetID()];
            pio.d = io.d; // overwrite this frame's input
            pio.u = io.u;
            pio.l = io.l;
            pio.r = io.r;
            pio.space = io.space;
            break;
        }
        case Msg::Client_NeighborhoodTiles:
        {

            NeighborhoodTiles nt{};
            msg >> nt;

            if (!m_mapPlayerRoster.count(nt.playerId)) {
                // ignore / log
                break;
            }

            auto [it, inserted] = m_neighborhoods.try_emplace(nt.playerId, LatestNeighborhood{});
            auto& slot = it->second;

            slot.cx = nt.centerTx;
            slot.cy = nt.centerTy;
            slot.tiles = nt.tilesetIdx;
            slot.valid = true;
            slot.r.left = nt.left;
            slot.r.top = nt.top;
            slot.r.width = nt.right - nt.left;
            slot.r.height = nt.bottom - nt.top;


            hasTilesToCheck = true;
            break;
        }

        default:
            break;
        }
    }

public:
   

    // One simulation tick
    void Simulate(float dt)
    {
        

        for (auto& kv : playerDataMap)
        {
            handleInput(playerIOMap[kv.first], kv.second);

            // collision detection here

            kv.second.xpos += kv.second.xvel * dt;
            kv.second.ypos += kv.second.yvel * dt;


            animate(playerIOMap[kv.first], kv.second, dt);


            //static const float frameAnimDelay = 0.1f;
            //static const float attackAnimDelay = 0.02f;
            //static const float runAnimDelay = 0.03f;

            //static int currAnim{ 0 };

            //currAnim = (int)(uint32_t)kv.second.animID;

            //static float elapsed{ 0 };

            //elapsed += dt;

            //static float currFrameDelay{};



            //switch (currAnim)
            //{
            //case 0: // Idle
            //{
            //    if (currFrameDelay != frameAnimDelay)
            //    {
            //        elapsed = 0;
            //    }
            //    currFrameDelay = frameAnimDelay;
            //}
            //break;
            //case 1: // Run
            //{
            //    if (currFrameDelay != runAnimDelay)
            //    {
            //        elapsed = 0;
            //    }
            //    currFrameDelay = runAnimDelay;
            //}
            //break;
            //case 2: // Attack
            //{
            //    if (currFrameDelay != attackAnimDelay)
            //    {
            //        elapsed = 0;
            //    }
            //    currFrameDelay = attackAnimDelay;
            //}
            //break;
            //default: // Idle
            //{
            //    currFrameDelay = 100000.f;
            //}
            //break;
            //}


            //while (elapsed >= currFrameDelay)
            //{

            //    ++kv.second.frameIndex;

            //    if (std::abs(sx) > 0.001f || std::abs(sy) > 0.001f)
            //    {
            //        if (kv.second.animID != AnimID::Run && kv.second.animID != AnimID::Attack)
            //        {
            //            kv.second.animID = AnimID::Run; kv.second.frameIndex = 0;
            //        }
            //    }
            //    else
            //    {
            //        if (kv.second.animID != AnimID::Idle && kv.second.animID != AnimID::Attack)
            //        {
            //            kv.second.animID = AnimID::Idle; kv.second.frameIndex = 0;
            //        }
            //    }

            //    if (playerIOMap[kv.first].space)
            //    {
            //        kv.second.xvel = 0.f;
            //        kv.second.yvel = 0.f;
            //        if (kv.second.animID != AnimID::Attack)
            //        {

            //            kv.second.animID = AnimID::Attack;
            //            kv.second.frameIndex = 0;
            //        }
            //    }
            //    else
            //    {
            //        if (kv.second.animID == AnimID::Attack)
            //        {
            //            if (kv.second.frameIndex >= 17)
            //            {
            //                kv.second.frameIndex = 0;
            //                if (playerIOMap[kv.first].d || playerIOMap[kv.first].r || playerIOMap[kv.first].l || playerIOMap[kv.first].u)
            //                {
            //                    kv.second.animID = AnimID::Run;
            //                }
            //                else
            //                {
            //                    kv.second.animID = AnimID::Idle;
            //                }
            //            }
            //        }
            //    }

            //    if (kv.second.animID == AnimID::Idle)
            //    {
            //        if (kv.second.frameIndex >= 16)
            //        {
            //            kv.second.frameIndex = 0;
            //        }
            //    }
            //    else if (kv.second.animID == AnimID::Run)
            //    {
            //        if (kv.second.frameIndex >= 17)
            //        {
            //            kv.second.frameIndex = 0;
            //        }
            //    }
            //    else if (kv.second.animID == AnimID::Attack)
            //    {
            //        if (kv.second.frameIndex >= 17)
            //        {
            //            kv.second.frameIndex = 0;
            //        }
            //    }

            //    elapsed -= currFrameDelay;
            //}
            //



            // with:
            //collidePlayerWithNeighborhood(kv.second, dt);
 
            

           /* if (++kv.second.frameIndex > playerAnim)
            kv.second.frameIndex = (kv.second.frameIndex + 1u % 32u;*/
        }
    }



    void BroadcastSnapshot()
    {
        cnet::message<Msg> out;
        out.header.id = Msg::Server_PlayerDrawSnapshot;

        // 1) Collect first (optional, just for clarity)
        std::vector<PlayerDrawData> vec;
        vec.reserve(playerDataMap.size());
        for (const auto& kv : playerDataMap)
        {
            const auto& p = kv.second;
            PlayerDrawData d{};
            d.id = kv.first;
            d.xpos = p.xpos; d.ypos = p.ypos;

            // clamp to valid ranges to be safe
            auto clampAnim = [](AnimID a) {
                uint32_t v = static_cast<uint32_t>(a);
                return a;
                //(v <= static_cast<uint32_t>(AnimID::Attack)) ? a : (v <= (uint32_t)(AnimID::Run) ? AnimID::Attack : AnimID::Idle);
                };
            auto clampDir = [](Dir d) {
                uint32_t v = static_cast<uint32_t>(d);
                return (v < 8u) ? d : Dir::D;
                };

            d.animID = clampAnim(p.animID);
            d.dir = clampDir(p.dir);
            d.frameIndex = p.frameIndex;

            vec.push_back(d);

        }

            // 2) PUSH PLAYERS FIRST ...
            for (const auto& d : vec)
            {
                out << d;
            }

            // 3) ... and PUSH COUNT LAST so client can POP it first
            uint32_t count = static_cast<uint32_t>(vec.size());
            out << count;

            MessageAllClients(out);
        
    }
};

int main(int argc, char* argv[])
{
    GameServer server(60000);
    server.Start();

    sf::RenderWindow window(sf::VideoMode({ 600, 400 }), "GameServer!");
    if (!window.isOpen()) return 420;
    window.setPosition(sf::Vector2i(1200, 200));

    // Hide console window
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    

    // Fixed timestep
    const float dt = 1.0f / 60.0f;
    float accumulator = 0.f;
    sf::Clock clock;

    while (window.isOpen())
    {
        // handle network and window messages
        server.Update(10);
        handleWindowEvents(window);

        // update game state for all clients and send updated state to all of them for each client' state
        server.updateState();
        server.BroadcastSnapshot();

        // Render the server window, able to debug remotely
        server.renderServerDisplay(window);
    }
    return 0;
}

sf::Vector2f GameServer::getPlayerColliderPos(sf::Sprite& player_)
{
    sf::Vector2f pos{};

    std::unique_ptr<Collider> col = std::make_unique<BoxCollider>(player_, 50.f, 30.f);

    pos = col->getCenter();

    return pos;
}

sf::Vector2f GameServer::getTileColliderPos(sf::Sprite& spr_)
{
    sf::Vector2f pos{};

    return pos;
}

bool GameServer::updateState()
{
    static float accumulator = 0;
    static sf::Clock clock = {};
    static float fps60 = 1.f / 60.f;
    float dt_ = clock.restart().asSeconds();
    accumulator += dt_;
    while (accumulator >= fps60)
    {
        this->Simulate(fps60);
        accumulator -= fps60;
    }
    
    return true;
}

void GameServer::renderServerDisplay(sf::RenderWindow& window)
{
    // Debug draw
    window.clear();
    static sf::Font crustyFont{ "assets/font/font2.ttf" };
    // Show text messages (your original layout kept)
    std::vector<int> lastSize;
    static int totalSizeOffset = 0;
    static float bottomY = 0.f;
    if (!this->m_messagesToDisplay.empty())
    {
        int i = 0;
        lastSize.push_back(0);
        for (auto& e : this->m_messagesToDisplay)
        {
            for (int sz = 0; sz < (int)e.second.size(); sz++)
            {
                totalSizeOffset = 0;
                for (auto& l : lastSize) totalSizeOffset += (int)l * 32;

                sf::Text text{ crustyFont };
                text.setCharacterSize(32U);
                bottomY = 10.f + (sz * 32.f) + totalSizeOffset + 8.f;
                text.setPosition({ 30.f, bottomY });
                text.setFillColor(sf::Color::Red);
                text.setOutlineColor(sf::Color::White);
                text.setOutlineThickness(2.f);
                text.setString(this->m_msgStrLUT[e.second.at(sz)]);
                window.draw(text);
            }
            lastSize.push_back((int)e.second.size());
            i++;
        }
    }
    {
        sf::Text text{ crustyFont };
        text.setCharacterSize(32U);
        bottomY = 10.f + (this->m_messagesToDisplay.size() * 32.f) + float(bottomY) + 8.f;
        text.setPosition({ 30.f,bottomY });
        text.setFillColor(sf::Color::Red);
        text.setOutlineColor(sf::Color::White);
        text.setOutlineThickness(2.f);
        std::string str = std::to_string(this->ioD) + " " + std::to_string(this->ioL) + " " +
            std::to_string(this->ioU) + " " + std::to_string(this->ioR) + " " + std::to_string(this->ioSpace);
        text.setString(this->myMsg);
        window.draw(text);
    }

    // draw each roster entry without inserting new ones
    float y = bottomY;
    for (const auto& [id, desc] : this->m_mapPlayerRoster) {
        sf::Text text{ crustyFont };
        text.setCharacterSize(32U);
        y += 40.f;
        text.setPosition({ 30.f, y });
        text.setFillColor(sf::Color::Red);
        text.setOutlineColor(sf::Color::White);
        text.setOutlineThickness(2.f);
        text.setString("PlayerID: " + std::to_string(desc.nUniqueID));
        window.draw(text);
    }

    window.display();
}

void GameServer::handleInput(PlayerIO& pIO, PlayerData& pData)
{

    if (pData.animID != AnimID::Attack)
    {
        constexpr float SPEED = 500.f;

        //auto& p = kv.second;
        auto& io = pIO;

        //// --- Screen-space intent (what the player expects) ---
        //// W?, S?, A?, D? in screen space
        float sx = 0.f, sy = 0.f;
        if (io.u) sy -= 1.f;   // Up
        if (io.d) sy += 1.f;   // Down
        if (io.l) sx -= 1.f;   // Left
        if (io.r) sx += 1.f;   // Right

        //// --- Convert screen intent to isometric world axes ---
        //// Screen X ? (+0.5, -0.5), Screen Y ? (+0.5, +0.5)
        float vx = 0.5f * sx + 0.5f * sy;
        float vy = -0.5f * sx + 0.5f * sy;



        //// Normalize
        const float len = std::sqrt(vx * vx + vy * vy);
        if (len > 0.0001f) { vx /= len; vy /= len; }

        pData.xvel = vx * SPEED;
        pData.yvel = vy * SPEED;
    }
    else
    {
        pData.xvel = 0.f;
        pData.yvel = 0.f;
    }
}

void GameServer::animate(PlayerIO& pIO, PlayerData& pData, float dt)
{

   

    float sx = 0.f, sy = 0.f;
    if (pIO.u) sy -= 1.f;   // Up
    if (pIO.d) sy += 1.f;   // Down
    if (pIO.l) sx -= 1.f;   // Left
    if (pIO.r) sx += 1.f;   // Right

    if (std::abs(sx) > 0.001f || std::abs(sy) > 0.001f)
    {
        if (sy > 0.5f && sx > 0.5f)     pData.dir = Dir::DR; // Down+Right
        else if (sy < -0.5f && sx >  0.5f) pData.dir = Dir::UR; // Up+Right
        else if (sy < -0.5f && sx < -0.5f) pData.dir = Dir::UL; // Up+Left
        else if (sy > 0.5f && sx < -0.5f) pData.dir = Dir::DL; // Down+Left
        else if (sx > 0.5f)               pData.dir = Dir::R;  // Right
        else if (sx < -0.5f)               pData.dir = Dir::L;  // Left
        else if (sy < -0.5f)               pData.dir = Dir::U;  // Up
        else                                pData.dir = Dir::D;  // Down
    }

    static const float frameAnimDelay = 0.1f;
    static const float attackAnimDelay = 0.02f;
    static const float runAnimDelay = 0.03f;

    static int currAnim{ 0 };

    currAnim = (int)(uint32_t)pData.animID;

    static float elapsed{ 0 };

    elapsed += dt;

    static float currFrameDelay{};



    switch (currAnim)
    {
    case 0: // Idle
    {
        if (currFrameDelay != frameAnimDelay)
        {
            elapsed = 0;
        }
        currFrameDelay = frameAnimDelay;
    }
    break;
    case 1: // Run
    {
        if (currFrameDelay != runAnimDelay)
        {
            elapsed = 0;
        }
        currFrameDelay = runAnimDelay;
    }
    break;
    case 2: // Attack
    {
        if (currFrameDelay != attackAnimDelay)
        {
            elapsed = 0;
        }
        currFrameDelay = attackAnimDelay;
    }
    break;
    default: // Idle
    {
        currFrameDelay = 100000.f;
    }
    break;
    }


    while (elapsed >= currFrameDelay)
    {

        ++pData.frameIndex;

        if (std::abs(sx) > 0.001f || std::abs(sy) > 0.001f)
        {
            if (pData.animID != AnimID::Run && pData.animID != AnimID::Attack)
            {
                pData.animID = AnimID::Run; pData.frameIndex = 0;
            }
        }
        else
        {
            if (pData.animID != AnimID::Idle && pData.animID != AnimID::Attack)
            {
                pData.animID = AnimID::Idle; pData.frameIndex = 0;
            }
        }

        if (pIO.space)
        {
            pData.xvel = 0.f;
            pData.yvel = 0.f;
            if (pData.animID != AnimID::Attack)
            {

                pData.animID = AnimID::Attack;
                pData.frameIndex = 0;
            }
        }
        else
        {
            if (pData.animID == AnimID::Attack)
            {
                if (pData.frameIndex >= 17)
                {
                    pData.frameIndex = 0;
                    if (pIO.d || pIO.r || pIO.l || pIO.u)
                    {
                        pData.animID = AnimID::Run;
                    }
                    else
                    {
                        pData.animID = AnimID::Idle;
                    }
                }
            }
        }

        if (pData.animID == AnimID::Idle)
        {
            if (pData.frameIndex >= 16)
            {
                pData.frameIndex = 0;
            }
        }
        else if (pData.animID == AnimID::Run)
        {
            if (pData.frameIndex >= 17)
            {
                pData.frameIndex = 0;
            }
        }
        else if (pData.animID == AnimID::Attack)
        {
            if (pData.frameIndex >= 17)
            {
                pData.frameIndex = 0;
            }
        }

        elapsed -= currFrameDelay;
    }

}


void handleWindowEvents(sf::RenderWindow& window)
{
    // Window events
    while (const std::optional ev = window.pollEvent())
    {
        if (ev->is<sf::Event::Closed>()) window.close();
        if (auto keypressed = ev->getIf<sf::Event::KeyReleased>())
        {
            if (keypressed->code == sf::Keyboard::Key::Escape)
            {
                window.close();
            }
        }
    }
}