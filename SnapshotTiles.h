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
    std::array<uint32_t, 9> tilesetIdx{}; // 3×3 row-major: TL..BR
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

