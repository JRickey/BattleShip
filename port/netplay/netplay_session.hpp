/*
 * Online/LAN multiplayer transport for the SSB64 port (up to 4 players).
 *
 * Topology: star. The host (slot 0) is the lobby authority and relays input
 * between clients. Clients only ever talk to the host, so the only thing a
 * player has to know is the host's IP:port (e.g. the Radmin VPN address).
 *
 * Gameplay model: deterministic lockstep + input delay (see docs/netplay.md).
 * This file knows nothing about the game; it only moves bytes:
 *   - lobby (HELLO / WELCOME / ROSTER / CHOICE / LEAVE),
 *   - match configuration (opaque blob, reliable via resend + ack),
 *   - start barrier (READY / START),
 *   - per-tick controller inputs for 4 slots (redundant, ack driven),
 *   - ABORT.
 *
 * It is written as a class (not globals) so tools/netplay_test.cpp can run
 * 4 sessions in one process over loopback.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace ssb64net {

constexpr int kMaxSlots = 4;
constexpr int kRing = 1024;       // input ring, indexed by tick % kRing
constexpr int kMaxBlob = 512;     // max match-config blob
constexpr int kNameLen = 16;

enum class State { Idle, Hosting, Joining, Connected, Failed };

struct InputFrame {
    uint16_t buttons = 0;
    int8_t stickX = 0;
    int8_t stickY = 0;
};

struct RosterEntry {
    bool used = false;
    char name[kNameLen + 1] = {0};
    uint8_t fighter = 0;
    uint8_t costume = 0;
};

class Session {
  public:
    Session();
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // ---- lobby -----------------------------------------------------------
    bool Host(uint16_t port, const char* name);
    bool Join(const char* host, uint16_t port, const char* name);
    void Stop();                    // leaves the lobby/match, closes socket
    void Poll();                    // call once per frame (non-blocking)

    State GetState() const { return mState; }
    bool IsHost() const { return mState == State::Hosting; }
    int LocalSlot() const { return mSlot; }
    const char* Status() const { return mStatus.c_str(); }
    const RosterEntry& Roster(int slot) const { return mRoster[slot]; }
    int ConnectedCount() const;     // players in lobby, including self
    void SetChoice(int fighter, int costume);
    void SetInputDelay(int d);      // host only; sent with the match config
    int InputDelay() const { return mDelay; }
    void SetLocalName(const char* name);

    // ---- match -----------------------------------------------------------
    // Host: freeze the roster into a match and send `blob` to every client.
    // Returns false if not hosting or nobody is connected.
    bool HostBeginMatch(const void* blob, int size);
    // Client: if a new match config arrived, copies it out, activates the
    // match locally and returns its size (>0). Otherwise returns 0.
    int ClientTakeMatch(void* out, int cap);
    bool MatchActive() const { return mMatchActive; }
    int MatchMask() const { return mMatchMask; }      // bit i => slot i plays
    int MatchDelay() const { return mMatchDelay; }
    void Ready();                   // local side finished loading
    bool StartReleased() const { return mReleased; }
    void EndMatch();                // keeps relaying inputs briefly
    bool ConsumeAbort(std::string* reason);

    // ---- inputs ------------------------------------------------------------
    void PutInput(int slot, uint32_t tick, InputFrame f);   // local or remote
    bool GetInput(int slot, uint32_t tick, InputFrame* out) const;

    std::string DebugString() const;

    // ---- testing -----------------------------------------------------------
    float dropRate = 0.0f;          // simulated packet loss on receive
    uint32_t packetsSent = 0, packetsRecv = 0;

    struct Peer;
    struct Impl;

  private:
    Impl* d;

    State mState = State::Idle;
    int mSlot = 0;
    std::string mStatus = "Idle";
    RosterEntry mRoster[kMaxSlots];
    int mDelay = 5;
    bool mMatchActive = false;
    int mMatchMask = 0;
    int mMatchDelay = 5;
    bool mReleased = false;
};

} // namespace ssb64net
