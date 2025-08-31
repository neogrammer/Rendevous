//#include "net_server.h"
//#include <iostream>
//#include "cid_net.h"
//#include <SFML/Graphics.hpp>
//#include "net_common.h"
//#include <unordered_map>
//#include "../Network_Common.h"
//#include <vector>
//#include <map>
//#include <cmath>
//#include <cstdlib>
//#include "../SnapshotTiles.h"
//
//// ===== Shared constants (keep client & server in sync) =====
//static constexpr int TILE_SIZE = 128;
//// Match the client's offsets exactly:
//static constexpr float FEET_OFFSET_X = 145.f; // client: playerXWidthOffset
//static constexpr float FEET_OFFSET_Y = 156.f; // client: playerZHeightOffset
//static constexpr int MAP_COLS = 40;
//static constexpr int MAP_ROWS = 40;
//
//// ===== Neighborhood cache =====
//struct LatestNeighborhood {
//    uint32_t seq = 0;
//    int32_t  cx = 0, cy = 0;                 // center tile
//    std::array<uint32_t, 9> tiles{};         // 3x3 tileset indices
//    bool valid = false;
//};
//
//// ===== AABB (inclusive, with epsilon) =====
//static inline bool aabbOverlap(float ax0, float ay0, float ax1, float ay1,
//    float bx0, float by0, float bx1, float by1)
//{
//    const float EPS = 1e-4f;
//    return (ax0 <= bx1 + EPS && ax1 >= bx0 - EPS &&
//        ay0 <= by1 + EPS && ay1 >= bx0 - EPS);
//}
//
//class GameServer : public cnet::server_interface<Msg>
//{
//public:
//    // Sim state
//    std::unordered_map<uint32_t, PlayerData> playerDataMap;
//    std::unordered_map<uint32_t, PlayerIO>   playerIOMap;
//    std::unordered_map<uint32_t, LatestNeighborhood> m_neighborhoods; // by playerId
//
//
//    // LUT: tilesetIndex -> TileCol (0=Walk,1=BlockFeet,2/3=ramp placeholders)
//    // NOTE: adjust these per your tileset; indices with value=1 will block
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
//    };
//
//    // Debug
//    uint32_t ioU{}, ioD{}, ioL{}, ioR{}, ioSpace{};
//
//    // Text
//    std::map<uint32_t, std::vector<TEXT>> m_messagesToDisplay;
//    std::unordered_map<TEXT, std::string> m_msgStrLUT = {
//        {TEXT::POOPSIE, "Poopsie Daisy"},
//    };
//
//    std::unordered_map<uint32_t, sPlayerDescription> m_mapPlayerRoster;
//    std::vector<uint32_t> m_vGarbageIDs;
//
//public:
//    GameServer(uint16_t nPort) : cnet::server_interface<Msg>(nPort) {
//        playerDataMap.clear();
//        playerIOMap.clear();
//    }
//
//protected:
//    bool OnClientConnect(std::shared_ptr<cnet::connection<Msg>> client) override {
//        return true;
//    }
//
//    void OnClientValidated(std::shared_ptr<cnet::connection<Msg>> client) override {
//        cnet::message<Msg> msg;
//        msg.header.id = Msg::Client_Accepted;
//        client->Send(msg);
//    }
//
//    void OnClientDisconnect(std::shared_ptr<cnet::connection<Msg>> client) override {
//        if (!client) return;
//
//        auto it = m_mapPlayerRoster.find(client->GetID());
//        if (it != m_mapPlayerRoster.end()) {
//            const auto pid = it->second.nUniqueID;
//            m_mapPlayerRoster.erase(it);
//            m_vGarbageIDs.push_back(pid);
//
//            playerDataMap.erase(pid);
//            playerIOMap.erase(pid);
//            m_neighborhoods.erase(pid);
//        }
//    }
//
//    void handleGarbageRemovals() {
//        if (m_vGarbageIDs.empty()) return;
//        for (auto pid : m_vGarbageIDs) {
//            cnet::message<Msg> m;
//            m.header.id = Msg::Game_RemovePlayer;
//            m << pid;
//            MessageAllClients(m);
//        }
//        m_vGarbageIDs.clear();
//    }
//
//    void OnMessage(std::shared_ptr<cnet::connection<Msg>> client, cnet::message<Msg>& msg) override
//    {
//        handleGarbageRemovals();
//
//        switch (msg.header.id)
//        {
//        case Msg::Client_RegisterWithServer:
//        {
//            sPlayerDescription desc;
//            msg >> desc;
//            desc.nUniqueID = client->GetID();
//            m_mapPlayerRoster.insert_or_assign(desc.nUniqueID, desc);
//
//            // Create sim state
//            static int spawnIndex = 0;
//            const float baseX = 200.f;
//            const float baseY = 200.f;
//            const float gap = 96.f;
//
//            PlayerData p{};
//            p.id = desc.nUniqueID;
//            p.avatarID = 0;
//            p.animID = AnimID::Idle;
//            p.dir = Dir::D;
//            p.frameIndex = 0;
//            p.xpos = baseX + (spawnIndex % 3) * gap;
//            p.ypos = baseY + (spawnIndex / 3) * gap;
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
//            PlayerIO io{}; msg >> io;
//            ioU = io.u; ioD = io.d; ioL = io.l; ioR = io.r; ioSpace = io.space;
//            playerIOMap[client->GetID()] = io;
//            break;
//        }
//
//        case Msg::Client_NeighborhoodTiles:
//        {
//            NeighborhoodTiles nt{};
//            msg >> nt;
//
//            if (!m_mapPlayerRoster.count(nt.playerId)) break;
//
//            auto [it, inserted] = m_neighborhoods.try_emplace(nt.playerId, LatestNeighborhood{});
//            auto& slot = it->second;
//
//            // Optional: if you add seq later, ignore stale: if (nt.seq <= slot.seq) break;
//            slot.seq = nt.seq;
//            slot.cx = nt.centerTx;
//            slot.cy = nt.centerTy;
//            slot.tiles = nt.tilesetIdx;
//            slot.valid = true;
//            break;
//        }
//
//        default:
//            break;
//        }
//    }
//
//    //// ===== Collision against cached 3x3 neighborhood =====
//    //void collidePlayerWithNeighborhood(PlayerData& p, float dt)
//    //{
//    //    // Compute desired velocity from input (isometric intent)
//    //    const auto& io = playerIOMap[p.id];
//    //    float sx = 0.f, sy = 0.f;
//    //    if (io.u) sy -= 1.f;
//    //    if (io.d) sy += 1.f;
//    //    if (io.l) sx -= 1.f;
//    //    if (io.r) sx += 1.f;
//
//    //    float vx = 0.5f * sx + 0.5f * sy;
//    //    float vy = -0.5f * sx + 0.5f * sy;
//
//    //    const float len = std::sqrt(vx * vx + vy * vy);
//    //    if (len > 0.0001f) { vx /= len; vy /= len; }
//
//    //    constexpr float SPEED = 500.f;
//    //    p.xvel = vx * SPEED;
//    //    p.yvel = vy * SPEED;
//
//    //    // Proposed movement
//    //    float nextX = p.xpos + p.xvel * dt;
//    //    float nextY = p.ypos + p.yvel * dt;
//
//    //    // Feet box (assumes p.xpos/p.ypos are FEET; tune if not)
//    //    const float feetW = 150.f;
//    //    const float feetH = 30.f;
//    //    float pf_x0 = nextX - feetW * 0.5f;
//    //    float pf_y0 = nextY - feetH;
//    //    float pf_x1 = nextX + feetW * 0.5f;
//    //    float pf_y1 = nextY;
//
//    //    // If we have no neighborhood yet, move freely
//    //    auto it = g_neighborhoods.find(p.id);
//    //    if (it == g_neighborhoods.end() || !it->second.valid) {
//    //        p.xpos = nextX; p.ypos = nextY;
//    //        return;
//    //    }
//
//    //    const LatestNeighborhood& N = it->second;
//
//    //    // Iterate 3×3
//    //    int i = 0;
//    //    for (int dy = -1; dy <= 1; ++dy) {
//    //        for (int dx = -1; dx <= 1; ++dx, ++i) {
//    //            const uint32_t tilesetIndex = N.tiles[i];
//    //            if (tilesetIndex >= currTSetDetails.size()) continue;
//
//    //            TileCol type = static_cast<TileCol>(currTSetDetails[tilesetIndex]);
//    //            if (type == TileCol::Walk) continue;
//
//    //            const int tx = N.cx + dx;
//    //            const int ty = N.cy + dy;
//    //            const float tileX0 = tx * float(TILE_SIZE);
//    //            const float tileY0 = ty * float(TILE_SIZE);
//    //            const float tileX1 = tileX0 + float(TILE_SIZE);
//    //            const float tileY1 = tileY0 + float(TILE_SIZE);
//
//    //            if (type == TileCol::BlockFeet) {
//    //                // bottom 20% band
//    //                const float band = 0.20f;
//    //                const float by0 = tileY1 - band * float(TILE_SIZE);
//    //                const float by1 = tileY1;
//
//    //                if (aabbOverlap(pf_x0, pf_y0, pf_x1, pf_y1, tileX0, by0, tileX1, by1)) {
//    //                    // push feet to top of band
//    //                    nextY = by0;
//    //                    if (p.yvel > 0.f) p.yvel = 0.f;
//    //                    // recompute feet box
//    //                    pf_y0 = nextY - feetH;
//    //                    pf_y1 = nextY;
//    //                }
//    //            }
//    //            // TODO ramps
//    //        }
//    //    }
//
//    //    p.xpos = nextX;
//    //    p.ypos = nextY;
//    //}
//
//    //void collidePlayerWithNeighborhood(PlayerData& p, float dt)
//    //{
//    //    // --- 1) Desired velocity from input (same as you had) ---
//    //    const auto& io = playerIOMap[p.id];
//    //    float sx = 0.f, sy = 0.f;
//    //    if (io.u) sy -= 1.f;
//    //    if (io.d) sy += 1.f;
//    //    if (io.l) sx -= 1.f;
//    //    if (io.r) sx += 1.f;
//
//    //    float vx = 0.5f * sx + 0.5f * sy;
//    //    float vy = -0.5f * sx + 0.5f * sy;
//
//    //    const float len = std::sqrt(vx * vx + vy * vy);
//    //    if (len > 0.0001f) { vx /= len; vy /= len; }
//
//    //    constexpr float SPEED = 500.f;
//    //    p.xvel = vx * SPEED;
//    //    p.yvel = vy * SPEED;
//
//    //    // Proposed movement
//    //    float nextX = p.xpos + p.xvel * dt;
//    //    float nextY = p.ypos + p.yvel * dt;
//
//
//    //    // Convert sprite-origin position to FEET for collision
//    //    float feetX = nextX + FEET_OFFSET_X;
//    //    float feetY = nextY + FEET_OFFSET_Y;
//
//    //    float feetW = 128.f * 0.9f;  // a bit narrower than a tile
//    //    float feetH = 28.f;          // thin shoes band
//
//    //    float pf_x0 = feetX - feetW * 0.5f;
//    //    float pf_x1 = feetX + feetW * 0.5f;
//    //    float pf_y0 = feetY - feetH;
//    //    float pf_y1 = feetY;
//
//    //    //// --- FEET-BASED AABB (p.xpos/p.ypos are FEET) ---
//    //    //float feetW = 128.f * 0.9f;  // a bit narrower than a tile
//    //    //float feetH = 28.f;          // thin shoe band
//
//    //    //// nextX/nextY are FEET
//    //    //float pf_x0 = nextX - feetW * 0.5f;
//    //    //float pf_x1 = nextX + feetW * 0.5f;
//    //    //float pf_y0 = nextY - feetH;
//    //    //float pf_y1 = nextY;
//
//    //    // Feet box (assumes p.xpos/p.ypos are FEET)
//    //    //float feetW = 128.f * 0.9f;  // narrower than a tile to reduce multi-column hits
//    //    //float feetH = 28.f;          // thin band around shoes
//    //    //float pf_x0 = nextX - feetW * 0.5f;
//    //    //float pf_x1 = nextX + feetW * 0.5f;
//    //    //float pf_y0 = nextY - feetH;
//    //    //float pf_y1 = nextY;
//
//    //    // No neighborhood? free move
//    //    auto it = g_neighborhoods.find(p.id);
//    //    if (it == g_neighborhoods.end() || !it->second.valid) {
//    //        p.xpos = nextX; p.ypos = nextY; return;
//    //    }
//    //    const LatestNeighborhood& N = it->second;
//
//    //    // --- 2) Resolve Y against BlockFeet, only when moving DOWN ---
//    //    if (p.yvel > 0.f) {
//    //        std::optional<float> minBy0; // the tightest ceiling we hit this tick
//
//    //        int i = 0;
//    //        for (int dy = -1; dy <= 1; ++dy) {
//    //            for (int dx = -1; dx <= 1; ++dx, ++i) {
//    //                const uint32_t tilesetIndex = N.tiles[i];
//    //                if (tilesetIndex >= currTSetDetails.size()) continue;
//
//    //                TileCol type = static_cast<TileCol>(currTSetDetails[tilesetIndex]);
//
//    //                if (type == TileCol::BlockFeet) {
//    //                    // Tile rect
//    //                    const float tileX0 = tx * float(TILE_SIZE);
//    //                    const float tileY0 = ty * float(TILE_SIZE);
//    //                    const float tileX1 = tileX0 + float(TILE_SIZE);
//    //                    const float tileY1 = tileY0 + float(TILE_SIZE);
//
//    //                    // Bottom band (20% of tile height)
//    //                    const float band = 0.20f;
//    //                    const float by0 = tileY1 - band * float(TILE_SIZE);
//    //                    const float by1 = tileY1;
//
//    //                    const float EPS = 1e-4f;
//
//    //                    // --- vertical resolution: only when moving DOWN into the band ---
//    //                    if (p.yvel > 0.f) {
//    //                        const bool xOver =
//    //                            (pf_x0 <= tileX1 + EPS) && (pf_x1 >= tileX0 - EPS);
//    //                        const bool yOver =
//    //                            (pf_y0 <= by1 + EPS) && (pf_y1 >= by0 - EPS);
//
//    //                        if (xOver && yOver) {
//    //                            // Clamp feet to the top of the band; this is the tightest vertical stop
//    //                            nextY = std::min(nextY, by0);
//    //                            p.yvel = 0.f;
//    //                            pf_y0 = nextY - feetH;
//    //                            pf_y1 = nextY;
//    //                        }
//    //                    }
//
//    //                    // --- horizontal resolution: block sides when overlapping band vertically ---
//    //                    // If the feet cross the band's vertical span, treat band edges as walls
//    //                    {
//    //                        const bool yBandOverlap =
//    //                            (pf_y0 <= by1 + EPS) && (pf_y1 >= by0 - EPS);
//
//    //                        if (yBandOverlap) {
//    //                            // moving right into the band's left edge?
//    //                            if (p.xvel > 0.f && (pf_x1 > tileX0 - EPS) && (pf_x0 < tileX0)) {
//    //                                nextX = std::min(nextX, tileX0 - feetW * 0.5f);
//    //                                p.xvel = 0.f;
//    //                                pf_x0 = nextX - feetW * 0.5f;
//    //                                pf_x1 = nextX + feetW * 0.5f;
//    //                            }
//    //                            // moving left into the band's right edge?
//    //                            if (p.xvel < 0.f && (pf_x0 < tileX1 + EPS) && (pf_x1 > tileX1)) {
//    //                                nextX = std::max(nextX, tileX1 + feetW * 0.5f);
//    //                                p.xvel = 0.f;
//    //                                pf_x0 = nextX - feetW * 0.5f;
//    //                                pf_x1 = nextX + feetW * 0.5f;
//    //                            }
//    //                        }
//    //                    }
//    //                }
//
//
//
//
//    //                //if (type != TileCol::BlockFeet) continue;
//
//    //                //const int tx = N.cx + dx;
//    //                //const int ty = N.cy + dy;
//    //                //const float tileX0 = tx * float(TILE_SIZE);
//    //                //const float tileY0 = ty * float(TILE_SIZE);
//    //                //const float tileX1 = tileX0 + float(TILE_SIZE);
//    //                //const float tileY1 = tileY0 + float(TILE_SIZE);
//
//    //                //// bottom band of the tile
//    //                //const float band = 0.20f;
//    //                //const float by0 = tileY1 - band * float(TILE_SIZE);
//    //                //const float by1 = tileY1;
//
//    //                //// Quick horizontal overlap check fwwirst (cheaper)
//    //                //const float EPS = 1e-4f;
//    //                //const bool xOver =
//    //                //    (pf_x0 <= tileX1 + EPS) && (pf_x1 >= tileX0 - EPS);
//
//    //                //if (!xOver) continue;
//
//    //                //// Now vertical overlap with the band
//    //                //const bool yOver =
//    //                //    (pf_y0 <= by1 + EPS) && (pf_y1 >= by0 - EPS);
//
//    //                //if (yOver) {
//    //                //    // We can't go below this band's top
//    //                //    if (!minBy0.has_value() || by0 < *minBy0) {
//    //                //        minBy0 = by0;
//    //                //    }
//    //                //}
//    //            }
//    //        }
//
//
//    //        if (minBy0.has_value()) {
//    //            feetY = std::min(feetY, *minBy0);
//    //            if (p.yvel > 0.f) p.yvel = 0.f;
//
//    //            pf_y0 = feetY - feetH;
//    //            pf_y1 = feetY;
//    //        }
//
//    //        //if (minBy0.has_value()) {
//    //        //    // Clamp feet to the tightest band's top
//    //        //    nextY = std::min(nextY, *minBy0);
//    //        //    if (p.yvel > 0.f) p.yvel = 0.f;
//
//    //        //    // Recompute feet box after correction
//    //        //    pf_y0 = nextY - feetH;
//    //        //    pf_y1 = nextY;
//    //        //}
//    //    }
//
//    //    // (No X blocking for BlockFeet; ramps come later.)
//
//    //    // Commit
//    //    // Convert feet back to sprite-origin for storage
//    //    nextX = feetX - FEET_OFFSET_X;
//    //    nextY = feetY - FEET_OFFSET_Y;
//
//    //    p.xpos = nextX;
//    //    p.ypos = nextY;
//    //}
//void collidePlayerWithNeighborhood(PlayerData& p, float dt)
//{
//    // 0) Velocity from input (same as before)
//    const auto& io = playerIOMap[p.id];
//    float sx = 0.f, sy = 0.f;
//    if (io.u) sy -= 1.f;
//    if (io.d) sy += 1.f;
//    if (io.l) sx -= 1.f;
//    if (io.r) sx += 1.f;
//
//    float vx = 0.5f * sx + 0.5f * sy;
//    float vy = -0.5f * sx + 0.5f * sy;
//    const float len = std::sqrt(vx * vx + vy * vy);
//    if (len > 0.0001f) { vx /= len; vy /= len; }
//
//    constexpr float SPEED = 500.f;
//    p.xvel = vx * SPEED;
//    p.yvel = vy * SPEED;
//
//    // 1) Convert sprite-origin to FEET space and apply proposed move
//    float feetX = (p.xpos + p.xvel * dt) + FEET_OFFSET_X;
//    float feetY = (p.ypos + p.yvel * dt) + FEET_OFFSET_Y;
//
//    // Feet AABB (in FEET space)
//    const float feetW = 128.f * 0.90f;
//    const float feetH = 28.f;
//    auto rebuildFeetBox = [&](float& x0, float& y0, float& x1, float& y1) {
//        x0 = feetX - feetW * 0.5f;
//        x1 = feetX + feetW * 0.5f;
//        y0 = feetY - feetH;
//        y1 = feetY;
//        };
//    float pf_x0, pf_y0, pf_x1, pf_y1;
//    rebuildFeetBox(pf_x0, pf_y0, pf_x1, pf_y1);
//
//    auto it = m_neighborhoods.find(p.id);
//    if (it != m_neighborhoods.end() && it->second.valid)
//    {
//        const LatestNeighborhood& N = it->second;
//        const int TILE = TILE_SIZE;
//        const float band = 0.20f;      // bottom 20% blocks
//        const float EPS = 1e-4f;
//
//        // ---- Phase A: resolve Y (only when moving DOWN) ----
//        if (p.yvel > 0.f) {
//            float bestY = feetY; // clamp to the tightest band's top
//            int i = 0;
//            for (int dy = -1; dy <= 1; ++dy) {
//                for (int dx = -1; dx <= 1; ++dx, ++i) {
//                    const uint32_t idx = N.tiles[i];
//                    if (idx >= currTSetDetails.size()) continue;
//                    if (static_cast<TileCol>(currTSetDetails[idx]) != TileCol::BlockFeet) continue;
//
//                    const int tx = N.cx + dx;
//                    const int ty = N.cy + dy;
//                    const float tileX0 = tx * float(TILE);
//                    const float tileY0 = ty * float(TILE);
//                    const float tileX1 = tileX0 + float(TILE);
//                    const float tileY1 = tileY0 + float(TILE);
//
//                    // bottom band
//                    const float by0 = tileY1 - band * float(TILE);
//                    const float by1 = tileY1;
//
//                    // overlap test: horizontal overlap with tile, and vertical overlap with band
//                    const bool xOver = (pf_x0 <= tileX1 + EPS) && (pf_x1 >= tileX0 - EPS);
//                    const bool yOver = (pf_y0 <= by1 + EPS) && (pf_y1 >= by0 - EPS);
//                    if (xOver && yOver) {
//                        bestY = std::min(bestY, by0);
//                    }
//                }
//            }
//            if (bestY < feetY) {
//                feetY = bestY;
//                p.yvel = 0.f;
//                rebuildFeetBox(pf_x0, pf_y0, pf_x1, pf_y1);
//            }
//        }
//
//        // ---- Phase B: resolve X against band sides (either direction) ----
//        if (p.xvel != 0.f) {
//            int i = 0;
//            for (int dy = -1; dy <= 1; ++dy) {
//                for (int dx = -1; dx <= 1; ++dx, ++i) {
//                    const uint32_t idx = N.tiles[i];
//                    if (idx >= currTSetDetails.size()) continue;
//                    if (static_cast<TileCol>(currTSetDetails[idx]) != TileCol::BlockFeet) continue;
//
//                    const int tx = N.cx + dx;
//                    const int ty = N.cy + dy;
//                    const float tileX0 = tx * float(TILE);
//                    const float tileY0 = ty * float(TILE);
//                    const float tileX1 = tileX0 + float(TILE);
//                    const float tileY1 = tileY0 + float(TILE);
//
//                    // vertical span of the band
//                    const float by0 = tileY1 - band * float(TILE);
//                    const float by1 = tileY1;
//
//                    // only treat sides as walls if we overlap the band's vertical span
//                    const bool yBandOverlap = (pf_y0 <= by1 + EPS) && (pf_y1 >= by0 - EPS);
//                    if (!yBandOverlap) continue;
//
//                    if (p.xvel > 0.f) { // moving right -> collide with tile's left edge
//                        if (pf_x1 > tileX0 - EPS && pf_x0 < tileX0) {
//                            feetX = std::min(feetX, tileX0 - feetW * 0.5f);
//                            p.xvel = 0.f;
//                            rebuildFeetBox(pf_x0, pf_y0, pf_x1, pf_y1);
//                        }
//                    }
//                    else {             // moving left -> collide with tile's right edge
//                        if (pf_x0 < tileX1 + EPS && pf_x1 > tileX1) {
//                            feetX = std::max(feetX, tileX1 + feetW * 0.5f);
//                            p.xvel = 0.f;
//                            rebuildFeetBox(pf_x0, pf_y0, pf_x1, pf_y1);
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//    // ---- World bounds clamp (FEET space) ----
//    const float worldW = MAP_COLS * float(TILE_SIZE);
//    const float worldH = MAP_ROWS * float(TILE_SIZE);
//    feetX = std::clamp(feetX, 0.0f + feetW * 0.5f, worldW - feetW * 0.5f);
//    feetY = std::clamp(feetY, 0.0f + feetH, worldH); // stop when feet band hits bottom
//
//    // 2) Convert back to sprite-origin and commit
//    p.xpos = feetX - FEET_OFFSET_X;
//    p.ypos = feetY - FEET_OFFSET_Y;
//}
//
//
//public:
//    // ===== One simulation tick =====
//    void Simulate(float dt)
//    {
//        for (auto& kv : playerDataMap)
//        {
//            auto& p = kv.second;
//            const auto& io = playerIOMap[p.id];
//
//            // dir/anim (kept)
//            float sx = (io.r ? 1.f : 0.f) + (io.l ? -1.f : 0.f);
//            float sy = (io.d ? 1.f : 0.f) + (io.u ? -1.f : 0.f);
//
//            if (std::abs(sx) > 0.001f || std::abs(sy) > 0.001f) {
//                if (sy > 0.5f && sx > 0.5f)        p.dir = Dir::DR;
//                else if (sy < -0.5f && sx >  0.5f) p.dir = Dir::UR;
//                else if (sy < -0.5f && sx < -0.5f) p.dir = Dir::UL;
//                else if (sy > 0.5f && sx < -0.5f)  p.dir = Dir::DL;
//                else if (sx > 0.5f)                p.dir = Dir::R;
//                else if (sx < -0.5f)               p.dir = Dir::L;
//                else if (sy < -0.5f)               p.dir = Dir::U;
//                else                                p.dir = Dir::D;
//
//                if (p.animID != AnimID::Run && p.animID != AnimID::Attack) { p.animID = AnimID::Run; p.frameIndex = 0; }
//            }
//            else {
//                if (p.animID != AnimID::Idle && p.animID != AnimID::Attack) { p.animID = AnimID::Idle; p.frameIndex = 0; }
//            }
//
//            if (io.space) {
//                if (p.animID != AnimID::Attack) { p.animID = AnimID::Attack; p.frameIndex = 0; }
//            }
//
//            // Always collide when moving; fallback to free move if no neighborhood yet
//            const bool moving = io.l || io.r || io.u || io.d;
//            if (moving) {
//                collidePlayerWithNeighborhood(p, dt);
//            }
//            else {
//                // idle; keep position
//            }
//
//            // anim frame
//            p.frameIndex = (p.frameIndex + 1u) % 32u;
//        }
//    }
//
//    void BroadcastSnapshot()
//    {
//        cnet::message<Msg> out;
//        out.header.id = Msg::Server_PlayerDrawSnapshot;
//
//        std::vector<PlayerDrawData> vec;
//        vec.reserve(playerDataMap.size());
//        for (const auto& kv : playerDataMap)
//        {
//            const auto& p = kv.second;
//            PlayerDrawData d{};
//            d.id = p.id; d.xpos = p.xpos; d.ypos = p.ypos;
//            d.animID = p.animID; d.dir = p.dir; d.frameIndex = p.frameIndex;
//            vec.push_back(d);
//        }
//
//        for (const auto& d : vec) out << d;
//        uint32_t count = static_cast<uint32_t>(vec.size());
//        out << count;
//
//        MessageAllClients(out);
//    }
//};
//
//int main(int argc, char* argv[])
//{
//    GameServer server(60000);
//    server.Start();
//
//    sf::RenderWindow window(sf::VideoMode({ 1280, 920 }), "GameServer!");
//    if (!window.isOpen()) return 420;
//    window.setPosition(sf::Vector2i(1670, 520));
//
//    // Hide console window
//    HWND hWnd = GetConsoleWindow();
//    ShowWindow(hWnd, SW_HIDE);
//
//    sf::Font crustyFont{ "assets/font/Crusty.ttf" };
//
//    const float dt = 1.0f / 60.0f;
//    float accumulator = 0.f;
//    sf::Clock clock;
//
//    while (window.isOpen())
//    {
//        server.Update(-1);
//
//        while (const std::optional ev = window.pollEvent())
//            if (ev->is<sf::Event::Closed>()) window.close();
//
//        accumulator += clock.restart().asSeconds();
//        while (accumulator >= dt) { server.Simulate(dt); accumulator -= dt; }
//
//        server.BroadcastSnapshot();
//
//        window.clear();
//        // (draw debug text same as your code — omitted for brevity)
//        window.display();
//    }
//    return 0;
//}


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




struct LatestNeighborhood {
    uint32_t seq = 0;
    int32_t  cx = 0, cy = 0;
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
    bool hasTilesToCheck{ false };
public:
    // Sim state
    std::unordered_map<uint32_t, PlayerData> playerDataMap;

    float zHeightOffset = 156.f;
    std::unordered_map<uint32_t, PlayerIO>   playerIOMap;
    void collidePlayerWithNeighborhood(PlayerData& p, float dt)
    {
        auto it = m_neighborhoods.find(p.id);

        static const float SPEED = 500.f;
        float sx = 0.f, sy = 0.f;
        if (this->playerIOMap[p.id].u) sy -= 1.f;   // Up
        if (this->playerIOMap[p.id].d) sy += 1.f;   // Down
        if (this->playerIOMap[p.id].l) sx -= 1.f;   // Left
        if (this->playerIOMap[p.id].r) sx += 1.f;   // Right
        float vx = 0.5f * sx + 0.5f * sy;
        float vy = -0.5f * sx + 0.5f * sy;
        const float len = std::sqrt(vx * vx + vy * vy);
        if (len > 0.0001f) { vx /= len; vy /= len; }

        p.xvel = vx * SPEED;
        p.yvel = vy * SPEED;

        if (it == m_neighborhoods.end() || !it->second.valid) {
            // No neighborhood tiles yet -> allow movement (or clamp)
            
            


            p.xpos += p.xvel * dt;
            p.ypos += p.yvel * dt;
            return;
        }
       
        const LatestNeighborhood& N = it->second;
        // Proposed movement
        float nextX = p.xpos + p.xvel * dt;
        float nextY = p.ypos + p.yvel * dt;

        // Player feet box (tune these to your sprite)
        const float feetW = 150.f;
        const float feetH = 30.f;
        float pf_x0 = nextX - feetW * 0.5f;
        float pf_y0 = nextY - feetH;      // feet rectangle just above the feet point
        float pf_x1 = nextX + feetW * 0.5f;
        float pf_y1 = nextY;

        // Iterate 3×3 tiles and block if colliding with BlockFeet band
        const int T = 128;                 // tile size (server constant)
        int i = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx, ++i) {
                const uint32_t tilesetIndex = N.tiles[i];
                if (tilesetIndex >= currTSetDetails.size()) continue;

                TileCol type = (TileCol)currTSetDetails[tilesetIndex];
                if (type == TileCol::Walk) continue;

                const int tx = N.cx + dx;
                const int ty = N.cy + dy;
                const float tileX0 = tx * float(T);
                const float tileY0 = ty * float(T);
                const float tileX1 = tileX0 + float(T);
                const float tileY1 = tileY0 + float(T);

                if (type == TileCol::BlockFeet) {
                    // Only collide with the bottom band (e.g., bottom 20%)
                    const float band = 0.20f;
                    const float by0 = tileY1 - band * float(T);
                    const float by1 = tileY1;
                    if (aabbOverlap(pf_x0, pf_y0, pf_x1, pf_y1, tileX0, by0, tileX1, by1)) {
                        // push player up to top of band
                        nextY = by0;  // feet touch band top
                        // zero Y vel if moving into it
                        if (p.yvel > 0.f) p.yvel = 0.f;
                        // recompute feet box after correction
                        pf_y0 = nextY - feetH;
                        pf_y1 = nextY;
                    }
                }
                else if (type == TileCol::RampR || type == TileCol::RampL) {
                    // TODO: handle ramps (compute tile-local height at x; clamp feetY)
                    // Keep as Walk for now or implement your ramp curve.
                }
                else {
                    // If you later add full blocking, handle here
                }
            }
        }

        p.xpos = nextX;
        p.ypos = nextY;
    }
   
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
            pio = io; // overwrite this frame's input
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

            hasTilesToCheck = true;
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

            if (io.space)
            {
                vx = 0.f;
                vy = 0.f;
            }
            else {
                if (p.animID == AnimID::Attack)
                {
                    p.animID = AnimID::Idle;
                    p.frameIndex = 0;
                }
            }

           // p.xvel = kv.second.xvel;// vx* SPEED;
           // p.yvel = kv.second.yvel;// vy* SPEED;
            
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

                if (p.animID != AnimID::Run && p.animID != AnimID::Attack) { p.animID = AnimID::Run; p.frameIndex = 0; }
            }
            else
            {
                if (p.animID != AnimID::Idle && p.animID != AnimID::Attack) { p.animID = AnimID::Idle; p.frameIndex = 0; }
            }

            if (io.space)
            {
                if (p.animID != AnimID::Attack)
                {
                    p.animID = AnimID::Attack;
                    p.frameIndex = 0;
                }
            }

            // Advance position
            


            if (hasTilesToCheck)
            {
                // with:
                collidePlayerWithNeighborhood(p, dt);
            }


            hasTilesToCheck = false;

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
        for (const auto& d : vec) out << d;

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
        static int totalSizeOffset = 0;
        static float bottomY = 0.f;
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
                    bottomY = 10.f + (sz * 32.f) + totalSizeOffset + 8.f;
                    text.setPosition({ 30.f, bottomY });
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
        {
            sf::Text text{ crustyFont };
            text.setCharacterSize(32U);
            bottomY = 10.f + (server.m_messagesToDisplay.size() * 32.f) + float(bottomY) + 8.f;
            text.setPosition({ 30.f,bottomY });
            text.setFillColor(sf::Color::Red);
            text.setOutlineColor(sf::Color::White);
            text.setOutlineThickness(2.f);
            std::string str = std::to_string(server.ioD) + " " + std::to_string(server.ioL) + " " +
                std::to_string(server.ioU) + " " + std::to_string(server.ioR) + " " + std::to_string(server.ioSpace);
            text.setString(str);
            window.draw(text);
        }

        // draw each roster entry without inserting new ones
        float y = bottomY;
        for (const auto& [id, desc] : server.m_mapPlayerRoster) {
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
       


    /*    sf::Text text{ crustyFont };
        text.setCharacterSize(32U);
        text.setPosition({ 30.f, 10.f + (server.m_messagesToDisplay.size() * 32.f) + totalSizeOffset + 8.f });
        text.setFillColor(sf::Color::Red);
        text.setOutlineColor(sf::Color::White);
        text.setOutlineThickness(2.f);
        std::string str = std::to_string(server.ioD) + " " + std::to_string(server.ioL) + " " +
            std::to_string(server.ioU) + " " + std::to_string(server.ioR) + " " + std::to_string(server.ioSpace);
        text.setString(str);
        window.draw(text);*/


        window.display();
    }

    return 0;
}