#include "GameServer.h"
#include <fstream>
#include "../SolutionHelper.cpp"

GameServer::GameServer(uint16_t nPort) : cnet::server_interface<Msg>(nPort) {
    playerDataMap.clear();
    playerIOMap.clear();
}
    bool GameServer::OnClientConnect(std::shared_ptr<cnet::connection<Msg>> client) {
        return true;
    }
    void GameServer::OnClientValidated(std::shared_ptr<cnet::connection<Msg>> client) {
        cnet::message<Msg> msg;
        msg.header.id = Msg::Client_Accepted;
        client->Send(msg);
    }
    void GameServer::OnClientDisconnect(std::shared_ptr<cnet::connection<Msg>> client) {
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
    void GameServer::handleGarbageRemovals() {
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
    void GameServer::OnMessage(std::shared_ptr<cnet::connection<Msg>> client, cnet::message<Msg>& msg)
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
            break;
        }
        case Msg::Client_CollideTiles:
        {
            PlayerCollisionTiles pt{};
            msg >> pt;

            playerCollideTiles.insert_or_assign(pt.playerId, pt);

            break;
        }

        default:
            break;
        }
    }



    bool GameServer::loadMap(uint32_t** data, int numElems, const std::string& filename)
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


    void GameServer::Simulate(float dt)
    {
       

        for (auto& kv : playerDataMap)
        {
            handleInput(playerIOMap[kv.first], kv.second);
            
            //if (playerCollideTiles.find(kv.first) != playerCollideTiles.end() && playerDataMap.find(kv.first) != playerDataMap.end())
            //{
            //    auto& collideTiles = playerCollideTiles[kv.first];

            //    // player has collideTiles
            //    float startTileX = (float)(collideTiles.startTileXIdx);
            //    float startTileY = (float)(collideTiles.startTileYIdx);

            //    auto startTilePos = sf::Vector2f{ startTileX * (float)collideTiles.tileSize,startTileY * (float)collideTiles.tileSize };
            //    auto startTileCartPos = hlp::ToCart((uint32_t)startTilePos.x, (uint32_t)startTilePos.y);
            //    auto startTileCart = sf::Vector2i{ (int)startTileCartPos.first / (int)collideTiles.tileSize, (int)startTileCartPos.second / (int)collideTiles.tileSize };



            //    float endTileX = (float)(collideTiles.endTileXIdx);
            //    float endTileY = (float)(collideTiles.endTileYIdx);

            //    auto endTilePos = sf::Vector2f{ endTileX * (float)collideTiles.tileSize,endTileY * (float)collideTiles.tileSize };
            //    auto endTileCartPos = hlp::ToCart((uint32_t)startTilePos.x, (uint32_t)startTilePos.y);
            //    auto endTileCart = sf::Vector2i{ (int)endTileCartPos.first / (int)collideTiles.tileSize, (int)endTileCartPos.second / (int)collideTiles.tileSize };


            //    int tileCountX = endTileCart.x - startTileCart.x;
            //    int tileCountY = endTileCart.y - startTileCart.y;

            //    bool collided = false;

            //    myStrings.clear();
            //    myStrings.reserve(tileCountX * tileCountY);
            //    myStrings.emplace_back("\nNumber of Tiles being considered: ");
            //    myStrings.back().append(std::to_string(tileCountX * tileCountY));
            //    myStrings.back().append("\n");

            //    for (uint32_t tileY = (uint32_t)startTileCart.y; tileY <= (uint32_t)endTileCart.y; tileY++)
            //    {
            //        for (uint32_t tileX = (uint32_t)startTileCart.x; tileX <= (uint32_t)endTileCart.x; tileX++)
            //        {
            //            uint32_t num = (uint32_t)tileY * collideTiles.pitch + (uint32_t)tileX;
            //            if (num >= collideTiles.mapTileCount) { break; }
            //            if (currTSetDetails[mapData[num]] != 1)
            //            {
            //                myStrings.emplace_back("\nTile not considered at location in isospace x: ");
            //                myStrings.back().append(std::to_string(hlp::ToScreen( (float)tileX * collideTiles.tileSize,(float)tileY * collideTiles.tileSize).first));
            //                myStrings.back().append(", ");
            //                myStrings.back().append(std::to_string(hlp::ToScreen((float)tileX * collideTiles.tileSize,(float)tileY * collideTiles.tileSize ).second));

            //                continue;
            //            }
            //            else
            //            {
            //                myStrings.emplace_back("\nTile considered at location in isospace x: ");
            //                myStrings.back().append(std::to_string(hlp::ToScreen( (float)tileX * collideTiles.tileSize,(float)tileY * collideTiles.tileSize ).first));
            //                myStrings.back().append(", ");
            //                myStrings.back().append(std::to_string(hlp::ToScreen( (float)tileX * collideTiles.tileSize,(float)tileY * collideTiles.tileSize ).second));


            //                // we have a blocking tile, map back to iso, create a sprite positioned at top left of iso pos, and then make a collider using that sprite
            //                //  then make a sprite and collider for the player
            //                //  then collide and resolve using my awesome diag function
            //                auto isoPos = hlp::ToScreen( (float)tileX * (float)collideTiles.tileSize, (float)tileY * (float)collideTiles.tileSize );
            //                sf::Sprite tile{ dummyTex };
            //                tile.setPosition({ (float)isoPos.first, (float)isoPos.second });
            //                std::unique_ptr<Collider> tileCollider = std::make_unique<IsoTileCollider>(tile);
            //                sf::Sprite pSpr{ dummyTex };
            //                pSpr.setPosition({ (float)hlp::ToScreen((playerDataMap[kv.first].xpos + 124.f) + (playerDataMap[kv.first].xvel * dt),(playerDataMap[kv.first].ypos + 180.f) + (playerDataMap[kv.first].yvel * dt)).first, (float)hlp::ToScreen((playerDataMap[kv.first].xpos + 124.f) + (playerDataMap[kv.first].xvel * dt), (playerDataMap[kv.first].ypos + 180.f) + (playerDataMap[kv.first].yvel * dt)).second });
            //                std::unique_ptr<Collider> playerCollider = std::make_unique<BoxCollider>(pSpr, 50.f, 30.f);

            //                if (Physics::DetectAndResolve(*playerCollider, *tileCollider))
            //                {
            //                    collided = true;
            //                    auto nextPos = hlp::ToCart((uint32_t)pSpr.getPosition().x, (uint32_t)pSpr.getPosition().y);
            //                    playerDataMap[kv.first].xpos = nextPos.first - 124.f;
            //                    playerDataMap[kv.first].ypos = nextPos.second - 180.f;
            //                }
            //            }
            //        }
            //    }

            //    // if no collision took place then the velocity was not applied to the position yet, and we can just do that directly now, in this case
              //  if (!collided)
               // {
                    playerDataMap[kv.first].xpos += playerDataMap[kv.first].xvel * dt;
                    playerDataMap[kv.first].ypos += playerDataMap[kv.first].yvel * dt;
               // }



                animate(playerIOMap[kv.first], kv.second, dt);


                // player has collided with map tiles. right now, the water tiles, safe to send the snapshot over to the client
               

           // }
        }
    }
    void GameServer::BroadcastSnapshot()
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
       
            int i = 0;
            lastSize.clear();
            lastSize.push_back(0);
          
            for (int sz = 0; sz < (int)myStrings.size(); sz++)
            {
                totalSizeOffset = 0;
                for (auto& l : lastSize) totalSizeOffset += (int)l * 18;
                sf::Text text{ crustyFont };
                text.setCharacterSize(18U);
                bottomY = 10.f + (sz * 18.f) + totalSizeOffset + 8.f;
                text.setPosition({ 30.f, bottomY });
                text.setFillColor(sf::Color::Red);
                text.setOutlineColor(sf::Color::White);
                text.setOutlineThickness(2.f);
                text.setString(this->myStrings[sz]);
                window.draw(text);
            }
                
                 
        
        {
            sf::Text text{ crustyFont };
            text.setCharacterSize(18U);
            bottomY = 10.f + (this->m_messagesToDisplay.size() * 18.f) + float(bottomY) + 8.f;
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
            text.setCharacterSize(18U);
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
            auto& io = pIO;
            float sx = 0.f, sy = 0.f;
            if (io.u) sy -= 1.f;   // Up
            if (io.d) sy += 1.f;   // Down
            if (io.l) sx -= 1.f;   // Left
            if (io.r) sx += 1.f;   // Right
            float vx = 0.5f * sx + 0.5f * sy;
            float vy = -0.5f * sx + 0.5f * sy;
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
            if (sy > 0.5f && sx > 0.5f)        pData.dir = Dir::DR; // Down+Right
            else if (sy < -0.5f && sx >  0.5f) pData.dir = Dir::UR; // Up+Right
            else if (sy < -0.5f && sx < -0.5f) pData.dir = Dir::UL; // Up+Left
            else if (sy > 0.5f && sx < -0.5f)  pData.dir = Dir::DL; // Down+Left
            else if (sx > 0.5f)                pData.dir = Dir::R;  // Right
            else if (sx < -0.5f)               pData.dir = Dir::L;  // Left
            else if (sy < -0.5f)               pData.dir = Dir::U;  // Up
            else                               pData.dir = Dir::D;  // Down
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
            if (currFrameDelay != frameAnimDelay) { elapsed = 0; }
            currFrameDelay = frameAnimDelay;
        }
        break;
        case 1: // Run
        {
            if (currFrameDelay != runAnimDelay) { elapsed = 0; }
            currFrameDelay = runAnimDelay;
        }
        break;
        case 2: // Attack
        {
            if (currFrameDelay != attackAnimDelay) { elapsed = 0; }
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

    bool GameServer::initAssets()
    { 
        uint32_t* data = mapData;

        if (!loadMap(&data, 1800, "FrenchTickla"))
        {
           // std::cout << "Was not able to load the map tiles data into the array properly" << std::endl;
            return false;
        }

        return true;
    }
