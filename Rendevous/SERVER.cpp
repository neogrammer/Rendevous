#include "./src/core/GameServer.h"
//struct LatestNeighborhood {
//    uint32_t seq = 0;
//    int32_t  cx = 0, cy = 0;
//    rect r{};
//    std::array<uint32_t, 9> tiles{};
//    bool valid = false;
//};
//
//std::unordered_map<uint32_t, LatestNeighborhood> m_neighborhoods;
//// Helpers
//static inline bool aabbOverlap(float ax0, float ay0, float ax1, float ay1,
//    float bx0, float by0, float bx1, float by1)
//{
//    return (ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0);
//}
//
//class GameServer : public cnet::server_interface<Msg>
//{
//   
//public:
//    sf::Vector2f getPlayerColliderPos( sf::Sprite& player_);
//    sf::Vector2f getTileColliderPos(sf::Sprite& spr_);
//
//    bool updateState();
//    void renderServerDisplay(sf::RenderWindow& window);
//    void handleInput(PlayerIO& pIO, PlayerData& pData);
//    void animate(PlayerIO& pIO, PlayerData& pData, float dt);
//    public:
//    bool hasTilesToCheck{ false };
//public:
//    std::string myMsg{ "Not in collision" };
//    // Sim state
//    std::unordered_map<uint32_t, PlayerData> playerDataMap;
//    sf::Texture dummyTex{ "assets/textures/isometric_demo.png" };
//    float zHeightOffset = 156.f;
//    std::unordered_map<uint32_t, PlayerIO>   playerIOMap;
//    std::vector<int> currTSetDetails = {
//        1, 1, 0, 0, 0, 2, 3, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3,
//        0, 1, 0, 0, 0, 2, 3, 0, 0, 0, 0,
//        0, 2, 3, 0, 0, 2, 3, 0, 0, 0, 0,
//        0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0,
//        0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0,
//        0, 0, 2, 3, 0, 2, 3, 0, 2, 3, 0,
//        0, 2, 3, 0, 2, 3, 0, 0, 2, 3, 0,
//        0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0
//
//    };
//    public:
//    // Debug
//    uint32_t ioU{}, ioD{}, ioL{}, ioR{}, ioSpace{};
//    // Text
//    std::map<uint32_t, std::vector<TEXT>> m_messagesToDisplay;
//    std::unordered_map<TEXT, std::string> m_msgStrLUT = {
//        {TEXT::POOPSIE, "Poopsie Daisy"},
//    };
//    std::unordered_map<uint32_t, sPlayerDescription> m_mapPlayerRoster;
//    std::vector<uint32_t> m_vGarbageIDs;
//public:
//    GameServer(uint16_t nPort) : cnet::server_interface<Msg>(nPort) {
//        playerDataMap.clear();
//        playerIOMap.clear();
//    }
//protected:
//    bool OnClientConnect(std::shared_ptr<cnet::connection<Msg>> client) override {
//        return true;
//    }
//    void OnClientValidated(std::shared_ptr<cnet::connection<Msg>> client) override {
//        cnet::message<Msg> msg;
//        msg.header.id = Msg::Client_Accepted;
//        client->Send(msg);
//    }
//    void OnClientDisconnect(std::shared_ptr<cnet::connection<Msg>> client) override {
//        if (!client) return;
//
//        auto it = m_mapPlayerRoster.find(client->GetID());
//        if (it != m_mapPlayerRoster.end()) {
//            const auto pid = it->second.nUniqueID;
//            //std::cout << "[UNGRACEFUL REMOVAL]:" << pid << "\n";
//            m_mapPlayerRoster.erase(it);
//            m_vGarbageIDs.push_back(pid);
//
//            playerDataMap.erase(pid);
//            playerIOMap.erase(pid);
//        }
//    }
//    void handleGarbageRemovals() {
//        if (m_vGarbageIDs.empty()) return;
//        for (auto pid : m_vGarbageIDs) {
//            cnet::message<Msg> m;
//            m.header.id = Msg::Game_RemovePlayer;
//            m << pid;
//            //std::cout << "Removing " << pid << "\n";
//            MessageAllClients(m);
//        }
//        m_vGarbageIDs.clear();
//    }
//    void OnMessage(std::shared_ptr<cnet::connection<Msg>> client, cnet::message<Msg>& msg) override
//    {
//        handleGarbageRemovals();
//
//        switch (msg.header.id)
//        {
//        case Msg::Client_RegisterWithServer:
//        {
//
//
//            sPlayerDescription desc;
//            msg >> desc;
//            desc.nUniqueID = client->GetID();
//            m_mapPlayerRoster.insert_or_assign(desc.nUniqueID, desc);
//
//            // Create sim state
//            static int spawnIndex = 0;          // <— NEW
//            const float baseX = 200.f;          // <— NEW
//            const float baseY = 200.f;          // <— NEW
//            const float gap = 96.f;           // <— NEW
//
//            // Create sim state
//            PlayerData p{};
//            p.id = desc.nUniqueID;
//            p.avatarID = 0;
//            p.animID = AnimID::Idle;
//            p.dir = Dir::D;
//            p.frameIndex = 0;
//            p.xpos = baseX + (spawnIndex % 3) * gap;   // <— NEW
//            p.ypos = baseY + (spawnIndex / 3) * gap;   // <— NEW
//            ++spawnIndex;
//            p.xvel = 0.f;   p.yvel = 0.f;
//            playerDataMap[p.id] = p;
//            playerIOMap[p.id] = PlayerIO{};
//
//            // Send their ID
//            {
//                cnet::message<Msg> msgSendID;
//                msgSendID.header.id = Msg::Client_AssignID;
//                msgSendID << desc.nUniqueID;
//                MessageClient(client, msgSendID);
//            }
//
//            // Tell everyone a player exists
//            {
//                cnet::message<Msg> msgAddPlayer;
//                msgAddPlayer.header.id = Msg::Game_AddPlayer;
//                msgAddPlayer << desc;
//                MessageAllClients(msgAddPlayer);
//            }
//
//            // Tell the new guy about existing players
//            for (const auto& player : m_mapPlayerRoster)
//            {
//                cnet::message<Msg> msgAddOtherPlayers;
//                msgAddOtherPlayers.header.id = Msg::Game_AddPlayer;
//                msgAddOtherPlayers << player.second;
//                MessageClient(client, msgAddOtherPlayers);
//            }
//            break;
//        }
//
//        case Msg::Client_UnregisterWithServer:
//            // Optional: handle graceful leave
//            break;
//
//        case Msg::Client_SendText:
//        {
//            sMessageFromClient message;
//            msg >> message;
//            auto& vec = m_messagesToDisplay[message.nUniqueID];
//            vec.push_back(message.message);
//            break;
//        }
//
//        case Msg::Client_IO:
//        {
//            PlayerIO io{};
//            msg >> io;
//
//            ioU = io.u; ioD = io.d; ioL = io.l; ioR = io.r; ioSpace = io.space;
//             
//            auto& pio = playerIOMap[client->GetID()];
//            pio.d = io.d; // overwrite this frame's input
//            pio.u = io.u;
//            pio.l = io.l;
//            pio.r = io.r;
//            pio.space = io.space;
//            break;
//        }
//        case Msg::Client_NeighborhoodTiles:
//        {
//
//            NeighborhoodTiles nt{};
//            msg >> nt;
//            break;
//        }
//
//        default:
//            break;
//        }
//    }
//public:
//    void Simulate(float dt)
//    {
//        for (auto& kv : playerDataMap)
//        {
//            handleInput(playerIOMap[kv.first], kv.second);
//            kv.second.xpos += kv.second.xvel * dt;
//            kv.second.ypos += kv.second.yvel * dt;
//            animate(playerIOMap[kv.first], kv.second, dt);
//        }
//    }
//    void BroadcastSnapshot()
//    {
//        cnet::message<Msg> out;
//        out.header.id = Msg::Server_PlayerDrawSnapshot;
//
//        // 1) Collect first (optional, just for clarity)
//        std::vector<PlayerDrawData> vec;
//        vec.reserve(playerDataMap.size());
//        for (const auto& kv : playerDataMap)
//        {
//            const auto& p = kv.second;
//            PlayerDrawData d{};
//            d.id = kv.first;
//            d.xpos = p.xpos; d.ypos = p.ypos;
//
//            // clamp to valid ranges to be safe
//            auto clampAnim = [](AnimID a) {
//                uint32_t v = static_cast<uint32_t>(a);
//                return a;
//                //(v <= static_cast<uint32_t>(AnimID::Attack)) ? a : (v <= (uint32_t)(AnimID::Run) ? AnimID::Attack : AnimID::Idle);
//                };
//            auto clampDir = [](Dir d) {
//                uint32_t v = static_cast<uint32_t>(d);
//                return (v < 8u) ? d : Dir::D;
//                };
//
//            d.animID = clampAnim(p.animID);
//            d.dir = clampDir(p.dir);
//            d.frameIndex = p.frameIndex;
//
//            vec.push_back(d);
//
//        }
//
//            // 2) PUSH PLAYERS FIRST ...
//            for (const auto& d : vec)
//            {
//                out << d;
//            }
//
//            // 3) ... and PUSH COUNT LAST so client can POP it first
//            uint32_t count = static_cast<uint32_t>(vec.size());
//            out << count;
//
//            MessageAllClients(out);
//        
//    }
//};


void handleWindowEvents(sf::RenderWindow& window);
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
        //auto keypressed = ev->getIf<sf::Event::KeyPressed>();
        //if (keypressed->code == sf::Keyboard::Key::Escape) 
        //{
        //    window.close(); 
        //} 
   
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
    {
        window.close();
    }
}