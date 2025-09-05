#ifndef SNAPSHOTTILES_H__
#define SNAPSHOTTILES_H__

#include <array>
#include <cstdint>

// Collision meaning of a *tileset index* (NOT map index)
enum class TileCol : uint8_t {
    Walk = 0,   // passable
    BlockFeet = 1,   // collide only at bottom band (e.g., 20%)
    RampR = 2,   // right-facing ramp (placeholder)
    RampL = 3    // left-facing ramp (placeholder)
};

// Client -> Server: “here are the 3×3 tiles around me this tick”
struct NeighborhoodTiles {
    uint32_t playerId = 0;
    uint32_t seq = 0;   // ok to keep 0 for now
    int32_t  centerTx = 0;  // integer tile X at player’s FEET
    int32_t  centerTy = 0;  // integer tile Y at player’s FEET
    int32_t top = 0;
    int32_t left = 0;
    int32_t right = 0;
    int32_t bottom = 0;

    std::array<uint32_t, 9> tilesetIdx{}; // 3×3 row-major: TL..BR
};

struct PlayerCollisionTiles
{
    uint32_t playerId = 0;
    int32_t startTileXIdx = -1;
    int32_t endTileXIdx = -1;
    int32_t startTileYIdx = -1;
    int32_t endTileYIdx = -1;
    uint32_t mapTileCount = 0;
    uint32_t tileSize = 0;
    uint32_t pitch = 0;
};



#endif // SNAPSHOTTILES_H__




//#ifndef SNAPSHOTTILES_H__
//#define SNAPSHOTTILES_H__
//// Shared/SnapshotTiles.h
//#include <array>
//#include <cstdint>
//
//// Collision meaning of a *tileset index* (NOT map index)
//enum class TileCol : uint8_t {
//    Walk = 0,   // passable
//    BlockFeet = 1,   // collide only at bottom band (e.g., 20%)
//    RampR = 2,   // right-facing ramp (placeholder)
//    RampL = 3    // left-facing ramp (placeholder)
//};
//
//// Client ? Server: “here are the 3×3 tiles around me this tick”
//struct NeighborhoodTiles {
//    uint32_t playerId;
//    uint32_t seq;        // match with input seq/tick
//    int32_t  centerTx;   // integer tile X at player’s feet
//    int32_t  centerTy;   // integer tile Y at player’s feet
//    std::array<uint32_t, 9> tilesetIdx; // 3×3, row-major: top-left..bottom-right
//};
//#endif

