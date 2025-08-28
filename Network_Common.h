#pragma once
#include <stdint.h>
#include <utility>

enum class Msg : uint32_t
{
    Server_GetStatus,
    Server_GetPing,

    // Server -> Client
    Server_PlayerDrawSnapshot,   // batched draw data for this frame

    // Text chat (kept)
    Client_SendText,

    // Handshake / session
    Client_Accepted,
    Client_AssignID,
    Client_RegisterWithServer,
    Client_UnregisterWithServer,

    // Legacy game events (kept for join/leave)
    Game_AddPlayer,
    Game_RemovePlayer,

    // Client -> Server
    Client_IO
};

enum class AnimID : uint32_t { Idle = 0, Run=1, Attack=2 };
enum class Dir : uint32_t { D = 0, DR, R, UR, U, UL, L, DL };
enum class TEXT : uint32_t { POOPSIE };

struct sPlayerDescription
{
    uint32_t nUniqueID = 0;
    uint32_t nAvatarID = 0;

    uint32_t nHealth = 100;
    uint32_t nAmmo = 20;
    uint32_t nKills = 0;
    uint32_t nDeaths = 0;

    float fRadius = 0.5f;

    std::pair<float, float> vPos{ 200.f,200.f };
    std::pair<float, float> vVel{ 0.f,0.f };
};

struct PlayerIO
{
    bool d = false;
    bool l = false;
    bool r = false;
    bool u = false;
};

struct PlayerData            // Server-side simulation state
{
    uint32_t id = 0;
    uint32_t avatarID = 0;

    float xpos = 200.f, ypos = 200.f;
    float xvel = 0.f, yvel = 0.f;

    AnimID animID = AnimID::Idle;
    Dir    dir = Dir::D;
    uint32_t frameIndex = 0;
};

struct PlayerDrawData        // Sent to clients (minimal for rendering)
{
    uint32_t id = 0;
    float xpos = 0.f, ypos = 0.f;
    AnimID animID = AnimID::Idle;
    Dir    dir = Dir::D;
    uint32_t frameIndex = 0;
};

struct sMessageFromClient
{
    uint32_t nUniqueID = 0;
    TEXT message = TEXT::POOPSIE;
};


// --- helpers for enum <-> u32 ---
static inline uint32_t to_u32(AnimID a) { return static_cast<uint32_t>(a); }
static inline uint32_t to_u32(Dir d) { return static_cast<uint32_t>(d); }
static inline uint32_t to_u32(TEXT t) { return static_cast<uint32_t>(t); }
static inline AnimID  to_anim(uint32_t v) { return static_cast<AnimID>(v); }
static inline Dir     to_dir(uint32_t v) { return static_cast<Dir>(v); }
static inline TEXT    to_text(uint32_t v) { return static_cast<TEXT>(v); }

// ===== PlayerIO =====
//inline cnet::message<Msg>& operator<<(cnet::message<Msg>& msg, const PlayerIO& v) {
//    uint8_t b = (v.u ? 1 : 0) | ((v.d ? 1 : 0) << 1) | ((v.l ? 1 : 0) << 2) | ((v.r ? 1 : 0) << 3);
//    msg << b;
//    return msg;
//}
//inline cnet::message<Msg>& operator>>(cnet::message<Msg>& msg, PlayerIO& v) {
//    uint8_t b = 0; msg >> b;
//    v.u = (b & 0x01) != 0;
//    v.d = (b & 0x02) != 0;
//    v.l = (b & 0x04) != 0;
//    v.r = (b & 0x08) != 0;
//    return msg;
//}


inline cnet::message<Msg>& operator<<(cnet::message<Msg>& msg, const PlayerIO& v) {
    msg << v.u
        << v.r
        << v.l
        << v.d;
    return msg;
}

inline cnet::message<Msg>& operator>>(cnet::message<Msg>& msg, PlayerIO& v) {
    msg >> v.d
        >> v.l
        >> v.r
        >> v.u;
    return msg;
}
// ===== sMessageFromClient =====
inline cnet::message<Msg>& operator<<(cnet::message<Msg>& msg, const sMessageFromClient& v) {
    msg << to_u32(v.message) << v.nUniqueID;
    return msg;
}
inline cnet::message<Msg>& operator>>(cnet::message<Msg>& msg, sMessageFromClient& v) {
    uint32_t t = 0; msg >> t >> v.nUniqueID;
    v.message = to_text(t);
    return msg;
}

// ===== sPlayerDescription =====
// (avoid serializing std::pair directly; write fields explicitly)
inline cnet::message<Msg>& operator<<(cnet::message<Msg>& msg, const sPlayerDescription& p) {
    msg << p.vVel.second << p.vVel.first
        << p.vPos.second << p.vPos.first
        << p.fRadius
        << p.nDeaths << p.nKills << p.nAmmo << p.nHealth
        << p.nAvatarID << p.nUniqueID;
    return msg;
}
inline cnet::message<Msg>& operator>>(cnet::message<Msg>& msg, sPlayerDescription& p) {
    msg >> p.nUniqueID >> p.nAvatarID
        >> p.nHealth >> p.nAmmo >> p.nKills >> p.nDeaths
        >> p.fRadius
        >> p.vPos.first >> p.vPos.second
        >> p.vVel.first >> p.vVel.second;
    return msg;
}

// ===== PlayerData (server sim state) =====
inline cnet::message<Msg>& operator<<(cnet::message<Msg>& msg, const PlayerData& d) {
    msg << d.frameIndex
        << to_u32(d.dir)
        << to_u32(d.animID)
        << d.yvel << d.xvel
        << d.ypos << d.xpos
        << d.avatarID << d.id;
    return msg;
}
inline cnet::message<Msg>& operator>>(cnet::message<Msg>& msg, PlayerData& d) {
    uint32_t a = 0, di = 0;
    msg >> d.id >> d.avatarID
        >> d.xpos >> d.ypos
        >> d.xvel >> d.yvel
        >> a >> di
        >> d.frameIndex;
    d.animID = to_anim(a);
    d.dir = to_dir(di);
    return msg;
}

// ===== PlayerDrawData (server -> client snapshot) =====
inline cnet::message<Msg>& operator<<(cnet::message<Msg>& msg, const PlayerDrawData& d) {
    msg << d.frameIndex
        << to_u32(d.dir)
        << to_u32(d.animID)
        << d.ypos << d.xpos
        << d.id;
    return msg;
}
inline cnet::message<Msg>& operator>>(cnet::message<Msg>& msg, PlayerDrawData& d) {
    uint32_t a = 0, di = 0;
    msg >> d.id >> d.xpos >> d.ypos >> a >> di >> d.frameIndex;
    d.animID = to_anim(a);
    d.dir = to_dir(di);
    return msg;
}
