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

class GameServer : public cnet::server_interface<Msg>
{
public:
    // Sim state
    std::unordered_map<uint32_t, PlayerData> playerDataMap;
    std::unordered_map<uint32_t, PlayerIO>   playerIOMap;

    // Debug
    uint32_t ioU{}, ioD{}, ioL{}, ioR{};

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
            std::cout << "[UNGRACEFUL REMOVAL]:" << pid << "\n";
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
            std::cout << "Removing " << pid << "\n";
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

            ioU = io.u; ioD = io.d; ioL = io.l; ioR = io.r;

            auto& pio = playerIOMap[client->GetID()];
            pio = io; // overwrite this frame's input
            break;
        }

        default:
            break;
        }
    }

public:
    //// One simulation tick
    //void Simulate(float dt)
    //{
    //    constexpr float SPEED = 500.f;

    //    for (auto& kv : playerDataMap)
    //    {
    //        auto& p = kv.second;
    //        auto& io = playerIOMap[p.id];

    //        // Compute desired velocity from input
    //        float vx = 0.f, vy = 0.f;
    //        if (io.l) vx -= 1.f;
    //        if (io.r) vx += 1.f;
    //        if (io.u) vy -= 1.f;
    //        if (io.d) vy += 1.f;

    //        // Normalize
    //        const float len = std::sqrt(vx * vx + vy * vy);
    //        if (len > 0.0001f) { vx /= len; vy /= len; }

    //        p.xvel = vx * SPEED;
    //        p.yvel = vy * SPEED;

    //        // Direction + animation
    //        if (len > 0.0001f)
    //        {
    //            if (vy > 0.5f && vx > 0.5f) p.dir = Dir::DR;
    //            else if (vy < -0.5f && vx >  0.5f) p.dir = Dir::UR;
    //            else if (vy < -0.5f && vx < -0.5f) p.dir = Dir::UL;
    //            else if (vy > 0.5f && vx < -0.5f) p.dir = Dir::DL;
    //            else if (vx > 0.5f) p.dir = Dir::R;
    //            else if (vx < -0.5f) p.dir = Dir::L;
    //            else if (vy < -0.5f) p.dir = Dir::U;
    //            else                  p.dir = Dir::D;

    //            if (p.animID != AnimID::Run) { p.animID = AnimID::Run; p.frameIndex = 0; }
    //        }
    //        else
    //        {
    //            if (p.animID != AnimID::Idle) { p.animID = AnimID::Idle; p.frameIndex = 0; }
    //        }

    //        // Advance position
    //        p.xpos += p.xvel * dt;
    //        p.ypos += p.yvel * dt;

    //        // Simple frame advance (server chooses; client just displays)
    //        // You can make this timer-based; for now, small cheap step:
    //        p.frameIndex = (p.frameIndex + ((p.animID == AnimID::Run) ? 1u : 1u)) % 32u; // safe cap
    //    }
    //}

    // One simulation tick
    void Simulate(float dt)
    {
        constexpr float SPEED = 500.f;

        for (auto& kv : playerDataMap)
        {
            auto& p = kv.second;
            auto& io = playerIOMap[p.id];

            // --- Screen-space intent (what the player expects) ---
            // W?, S?, A?, D? in screen space
            float sx = 0.f, sy = 0.f;
            if (io.u) sy -= 1.f;   // Up
            if (io.d) sy += 1.f;   // Down
            if (io.l) sx -= 1.f;   // Left
            if (io.r) sx += 1.f;   // Right

            // --- Convert screen intent to isometric world axes ---
            // Screen X ? (+0.5, -0.5), Screen Y ? (+0.5, +0.5)
            float vx = 0.5f * sx + 0.5f * sy;
            float vy = -0.5f * sx + 0.5f * sy;

            // Normalize
            const float len = std::sqrt(vx * vx + vy * vy);
            if (len > 0.0001f) { vx /= len; vy /= len; }

            p.xvel = vx * SPEED;
            p.yvel = vy * SPEED;

            // Direction + animation (use screen intent so facing matches keys)
            if (std::abs(sx) > 0.001f || std::abs(sy) > 0.001f)
            {
                if (sy > 0.5f && sx > 0.5f)      p.dir = Dir::DR; // Down+Right
                else if (sy < -0.5f && sx >  0.5f) p.dir = Dir::UR; // Up+Right
                else if (sy < -0.5f && sx < -0.5f) p.dir = Dir::UL; // Up+Left
                else if (sy > 0.5f && sx < -0.5f) p.dir = Dir::DL; // Down+Left
                else if (sx > 0.5f)               p.dir = Dir::R;  // Right
                else if (sx < -0.5f)               p.dir = Dir::L;  // Left
                else if (sy < -0.5f)               p.dir = Dir::U;  // Up
                else                                p.dir = Dir::D;  // Down

                if (p.animID != AnimID::Run) { p.animID = AnimID::Run; p.frameIndex = 0; }
            }
            else
            {
                if (p.animID != AnimID::Idle) { p.animID = AnimID::Idle; p.frameIndex = 0; }
            }

            // Advance position
            p.xpos += p.xvel * dt;
            p.ypos += p.yvel * dt;

            // Advance frame (same as before)
            p.frameIndex = (p.frameIndex + ((p.animID == AnimID::Run) ? 1u : 1u)) % 32u;
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
            d.id = p.id;
            d.xpos = p.xpos; d.ypos = p.ypos;

            // clamp to valid ranges to be safe
            auto clampAnim = [](AnimID a) {
                uint32_t v = static_cast<uint32_t>(a);
                return (v <= static_cast<uint32_t>(AnimID::Run)) ? a : AnimID::Idle;
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
        for (const auto& d : vec) out << d;

        // 3) ... and PUSH COUNT LAST so client can POP it first
        uint32_t count = static_cast<uint32_t>(vec.size());
        out << count;

        MessageAllClients(out);
    }
};

int main()
{
    GameServer server(60000);
    server.Start();

    sf::RenderWindow window(sf::VideoMode({ 1280, 920 }), "GameServer!");
    if (!window.isOpen()) return 420;
    window.setPosition(sf::Vector2i(1670, 520));

    // Hide console window
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    sf::Font crustyFont{ "assets/font/Crusty.ttf" };

    // Fixed timestep
    const float dt = 1.0f / 60.0f;
    float accumulator = 0.f;
    sf::Clock clock;

    while (window.isOpen())
    {
        // Pump network
        server.Update(-1);

        // Window events
        while (const std::optional ev = window.pollEvent())
        {
            if (ev->is<sf::Event::Closed>()) window.close();
        }

        // Fixed update loop
        accumulator += clock.restart().asSeconds();
        while (accumulator >= dt)
        {
            server.Simulate(dt);
            accumulator -= dt;
        }

        // Send snapshot for this rendered frame
        server.BroadcastSnapshot();

        // Debug draw
        window.clear();

        // Show text messages (your original layout kept)
        std::vector<int> lastSize;
        int totalSizeOffset = 0;

        if (!server.m_messagesToDisplay.empty())
        {
            int i = 0;
            lastSize.push_back(0);
            for (auto& e : server.m_messagesToDisplay)
            {
                for (int sz = 0; sz < (int)e.second.size(); sz++)
                {
                    totalSizeOffset = 0;
                    for (auto& l : lastSize) totalSizeOffset += (int)l * 32;

                    sf::Text text{ crustyFont };
                    text.setCharacterSize(32U);
                    text.setPosition({ 30.f, 10.f + (sz * 32.f) + totalSizeOffset + 8.f });
                    text.setFillColor(sf::Color::Red);
                    text.setOutlineColor(sf::Color::White);
                    text.setOutlineThickness(2.f);
                    text.setString(server.m_msgStrLUT[e.second.at(sz)]);
                    window.draw(text);
                }
                lastSize.push_back((int)e.second.size());
                i++;
            }
        }

        sf::Text text{ crustyFont };
        text.setCharacterSize(32U);
        text.setPosition({ 30.f, 10.f + (server.m_messagesToDisplay.size() * 32.f) + totalSizeOffset + 8.f });
        text.setFillColor(sf::Color::Red);
        text.setOutlineColor(sf::Color::White);
        text.setOutlineThickness(2.f);
        std::string str = std::to_string(server.ioD) + " " + std::to_string(server.ioL) + " " +
            std::to_string(server.ioU) + " " + std::to_string(server.ioR);
        text.setString(str);
        window.draw(text);

        window.display();
    }
}