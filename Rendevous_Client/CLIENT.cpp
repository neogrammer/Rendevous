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

class Client : public cnet::client_interface<Msg>
{
public:
    Client() { wnd = new sf::RenderWindow{}; }

private:
    // Tiles
    sf::Texture tilesetTex{ "assets/textures/blocksTileSheet128x128.png" };
    std::vector<sf::Sprite> tileset;

    sf::Font font1{ "assets/font/font1.ttf" };
    std::string txtStr = "";

    bool inEditMode = false;
    std::vector<sf::Sprite> tilemap;

    const unsigned SCRW = 1600;
    const unsigned SCRH = 900;
    const unsigned TW = 128;
    const unsigned TH = 128;

    // Player animation
    Dir    playerDir = Dir::D;
    AnimID playerAnimID = AnimID::Idle;
    uint32_t playerAnimFrameIdx = 0;
    const int playerWidth = 299;
    const int playerHeight = 240;

    // NOTE: client position now comes from server; keep a local for camera center
    sf::Vector2f playerPos{ 200.f,200.f };

    //std::unordered_map<AnimID, std::array<std::vector<sf::IntRect>, 8>> playerAnimFrames;

    // After:
    using AnimSheet = std::array<std::vector<sf::IntRect>, 8>;
    std::array<AnimSheet, 2> playerAnimFrames; // [0]=Idle, [1]=Run

    inline static int animIndex(AnimID a) {
        const auto v = static_cast<uint32_t>(a);
        return (v <= static_cast<uint32_t>(AnimID::Run)) ? static_cast<int>(v) : 0; // clamp
    }
    inline static int dirIndex(Dir d) {
        const auto v = static_cast<uint32_t>(d);
        return (v < 8u) ? static_cast<int>(v) : 0; // clamp
    }


    std::array<sf::Texture, 2> playerTexArr = {
        sf::Texture{"assets/textures/idle_sheet.png"},
        sf::Texture{"assets/textures/run_sheet.png"}
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

    int mapData[1800] = { /* (kept as your existing data) */ };

    bool OnUserCreate()
    {
        settings.antiAliasingLevel = 8;
        wnd->create(sf::VideoMode{ {SCRW,SCRH},32U }, "Client Window", sf::State::Windowed, settings);
        tilesetTex.setSmooth(false);

        // (Intro kitty scene kept, unchanged for brevity)

        if (!wnd->isOpen()) return false;
        wnd->setPosition(sf::Vector2i(20, 520));

        HWND hWnd = GetConsoleWindow();
        ShowWindow(hWnd, SW_HIDE);

        // Build anim frames
        playerAnimFrames[(uint32_t)AnimID::Idle] = AnimSheet{};
        playerAnimFrames[(uint32_t)AnimID::Run] = AnimSheet{};



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


        // Connect
        while (!Connect("24.236.104.52", 60000)) {}
        //192.168.0.5
        return true;
    }

    void run()
    {
        // Build tileset/tilemap (kept like your code)
        const int numCols = 11, numRows = 10, numTiles = numCols * numRows;
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
                    std::cout << "Server accepted client - you're in!\n";
                    cnet::message<Msg> out;
                    out.header.id = Msg::Client_RegisterWithServer;
                    // Fill minimal desc; server will override id
                    descPlayer.vPos = { 3.0f, 3.0f};
                    out << descPlayer;
                    Send(out);
                    break;
                }
                case Msg::Client_AssignID:
                    msg >> nPlayerID;
                    std::cout << "Assigned Client ID = " << nPlayerID << "\n";
                    break;

                case Msg::Game_AddPlayer:
                {
                    sPlayerDescription desc;
                    msg >> desc;
                    mapObjects.insert_or_assign(desc.nUniqueID, desc);
                    drawObjects.insert_or_assign(desc.nUniqueID, PlayerDrawData{});
                    if (desc.nUniqueID == nPlayerID) bWaitingForConnection = false;

           

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
                    uint32_t count = 0;
                    msg >> count;                  // OK now: count was pushed last

                    for (uint32_t i = 0; i < count; ++i)
                    {
                        PlayerDrawData d{};
                        msg >> d;                  // pops the last PlayerDrawData first (order doesn't matter)
                        drawObjects[d.id] = d;

                        if (d.id == nPlayerID) {
                            playerPos = { d.xpos, d.ypos };
                            // clamp indices before using them
                            auto ai = (static_cast<uint32_t>(d.animID) <= static_cast<uint32_t>(AnimID::Run))
                                ? static_cast<int>(d.animID) : 0;
                            auto di = (static_cast<uint32_t>(d.dir) < 8u)
                                ? static_cast<int>(d.dir) : 0;

                            const auto& frames = playerAnimFrames[ai][di];
                            if (!frames.empty()) {
                                playerAnimID = (ai == 1) ? AnimID::Run : AnimID::Idle;
                                playerDir = static_cast<Dir>(di);
                                playerAnimFrameIdx = d.frameIndex % static_cast<uint32_t>(frames.size());
                            }
                        }
                    }
                    break;
                }

                default: break;
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
                myIO = {};
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) myIO.r = true;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) myIO.l = true;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) myIO.u = true;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) myIO.d = true;

                cnet::message<Msg> ioMsg;
                ioMsg.header.id = Msg::Client_IO;
                ioMsg << myIO;
                Send(ioMsg);
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



        for (auto& kv : drawObjects)
        {



            const auto& d = kv.second;

            
            
            
            // Clamp bad values coming off the wire
            const int ai = animIndex(d.animID);
            const int di = dirIndex(d.dir);

            const auto& frames = playerAnimFrames[ai][di];
            if (frames.empty()) continue;


            if (d.id == nPlayerID) {
                playerPos = { d.xpos, d.ypos };
                // Clamp to avoid weird keys
                playerAnimID = (animIndex(d.animID) == 1) ? AnimID::Run : AnimID::Idle;
                playerDir = static_cast<Dir>(dirIndex(d.dir));
                playerAnimFrameIdx = frames.empty() ? 0u : (d.frameIndex % static_cast<uint32_t>(playerAnimFrames[animIndex(playerAnimID)][dirIndex(playerDir)].size()));
            }


            const auto idx = frames.empty() ? 0u : (d.frameIndex % static_cast<uint32_t>(frames.size()));

            sf::Sprite spr{ (ai == animIndex(AnimID::Idle)) ? playerTexArr[0] : playerTexArr[1] };

            // If your world is “logical” coords, iso-map them; otherwise just use d.xpos/d.ypos
            sf::Vector2f pos{ d.xpos, d.ypos };
            spr.setPosition(mapIso(pos));
            spr.setTextureRect(frames[idx]);

            wnd->draw(spr);
        }

        //// Draw all players from server snapshot
        //for (auto& kv : drawObjects)
        //{
        //    const auto& d = kv.second;
        //    sf::Sprite spr{ (d.animID == AnimID::Idle) ? playerTexArr[0] : playerTexArr[1] };

        //    // Server sends screen/world coords; if you want strict isometric logical->screen,
        //    // convert here. For now, assume world pixels like before:
        //    sf::Vector2f pos{ d.xpos, d.ypos };
        //    spr.setPosition(mapIso(pos));

        //    const auto& frames = playerAnimFrames[d.animID][(int)d.dir];
        //    if (frames.empty()) { continue; }
        //    const auto idx = frames.empty() ? 0u : (d.frameIndex % (uint32_t)frames.size());
        //    spr.setTextureRect(frames[idx]);

        //    wnd->draw(spr);
        //}

        // Optional nameplate/info for self
        sf::Text txt{ font1 };
        txt.setCharacterSize(24);
        txt.setFillColor(sf::Color::White);
        txt.setOutlineColor(sf::Color::Black);
        txt.setOutlineThickness(1);
        txt.setPosition({ cam.x, cam.y - 40.f });
        // txt.setString("You"); // add if wanted
        // wnd->draw(txt);

        wnd->display();
    }
};

int main()
{
    Client c;
    if (c.OnUserCreate()) c.run();
    return 0;
}