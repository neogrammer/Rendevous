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

#define LOG(stage, fmt, ...) do { \
    auto ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>( \
        std::chrono::steady_clock::now().time_since_epoch()).count(); \
    fprintf(stderr, "[%lld] %-14s " fmt "\n", ms, stage, ##__VA_ARGS__); \
} while(0)

constexpr int playerWidth = 299;
constexpr int playerHeight = 240;

struct TileLayer {
    int width, height;        // in tiles
    int tileSize;             // pixels
    const uint32_t* data;     // map[t] = tileset index at that map cell
};

inline NeighborhoodTiles makeNeighborhoodPacket(
    uint32_t playerId, uint32_t seq,
    const TileLayer & map, int playerFeetX, int playerFeetY, uint32_t** mapDD)
{
    const int tx = ((playerFeetX) + 124 + 25) / 128;
    const int ty = ((playerFeetY) + 200 + 15) / 128;

    NeighborhoodTiles pkt{};
    pkt.playerId = playerId;
    pkt.seq = seq;
    pkt.centerTx = tx;
    pkt.centerTy = ty;
    pkt.left = playerFeetX + 124i32;
    pkt.top = playerFeetY + 200i32;
    pkt.right = pkt.left + 50i32;
    pkt.bottom = pkt.top + 30i32;
   

    int i = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int qx = tx + dx;
            const int qy = ty + dy;
            uint32_t tilesetIndex = 0;
            if (qx >= 0 && qy >= 0 && qx < map.width && qy < map.height) {
                tilesetIndex = (*mapDD)[qy * map.width + qx]; // tileset index used by that map cell
            }
            pkt.tilesetIdx[i++] = tilesetIndex;
        }
    }
    return pkt;
}

class Client : public cnet::client_interface<Msg>
{
    bool moving{ false };
public:
    Client() { wnd = new sf::RenderWindow{}; }
    float getPlayerZHeight(uint32_t playerID)
    {
        sf::Vector2f ppos = { drawObjects[playerID].xpos,drawObjects[playerID].ypos };
        sf::Vector2f ipos = { (ppos.x - ppos.y) * ((float)TW / 2.f) ,  (ppos.x + ppos.y) * ((float)TH/4.f)};



        return ipos.y + playerZHeightOffset;
    }

private:

    

    // Tiles
    sf::Texture tilesetTex{ "assets/textures/blocksTileSheet128x128.png" };
    const unsigned TW = 128;
    const unsigned TH = 128;
    std::vector<sf::Sprite> tileset;

    sf::Font font1{ "assets/font/font1.ttf" };
    std::string txtStr = "";

    bool inEditMode = false;
    std::vector<sf::Sprite> tilemap;

    const unsigned SCRW = 1024;
    const unsigned SCRH = 576;
    int currTSetRows = 0;
    int currTSetCols = 0;
    bool startingAttack = false;
    

    // Player animation
    Dir    playerDir = Dir::D;
    AnimID playerAnimID = AnimID::Idle;
    float playerZHeightOffset = 156.f;
    float playerXWidthOffset = 145.f;
    uint32_t playerAnimFrameIdx = 0;

    bool isAttacking = false;

    // NOTE: client position now comes from server; keep a local for camera center
    sf::Vector2f playerPos{ 100.f,200.f };

    //std::unordered_map<AnimID, std::array<std::vector<sf::IntRect>, 8>> playerAnimFrames;

    // After:
    using AnimSheet = std::array<std::vector<sf::IntRect>, 8>;
    std::array<AnimSheet, 3> playerAnimFrames; // [0]=Idle, [1]=Run

    inline static uint32_t animIndex(AnimID a) {
        const auto v = static_cast<uint32_t>(a);
        return (v <= static_cast<uint32_t>(AnimID::Attack)) ? static_cast<int>(v) : 0; // clamp
    }
    inline static uint32_t dirIndex(Dir d) {
        const auto v = static_cast<uint32_t>(d);
        return (v < 8u) ? static_cast<int>(v) : 0; // clamp
    }


    std::array<sf::Texture, 3> playerTexArr = {
        sf::Texture{"assets/textures/idle_sheet.png"},
        sf::Texture{"assets/textures/run_sheet.png"},
        sf::Texture{"assets/textures/attack_sheet.png"}
    };
    sf::Texture& getPlayerTex(AnimID id_) { return playerTexArr[(int)id_]; }
    typedef std::array<std::vector<sf::IntRect>, 8> AnimSheet;

    sf::RenderWindow* wnd;
    sf::ContextSettings settings;

    // Input
    PlayerIO myIO{};

private:
    // Server-known players for rendering
    std::unordered_map<uint32_t, sPlayerDescription> mapObjects; // who exists
    std::unordered_map<uint32_t, PlayerDrawData>     drawObjects; // last snapshot data

    uint32_t nPlayerID = 0;
    sPlayerDescription descPlayer;

public:
    bool bWaitingForConnection = true;
    float elapsed{};
    sf::Clock timer{};

    std::deque<sMessageFromClient> messages;

    void sendMessageMsg(const TEXT t)
    {
        if (!IsConnected()) return;
        sMessageFromClient m{};
        m.nUniqueID = nPlayerID;
        m.message = t;
        messages.push_back(m);

        if (!Incoming().empty()) return;

        cnet::message<Msg> out;
        out.header.id = Msg::Client_SendText;
        out << messages.back().nUniqueID;
        out << messages.back().message;
        Send(out);
        messages.pop_back();
    }

    uint32_t mapData[1800] = { /* (kept as your existing data) */ };
    bool loadMap(uint32_t** data,int numElems, const std::string& filename)
    {
        std::string path{ "assets/areas/" };
        std::string ext{ ".map" };

        std::string fullRelativePath{ "" };
        fullRelativePath.append(path);
        fullRelativePath.append(filename);
        fullRelativePath.append(ext);

        std::ifstream iFile;
        //iFile.exceptions(std::ios::failbit || std::ios::badbit);
        iFile.open(fullRelativePath.c_str());

        if (iFile.is_open())
        {
            std::string fileCheck, areaName, sheetCheck, sheetName, mappingCheck, elevationCheck;
            int mapCols, mapRows, sheetCols, sheetRows, totalElevations;
            iFile >> fileCheck >> areaName >> mapCols >> mapRows >> sheetCheck >> sheetName >> sheetCols >> sheetRows >> mappingCheck >> totalElevations >> elevationCheck;

            assert(fileCheck == "area" && sheetCheck == "sheet" && mappingCheck == "mapping" && elevationCheck == "elevation");
            assert(numElems == (mapCols * mapRows));
            if (sheetName == "StarterSet")
            {
                assert(sheetRows == 10 && sheetCols == 11);
                currTSetCols = sheetCols;
                currTSetRows = sheetRows;
            }

            for (int i = 0; i < totalElevations; i++)
            {
                int elevationNum;
                iFile >> elevationNum;
                for (int y = 0; y < mapRows; y++)
                {
                    for (int x = 0; x < mapCols; x++)
                    {

                        int mapIdx = y * mapCols + x;
                        if (mapIdx >= numElems)
                        {
                            std::cout << "Tried to read beyond end of mapdata array.  Ill let it slide this time" << std::endl;
                            break; 
                        
                        }
                        int tsetNum;
                        iFile >> tsetNum;
                        (*data)[mapIdx] = uint32_t(tsetNum);
                    }
                }
            }

            std::cout << "Loaded map file: " << path << areaName << ext << " successfully" << std::endl;
            iFile.close();

        }
        else
        {
            std::cout << "Error with area file " << filename << std::endl;
            return false;
        }

        return true;
    }
    bool OnUserCreate()
    {

       

        uint32_t* data =  mapData;
       
        if (!loadMap(&data, 1800, "FrenchTickla"))
        {
            std::cout << "Was not able to load the map tiles data into the array properly" << std::endl;
            return false;
        }



        settings.antiAliasingLevel = 8;
        wnd->create(sf::VideoMode{ {SCRW,SCRH},32U }, "Client Window", sf::State::Windowed, settings);
        tilesetTex.setSmooth(false);

        // (Intro kitty scene kept, unchanged for brevity)

        if (!wnd->isOpen()) return false;
        wnd->setPosition(sf::Vector2i(20, 120));

        HWND hWnd = GetConsoleWindow();
        ShowWindow(hWnd, SW_HIDE);

        // Build anim frames
        playerAnimFrames[(uint32_t)AnimID::Idle] = AnimSheet{};
        playerAnimFrames[(uint32_t)AnimID::Run] = AnimSheet{};
        playerAnimFrames[(uint32_t)AnimID::Attack] = AnimSheet{};




        // Idle
        for (int i = 0; i < 8; ++i) {
            playerAnimFrames[0][i].clear();
            playerAnimFrames[0][i].reserve(16);
            for (int j = 0; j < 16; ++j)
                playerAnimFrames[0][i].emplace_back(sf::IntRect{ {j * playerWidth, i * playerHeight},{playerWidth,playerHeight} });
        }
        // Run
        for (int i = 0; i < 8; ++i) {
            playerAnimFrames[1][i].clear();
            playerAnimFrames[1][i].reserve(17);
            for (int j = 0; j < 17; ++j)
                playerAnimFrames[1][i].emplace_back(sf::IntRect{ {j * playerWidth, i * playerHeight},{playerWidth,playerHeight} });
        }

        // Attack
        for (int i = 0; i < 8; ++i) {
            playerAnimFrames[2][i].clear();
            playerAnimFrames[2][i].reserve(17);
            for (int j = 0; j < 17; ++j)
                playerAnimFrames[2][i].emplace_back(sf::IntRect{ {j * playerWidth, i * playerHeight},{playerWidth,playerHeight} });
        }
        
        // Connect
        while (!Connect("127.0.0.1", 60000)) {}
        //24.236.104.52
        //
        //phone: 10.143.2.210
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
        //192.168.0.5
        return true;
    }

    void run()
    {
        // Build tileset/tilemap (kept like your code)
        const int numCols = currTSetCols, numRows = currTSetRows, numTiles = numCols * numRows;
        tileset.reserve(numTiles);
        for (int i = 0; i < numTiles; i++) {
            int numx = i % numCols;
            int numy = i / numCols;
            auto& t = tileset.emplace_back(tilesetTex);
            t.setPosition({ 0.f,0.f });
            t.setTextureRect({ {numx * (int)TW, numy * (int)TH}, {(int)TW,(int)TH} });
        }

        const int mapCols = 40, mapRows = 45, total = mapCols * mapRows;
        tilemap.reserve(total);
        for (int i = 0; i < total; i++) {
            int numx = i % mapCols;
            int numy = i / mapCols;
            auto& t = tilemap.emplace_back(tilesetTex);
            t.setPosition({ (float)numx * TW, (float)numy * TH });
            t.setTextureRect({ sf::Vector2i{tileset[mapData[i]].getTextureRect().position},
                               sf::Vector2i{tileset[mapData[i]].getTextureRect().size} });
        }

        while (wnd->isOpen())
        {
            while (const std::optional ev = wnd->pollEvent())
            {
                if (ev->is<sf::Event::Closed>()) wnd->close();
                if (auto keyRel = ev->getIf<sf::Event::KeyReleased>()) {
                    if (keyRel->code == sf::Keyboard::Key::M) inEditMode = !inEditMode;
                }
            }

            // Example text send on key tap
            static bool keydown = false;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I)) keydown = true;
            else if (keydown) { keydown = false; sendMessageMsg(TEXT::POOPSIE); }

            elapsed = timer.restart().asSeconds();

            Update(elapsed);
            Render();
        }
    }

    bool Update(float fElapsedTime)
    {
        // Network receive
        if (IsConnected())
        {
            while (!Incoming().empty())
            {
                auto pkg = Incoming().pop_front();
                auto& msg = pkg.msg;

                switch (msg.header.id)
                {
                case Msg::Client_Accepted:
                {
                    std::cout << "Hell yeah mothafucka!  We connected!\n";
                    cnet::message<Msg> out;
                    out.header.id = Msg::Client_RegisterWithServer;
                    // Fill minimal desc; server will override id
                    descPlayer.vPos = { 3.0f, 3.0f };
                    out << descPlayer;
                    Send(out);
                    break;
                }
                case Msg::Client_AssignID:
                {
                  
                    msg >> nPlayerID;
                   
                    if (mapObjects.count(nPlayerID)) { bWaitingForConnection = false; }
                    break;
                }

                case Msg::Game_AddPlayer:
                {
                   
                    sPlayerDescription desc;
                    msg >> desc;
                  
                    mapObjects.insert_or_assign(desc.nUniqueID, desc);
                    drawObjects.insert_or_assign(desc.nUniqueID, PlayerDrawData{});
                    if (desc.nUniqueID == nPlayerID) { bWaitingForConnection = false; }
                
                    break;
                }


                case Msg::Game_RemovePlayer:
                {
                    uint32_t nRemovalID = 0;
                    msg >> nRemovalID;
                    mapObjects.erase(nRemovalID);
                    drawObjects.erase(nRemovalID);
                    break;
                }
                case Msg::Server_PlayerDrawSnapshot:
                {
                    uint32_t count = 0; msg >> count;
                    if (count == 0u)
                    {
                        break;
                    }
                    for (uint32_t i = 0; i < count; ++i)
                    {
                        PlayerDrawData d{};
                        msg >> d;
                        drawObjects[d.id] = d;
                        if (d.id == nPlayerID) {

                            playerPos = { d.xpos, d.ypos };
                            // clamp indices before using them
                            auto ai = (static_cast<uint32_t>(d.animID) <= static_cast<uint32_t>(AnimID::Attack)) ? static_cast<uint32_t>(d.animID) : 0;
                            auto di = (static_cast<uint32_t>(d.dir) < 8u) ? static_cast<uint32_t>(d.dir) : 0u;

                            const auto& frames = playerAnimFrames[ai][di];
                            if (!frames.empty()) {
                                playerAnimID = (ai == (uint32_t)1) ? AnimID::Run : ((ai == (uint32_t)2) ? AnimID::Attack : AnimID::Idle);
                                playerDir = static_cast<Dir>(di);
                                playerAnimFrameIdx++;
                                playerAnimFrameIdx = d.frameIndex % static_cast<uint32_t>(frames.size());
                            }
                        }
                    }
                }
                break;
                default:
                {}
                break;
                }
            }

            // Flush any queued chat messages
            if (!messages.empty())
            {
                for (auto& m : messages)
                {
                    cnet::message<Msg> out;
                    out.header.id = Msg::Client_SendText;
                    out << m.nUniqueID << m.message;
                    Send(out);
                }
                messages.clear();
            }

            // ===== INPUT ONLY (no local simulation) =====
            if (!inEditMode)
            {
                const bool ready = (nPlayerID != 0) && (mapObjects.count(nPlayerID) != 0);

                if (IsConnected() && ready)
                {
                    PlayerIO myIO{};
                    moving = false;
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                    {
                        myIO.r = true; moving = true;
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                        myIO.l = true; moving = true;
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                        myIO.u = true; moving = true;
                    }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                        myIO.d = true; moving = true;
                    }

                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
                    {
                        myIO.space = true;
                        isAttacking = true;
                        startingAttack = true;
                    }



                    cnet::message<Msg> ioMsg;
                    ioMsg.header.id = Msg::Client_IO;
                    ioMsg << myIO;
                    this->myIO = myIO;
                    Send(ioMsg);

                    if (moving)
                    {
                        
                        TileLayer tLayer = {};
                        tLayer.tileSize = (uint32_t)TW;
                        tLayer.width = 40;
                        tLayer.height = 45;
                        tLayer.data = mapData;

                        auto feet = sf::Vector2i{ (int)playerPos.x, (int)playerPos.y};
                        uint32_t* mapDD = mapData;
                        NeighborhoodTiles nt = makeNeighborhoodPacket(nPlayerID, 0, tLayer,  feet.x, feet.y, &mapDD);
                        cnet::message<Msg> m;
                        m.header.id = Msg::Client_NeighborhoodTiles;
                        m << nt; // implement << / >> for NeighborhoodTiles (trivial pack of POD)
                        Send(m);
                    }
                }
        
            }
            if (isAttacking)
            {
                if (startingAttack)
                {
                    startingAttack = false;
                }
            }
            
        }
        return true;
    }

    void Render()
    {
        auto mapIso = [&](const sf::Vector2f& pos) {
            float xIso = ((pos.x / (float)TW) - (pos.y / (float)TH)) * 0.5f * TW;
            float yIso = ((pos.x / (float)TW) + (pos.y / (float)TH)) * 0.5f * (TH / ((TW == TH) ? 2.f : 1.f));
            return sf::Vector2f{ xIso, yIso };
            };

        wnd->clear(sf::Color::Blue);

        // Camera centers on local player (isometric mapped)
        sf::Vector2f cam = mapIso(playerPos);
        auto vw = wnd->getView();
        vw.setCenter({ cam.x + playerWidth * 0.5f, cam.y + playerHeight * 0.5f });
        wnd->setView(vw);

        // Draw tilemap as isometric sprites (unchanged)
        for (int i = 0; i < (int)tilemap.size(); i++)
        {
            auto p = mapIso(tilemap[i].getPosition());
            sf::Sprite isoSprite{ tilesetTex };
            isoSprite.setPosition(p);
            isoSprite.setTextureRect(tilemap[i].getTextureRect());
            wnd->draw(isoSprite);
        }

        std::vector<std::pair<uint32_t, std::pair<uint32_t, PlayerDrawData>>> sortme;
        sortme.clear();
        sortme.reserve(drawObjects.size());

        for (auto& kv : drawObjects)
        {
            sortme.emplace_back(std::pair{ (int)getPlayerZHeight(kv.first), std::pair{kv.first, kv.second} });
        }

        sort(sortme.begin(), sortme.end(), [&](const std::pair<uint32_t, std::pair<uint32_t, PlayerDrawData>>& a,const std::pair<uint32_t,std::pair<uint32_t, PlayerDrawData>>& b)->bool{
            return (a.first < b.first);
            });


        for (int i =0; i < sortme.size(); i++)
        {
            uint32_t id = sortme[i].second.first;
            uint32_t zHeight = sortme[i].first;
            const auto& d = sortme[i].second.second;

            // Clamp bad values coming off the wire
            const uint32_t ai = animIndex(d.animID);
            const uint32_t di = dirIndex(d.dir);

            const auto& frames = playerAnimFrames[ai][di];
            if (frames.empty()) continue;

            if (d.id == nPlayerID) {
                playerPos = { d.xpos, d.ypos };
                sf::Vector2i pSpot = { (int)playerPos.x, (int)playerPos.y };
                std::cout << "\n" << std::to_string(pSpot.x) << ", " << std::to_string(pSpot.y) << std::endl;
                // Clamp to avoid weird keys
                playerAnimID = (((AnimID)animIndex(d.animID) == (AnimID)(uint32_t)1) ? AnimID::Run : ((d.animID == (AnimID)(uint32_t)2) ? AnimID::Attack : AnimID::Idle));
                playerDir = static_cast<Dir>(dirIndex(d.dir));
                playerAnimFrameIdx = frames.empty() ? 0u : (d.frameIndex % static_cast<uint32_t>(playerAnimFrames[animIndex(playerAnimID)][dirIndex(playerDir)].size()));
            }

            const auto idx = frames.empty() ? 0u : (d.frameIndex % static_cast<uint32_t>(frames.size()));

            sf::Sprite spr{ ((ai == animIndex(AnimID::Idle)) ? playerTexArr[0] : ((ai == animIndex(AnimID::Attack)) ? playerTexArr[2] : playerTexArr[1]))};

            // If your world is “logical” coords, iso-map them; otherwise just use d.xpos/d.ypos
            sf::Vector2f pos{ d.xpos, d.ypos };
            spr.setPosition(mapIso(pos));
            spr.setTextureRect(frames[idx]);

            wnd->draw(spr);
        }

        for (auto& p : drawObjects)
        {
            // Optional nameplate/info for self
            sf::Text txt{ font1 };
            txt.setCharacterSize(24);
            txt.setFillColor(sf::Color::White);
            txt.setOutlineColor(sf::Color::Black);
            txt.setOutlineThickness(1);
            sf::Vector2f camP = sf::Vector2f(mapIso({ p.second.xpos, p.second.ypos }));
            txt.setPosition({ camP.x + 110.f, camP.y - 5.f });
            txt.setString(std::to_string(p.first)); // add if wanted
            wnd->draw(txt);
        }
        wnd->display();
    }
};

int main()
{
    Client c;
    if (c.OnUserCreate()) c.run();

    HWND hWnd = GetConsoleWindow();
    CloseWindow(hWnd);
    return 0;
}