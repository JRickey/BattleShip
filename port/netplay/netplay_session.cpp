// SPDX-License-Identifier: MIT
#include "netplay_session.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
static const sock_t kBadSock = INVALID_SOCKET;
static void closeSock(sock_t s) { closesocket(s); }
static bool setNonBlocking(sock_t s) { u_long m = 1; return ioctlsocket(s, FIONBIO, &m) == 0; }
typedef int socklen_compat;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int sock_t;
static const sock_t kBadSock = -1;
static void closeSock(sock_t s) { close(s); }
static bool setNonBlocking(sock_t s) { int f = fcntl(s, F_GETFL, 0); return f >= 0 && fcntl(s, F_SETFL, f | O_NONBLOCK) == 0; }
typedef socklen_t socklen_compat;
#endif

namespace ssb64net {

namespace {

constexpr uint32_t kMagic = 0x4E505342; // "BSPN"
constexpr uint8_t kVersion = 2;

enum Type : uint8_t {
    T_HELLO = 1, T_WELCOME, T_REJECT, T_ROSTER, T_CHOICE, T_LEAVE,
    T_CONFIG, T_CONFIG_ACK, T_READY, T_START, T_INPUT, T_ABORT
};

constexpr int kMaxFramesPerBlock = 32;
constexpr uint64_t kRosterEveryMs = 500;
constexpr uint64_t kChoiceEveryMs = 500;
constexpr uint64_t kResendEveryMs = 150;
constexpr uint64_t kLobbyTimeoutMs = 10000;
constexpr uint64_t kLoadTimeoutMs = 25000;
constexpr uint64_t kMatchTimeoutMs = 8000;
constexpr uint64_t kLingerMs = 2000;

uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

struct Writer {
    uint8_t buf[1400];
    size_t n = 0;
    void u8(uint32_t v) { if (n < sizeof(buf)) buf[n++] = (uint8_t)v; }
    void u16(uint32_t v) { u8(v >> 8); u8(v); }
    void u32(uint32_t v) { u16(v >> 16); u16(v); }
    void bytes(const void* p, size_t len) { if (n + len <= sizeof(buf)) { memcpy(buf + n, p, len); n += len; } }
};

struct Reader {
    const uint8_t* p;
    size_t n;
    size_t i = 0;
    bool bad = false;
    Reader(const uint8_t* p_, size_t n_) : p(p_), n(n_) {}
    uint32_t u8() { if (i + 1 > n) { bad = true; return 0; } return p[i++]; }
    uint32_t u16() { uint32_t a = u8(); return (a << 8) | u8(); }
    uint32_t u32() { uint32_t a = u16(); return (a << 16) | u16(); }
    void bytes(void* out, size_t len) { if (i + len > n) { bad = true; memset(out, 0, len); return; } memcpy(out, p + i, len); i += len; }
};

struct Ring {
    struct Cell { uint32_t tick = 0; bool valid = false; InputFrame f; };
    Cell cells[kRing];
    uint32_t next = 0; // all ticks < next are present

    void clear() { for (auto& c : cells) c = Cell(); next = 0; }
    bool get(uint32_t tick, InputFrame* out) const {
        const Cell& c = cells[tick % kRing];
        if (!c.valid || c.tick != tick) return false;
        if (out) *out = c.f;
        return true;
    }
    void put(uint32_t tick, InputFrame f) {
        if (tick + kRing / 2 < next) return;          // far too old
        if (tick > next + kRing / 2) return;          // far too new (garbage)
        Cell& c = cells[tick % kRing];
        c.tick = tick; c.valid = true; c.f = f;
        while (get(next, nullptr)) next++;
    }
};

} // namespace

struct Session::Peer {
    bool used = false;
    sockaddr_in addr{};
    uint64_t lastRecv = 0;
    uint32_t ack[kMaxSlots] = {0, 0, 0, 0}; // peer's "next needed tick" per slot
    bool configAcked = false;
    bool ready = false;
    uint64_t lastConfigSend = 0;
};

struct Session::Impl {
    sock_t sock = kBadSock;
    Peer peers[kMaxSlots];       // host: index = slot (0 unused). client: [0] = host
    sockaddr_in hostAddr{};
    Ring rings[kMaxSlots];
    uint32_t hostAck[kMaxSlots] = {0, 0, 0, 0}; // client: host's next-needed
    char name[kNameLen + 1] = {0};
    uint8_t myFighter = 0, myCostume = 0;
    uint64_t lastRoster = 0, lastChoice = 0, lastHello = 0, lastReadySend = 0;
    uint64_t lastHostRecv = 0;
    uint64_t matchStart = 0, releasedAt = 0, lingerUntil = 0, startSentUntil = 0, lastStartSend = 0;
    uint32_t matchId = 0;         // current/last match
    uint32_t consumedId = 0;      // client: last match id taken
    bool localReady = false;
    bool rosterDirty = false;
    bool pendingCfg = false;
    std::vector<uint8_t> blob;    // config blob (host: to send, client: pending)
    uint8_t cfgMask = 0, cfgDelay = 5;
    bool aborted = false;
    std::string abortReason;
    uint32_t rng = 12345;
    int abortSends = 0;
    uint64_t lastAbortSend = 0;
};

// ---------------------------------------------------------------------------

Session::Session() : d(new Impl) {
#ifdef _WIN32
    static bool wsaInit = false;
    if (!wsaInit) { WSADATA w; WSAStartup(MAKEWORD(2, 2), &w); wsaInit = true; }
#endif
    d->rng = (uint32_t)(nowMs() * 2654435761u) | 1u;
}

Session::~Session() { Stop(); delete d; }

static bool sameAddr(const sockaddr_in& a, const sockaddr_in& b) {
    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

static void copyName(char* dst, const char* src) {
    memset(dst, 0, kNameLen + 1);
    if (src) strncpy(dst, src, kNameLen);
    for (int i = 0; i < kNameLen; i++) if ((unsigned char)dst[i] < 32) dst[i] = (dst[i] == 0) ? 0 : '?';
}

void Session::SetLocalName(const char* name) {
    copyName(d->name, name && *name ? name : "Player");
    if (mState == State::Hosting || mState == State::Connected) copyName(mRoster[mSlot].name, d->name);
    d->rosterDirty = true;
}


static bool sendRaw(Session* s, sock_t sock, const sockaddr_in& to, const Writer& w, uint32_t& cnt) {
    (void)s;
    int r = (int)sendto(sock, (const char*)w.buf, (int)w.n, 0, (const sockaddr*)&to, sizeof(to));
    if (r >= 0) cnt++;
    return r >= 0;
}

// Header: magic(4) version(1) type(1) slot(1) pad(1)
static void header(Writer& w, Type t, int slot) {
    w.u32(kMagic); w.u8(kVersion); w.u8(t); w.u8(slot); w.u8(0);
}

bool Session::Host(uint16_t port, const char* name) {
    Stop();
    std::string err;
    sockaddr_in a{};
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == kBadSock) { mStatus = "socket() failed"; mState = State::Failed; return false; }
    a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons(port);
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one));
    if (bind(s, (sockaddr*)&a, sizeof(a)) != 0 || !setNonBlocking(s)) {
        closeSock(s);
        mStatus = "Cannot open UDP port " + std::to_string(port) + " (already in use?)";
        mState = State::Failed;
        return false;
    }
    d->sock = s;
    mState = State::Hosting; mSlot = 0;
    for (auto& r : mRoster) r = RosterEntry();
    copyName(d->name, name && *name ? name : "Host");
    mRoster[0].used = true; copyName(mRoster[0].name, d->name);
    mStatus = "Hosting on UDP port " + std::to_string(port) + ". Waiting for players...";
    return true;
}

bool Session::Join(const char* host, uint16_t port, const char* name) {
    Stop();
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
    char ps[16]; snprintf(ps, sizeof(ps), "%u", port);
    if (getaddrinfo(host, ps, &hints, &res) != 0 || !res) {
        mStatus = std::string("Cannot resolve host '") + host + "'"; mState = State::Failed; return false;
    }
    d->hostAddr = *(sockaddr_in*)res->ai_addr;
    freeaddrinfo(res);
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = 0;
    if (s == kBadSock || bind(s, (sockaddr*)&a, sizeof(a)) != 0 || !setNonBlocking(s)) {
        if (s != kBadSock) closeSock(s);
        mStatus = "Cannot open UDP socket"; mState = State::Failed; return false;
    }
    d->sock = s;
    copyName(d->name, name && *name ? name : "Player");
    d->peers[0].used = true; d->peers[0].addr = d->hostAddr;
    d->lastHostRecv = nowMs();
    d->lastHello = 0;
    mState = State::Joining;
    mStatus = std::string("Connecting to ") + host + ":" + ps + "...";
    return true;
}

void Session::Stop() {
    if (d->sock != kBadSock) {
        Writer w; header(w, T_LEAVE, mSlot);
        if (mState == State::Connected || mState == State::Joining) {
            for (int i = 0; i < 3; i++) sendRaw(this, d->sock, d->hostAddr, w, packetsSent);
        } else if (mState == State::Hosting) {
            for (int s = 1; s < kMaxSlots; s++) if (d->peers[s].used)
                for (int i = 0; i < 3; i++) sendRaw(this, d->sock, d->peers[s].addr, w, packetsSent);
        }
        closeSock(d->sock);
        d->sock = kBadSock;
    }
    for (auto& p : d->peers) p = Peer();
    for (auto& r : d->rings) r.clear();
    for (auto& r : mRoster) r = RosterEntry();
    mState = State::Idle; mSlot = 0; mMatchActive = false; mReleased = false; mMatchMask = 0;
    d->pendingCfg = false; d->aborted = false; d->lingerUntil = 0; d->localReady = false;
    mStatus = "Idle";
}

int Session::ConnectedCount() const {
    int n = 0;
    for (auto& r : mRoster) if (r.used) n++;
    return n;
}

void Session::SetChoice(int fighter, int costume) {
    d->myFighter = (uint8_t)fighter; d->myCostume = (uint8_t)costume;
    if (mState == State::Hosting || mState == State::Connected) {
        mRoster[mSlot].fighter = d->myFighter; mRoster[mSlot].costume = d->myCostume;
    }
    d->rosterDirty = true;
}

void Session::SetInputDelay(int dl) { mDelay = std::max(1, std::min(15, dl)); }
void Session::SetMaxPlayers(int n) { mMaxPlayers = std::max(2, std::min(4, n)); mRosterDirtyFlag = true; }

bool Session::ConsumeAbort(std::string* reason) {
    if (!d->aborted) return false;
    d->aborted = false;
    if (reason) *reason = d->abortReason;
    return true;
}

// ---------------------------------------------------------------------------
// match control

bool Session::HostBeginMatch(const void* blob, int size) {
    if (mState != State::Hosting || size <= 0 || size > kMaxBlob) return false;
    int mask = 1;
    for (int s = 1; s < kMaxSlots; s++) if (d->peers[s].used) mask |= (1 << s);
    if (mask == 1) return false;
    d->matchId++;
    d->blob.assign((const uint8_t*)blob, (const uint8_t*)blob + size);
    d->cfgMask = (uint8_t)mask; d->cfgDelay = (uint8_t)mDelay;
    mMatchMask = mask; mMatchDelay = mDelay;
    for (auto& r : d->rings) r.clear();
    for (int s = 1; s < kMaxSlots; s++) { d->peers[s].configAcked = false; d->peers[s].ready = false; for (auto& a : d->peers[s].ack) a = 0; d->peers[s].lastConfigSend = 0; }
    d->localReady = false; mReleased = false; d->releasedAt = 0; d->startSentUntil = 0;
    d->matchStart = nowMs(); d->lingerUntil = 0; d->aborted = false;
    mMatchActive = true;
    mStatus = "Match starting...";
    return true;
}

int Session::ClientTakeMatch(void* out, int cap) {
    if (mState != State::Connected || !d->pendingCfg) return 0;
    if ((int)d->blob.size() > cap) return 0;
    d->pendingCfg = false;
    memcpy(out, d->blob.data(), d->blob.size());
    for (auto& r : d->rings) r.clear();
    for (auto& a : d->hostAck) a = 0;
    mMatchMask = d->cfgMask; mMatchDelay = d->cfgDelay;
    d->consumedId = d->matchId;
    d->localReady = false; mReleased = false;
    d->matchStart = nowMs(); d->lingerUntil = 0; d->aborted = false;
    mMatchActive = true;
    mStatus = "Match starting...";
    return (int)d->blob.size();
}

void Session::Ready() { d->localReady = true; }

void Session::EndMatch() {
    if (!mMatchActive) return;
    mMatchActive = false;
    d->lingerUntil = nowMs() + kLingerMs;
    d->localReady = false;
    if (mState == State::Hosting) mStatus = "Hosting. Waiting for players...";
    else if (mState == State::Connected) mStatus = "Connected to host.";
}

void Session::PutInput(int slot, uint32_t tick, InputFrame f) {
    if (slot < 0 || slot >= kMaxSlots) return;
    d->rings[slot].put(tick, f);
}

bool Session::GetInput(int slot, uint32_t tick, InputFrame* out) const {
    if (slot < 0 || slot >= kMaxSlots) return false;
    return d->rings[slot].get(tick, out);
}

// ---------------------------------------------------------------------------
// sending

static void writeInputBlocks(Session::Impl* d, Writer& w, int destSlot, int mySlot, bool isHost, const uint32_t peerAck[kMaxSlots], int mask) {
    // ack[4]: our "next needed" per slot
    for (int s = 0; s < kMaxSlots; s++) w.u32(d->rings[s].next);
    size_t countPos = w.n;
    w.u8(0);
    int blocks = 0;
    for (int s = 0; s < kMaxSlots; s++) {
        if (!(mask & (1 << s))) continue;
        if (isHost ? (s == destSlot) : (s != mySlot)) continue;
        uint32_t from = peerAck[s];
        uint32_t cnt = 0;
        while (cnt < (uint32_t)kMaxFramesPerBlock && d->rings[s].get(from + cnt, nullptr)) cnt++;
        if (cnt == 0) continue;
        w.u8(s); w.u32(from); w.u8(cnt);
        for (uint32_t i = 0; i < cnt; i++) {
            InputFrame f; d->rings[s].get(from + i, &f);
            w.u16(f.buttons); w.u8((uint8_t)f.stickX); w.u8((uint8_t)f.stickY);
        }
        blocks++;
    }
    w.buf[countPos] = (uint8_t)blocks;
}

// ---------------------------------------------------------------------------
// receiving

void Session::Poll() {
    if (d->sock == kBadSock) return;
    uint64_t now = nowMs();

    // ---- receive ----
    for (int guard = 0; guard < 256; guard++) {
        uint8_t buf[1500];
        sockaddr_in from{};
        socklen_compat fl = sizeof(from);
        int n = (int)recvfrom(d->sock, (char*)buf, sizeof(buf), 0, (sockaddr*)&from, &fl);
        if (n < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAECONNRESET) continue; // ICMP port unreachable; ignore
#else
            if (errno == ECONNREFUSED) continue;
#endif
            break;                                            // EWOULDBLOCK: nothing left to read
        }
        if (n < 8) continue;
        if (dropRate > 0.0f) {
            d->rng = d->rng * 1664525u + 1013904223u;
            if (((d->rng >> 8) & 0xFFFF) / 65536.0f < dropRate) continue;
        }
        packetsRecv++;
        Reader r(buf, n);
        if (r.u32() != kMagic) continue;
        uint8_t ver = (uint8_t)r.u8(); uint8_t type = (uint8_t)r.u8(); int slot = r.u8(); r.u8();
        if (slot >= kMaxSlots) continue;

        if (mState == State::Hosting) {
            if (ver != kVersion) {
                Writer w; header(w, T_REJECT, 0); w.u8(3);
                sendRaw(this, d->sock, from, w, packetsSent);
                continue;
            }
            if (type == T_HELLO) {
                char nm[kNameLen + 1] = {0}; r.bytes(nm, kNameLen);
                int found = -1;
                for (int s = 1; s < kMaxSlots; s++) if (d->peers[s].used && sameAddr(d->peers[s].addr, from)) found = s;
                if (found < 0) {
                    if (mMatchActive) { Writer w; header(w, T_REJECT, 0); w.u8(2); sendRaw(this, d->sock, from, w, packetsSent); continue; }
                    if (ConnectedCount() < mMaxPlayers)
                        for (int s = 1; s < kMaxSlots; s++) if (!d->peers[s].used) { found = s; break; }
                    if (found < 0) { Writer w; header(w, T_REJECT, 0); w.u8(1); sendRaw(this, d->sock, from, w, packetsSent); continue; }
                    Peer& p = d->peers[found];
                    p = Peer(); p.used = true; p.addr = from;
                    mRoster[found] = RosterEntry(); mRoster[found].used = true;
                    d->rosterDirty = true;
                }
                d->peers[found].lastRecv = now;
                copyName(nm[0] ? mRoster[found].name : mRoster[found].name, nm[0] ? nm : "Player");
                Writer w; header(w, T_WELCOME, 0); w.u8(found);
                sendRaw(this, d->sock, from, w, packetsSent);
                d->rosterDirty = true;
                continue;
            }
            // everything else must come from a known peer
            if (slot < 1 || !d->peers[slot].used || !sameAddr(d->peers[slot].addr, from)) continue;
            Peer& p = d->peers[slot];
            p.lastRecv = now;
            if (type == T_CHOICE) {
                uint8_t f = (uint8_t)r.u8(), c = (uint8_t)r.u8();
                char nm[kNameLen + 1] = {0}; r.bytes(nm, kNameLen);
                if (!r.bad) {
                    if (mRoster[slot].fighter != f || mRoster[slot].costume != c) d->rosterDirty = true;
                    mRoster[slot].fighter = f; mRoster[slot].costume = c;
                    if (nm[0]) copyName(mRoster[slot].name, nm);
                }
            } else if (type == T_LEAVE) {
                bool inMatch = mMatchActive;
                p = Peer(); mRoster[slot] = RosterEntry(); d->rosterDirty = true;
                if (inMatch) {
                    d->aborted = true; d->abortReason = "A player left the match.";
                    d->abortSends = 10; d->lastAbortSend = 0;
                    EndMatch();
                }
            } else if (type == T_CONFIG_ACK) {
                if ((uint32_t)r.u32() == d->matchId) p.configAcked = true;
            } else if (type == T_READY) {
                if ((uint32_t)r.u32() == d->matchId && mMatchActive) p.ready = true;
            } else if (type == T_INPUT && mMatchActive) {
                uint32_t ack[kMaxSlots];
                for (int s = 0; s < kMaxSlots; s++) ack[s] = r.u32();
                int blocks = r.u8();
                for (int b = 0; b < blocks && !r.bad; b++) {
                    int bs = r.u8(); uint32_t first = r.u32(); int cnt = r.u8();
                    for (int i = 0; i < cnt && !r.bad; i++) {
                        InputFrame f; f.buttons = (uint16_t)r.u16(); f.stickX = (int8_t)r.u8(); f.stickY = (int8_t)r.u8();
                        if (!r.bad && bs == slot) d->rings[bs].put(first + i, f); // host only trusts a peer for its own slot
                    }
                }
                if (!r.bad) for (int s = 0; s < kMaxSlots; s++) p.ack[s] = std::max(p.ack[s], ack[s]);
            }
        } else if (mState == State::Joining || mState == State::Connected) {
            if (!sameAddr(from, d->hostAddr)) continue;
            d->lastHostRecv = now;
            if (type == T_REJECT) {
                int why = r.u8();
                mStatus = why == 1 ? "Host is full (4 players)." : why == 2 ? "Match already in progress." : "Version mismatch: use the same build as the host.";
                mState = State::Failed;
                continue;
            }
            if (ver != kVersion) continue;
            if (type == T_WELCOME) {
                int s = r.u8();
                if (!r.bad && s >= 1 && s < kMaxSlots) {
                    mSlot = s;
                    if (mState == State::Joining) mStatus = "Connected to host. You are Player " + std::to_string(s + 1) + ".";
                    mState = State::Connected;
                }
            } else if (type == T_ROSTER && mState == State::Connected) {
                RosterEntry tmp[kMaxSlots];
                int rmode = r.u8(); int rmax = r.u8();
                if (!r.bad) { mMode = rmode; mMaxPlayers = rmax; }
                for (int s = 0; s < kMaxSlots; s++) {
                    tmp[s].used = r.u8() != 0; tmp[s].fighter = (uint8_t)r.u8(); tmp[s].costume = (uint8_t)r.u8();
                    r.bytes(tmp[s].name, kNameLen); tmp[s].name[kNameLen] = 0;
                }
                if (!r.bad) for (int s = 0; s < kMaxSlots; s++) { if (s == mSlot) { tmp[s].fighter = d->myFighter; tmp[s].costume = d->myCostume; } mRoster[s] = tmp[s]; }
            } else if (type == T_CONFIG && mState == State::Connected) {
                uint32_t id = r.u32(); int mask = r.u8(); int dl = r.u8(); int len = r.u16();
                if (!r.bad && len > 0 && len <= kMaxBlob) {
                    std::vector<uint8_t> b(len); r.bytes(b.data(), len);
                    if (!r.bad) {
                        if (id != d->consumedId && !mMatchActive && !(d->pendingCfg && id == d->matchId)) {
                            d->matchId = id; d->blob = b; d->cfgMask = (uint8_t)mask; d->cfgDelay = (uint8_t)dl; d->pendingCfg = true;
                        }
                        Writer w; header(w, T_CONFIG_ACK, mSlot); w.u32(id);
                        sendRaw(this, d->sock, d->hostAddr, w, packetsSent);
                    }
                }
            } else if (type == T_START) {
                if (mMatchActive && r.u32() == d->matchId) {
                    if (!mReleased) d->releasedAt = now;
                    mReleased = true;
                }
            } else if (type == T_INPUT && mMatchActive) {
                uint32_t ack[kMaxSlots];
                for (int s = 0; s < kMaxSlots; s++) ack[s] = r.u32();
                int blocks = r.u8();
                for (int b = 0; b < blocks && !r.bad; b++) {
                    int bs = r.u8(); uint32_t first = r.u32(); int cnt = r.u8();
                    for (int i = 0; i < cnt && !r.bad; i++) {
                        InputFrame f; f.buttons = (uint16_t)r.u16(); f.stickX = (int8_t)r.u8(); f.stickY = (int8_t)r.u8();
                        if (!r.bad && bs != mSlot) d->rings[bs].put(first + i, f);
                    }
                }
                if (!r.bad) for (int s = 0; s < kMaxSlots; s++) d->hostAck[s] = std::max(d->hostAck[s], ack[s]);
            } else if (type == T_ABORT) {
                d->aborted = true; d->abortReason = "The host ended the match.";
                EndMatch();
            }
        }
    }

    // ---- timers / sends ----
    if (mState == State::Joining || mState == State::Connected) {
        if (mState == State::Joining) {
            if (now - d->lastHello >= 500) {
                d->lastHello = now;
                Writer w; header(w, T_HELLO, 0); char nm[kNameLen]; memset(nm, 0, sizeof(nm)); memcpy(nm, d->name, kNameLen);
                w.bytes(nm, kNameLen);
                sendRaw(this, d->sock, d->hostAddr, w, packetsSent);
            }
            if (now - d->lastHostRecv > 8000) { mStatus = "No answer from host (check IP, port, firewall, VPN)."; mState = State::Failed; }
            return;
        }
        // Connected
        if (now - d->lastChoice >= kChoiceEveryMs) {
            d->lastChoice = now;
            Writer w; header(w, T_CHOICE, mSlot); w.u8(d->myFighter); w.u8(d->myCostume);
            char nm[kNameLen]; memset(nm, 0, sizeof(nm)); memcpy(nm, d->name, kNameLen); w.bytes(nm, kNameLen);
            sendRaw(this, d->sock, d->hostAddr, w, packetsSent);
        }
        if (mMatchActive) {
            if (d->localReady && !mReleased && now - d->lastReadySend >= 100) {
                d->lastReadySend = now;
                Writer w; header(w, T_READY, mSlot); w.u32(d->matchId);
                sendRaw(this, d->sock, d->hostAddr, w, packetsSent);
            }
            Writer w; header(w, T_INPUT, mSlot);
            writeInputBlocks(d, w, 0, mSlot, false, d->hostAck, mMatchMask);
            sendRaw(this, d->sock, d->hostAddr, w, packetsSent);
            uint64_t lim = mReleased ? kMatchTimeoutMs : kLoadTimeoutMs;
            if (now - d->lastHostRecv > lim) {
                d->aborted = true; d->abortReason = "Lost connection to the host.";
                EndMatch();
            }
        } else if (now - d->lastHostRecv > kLobbyTimeoutMs) {
            mStatus = "Lost connection to the host."; mState = State::Failed;
        }
        return;
    }

    if (mState != State::Hosting) return;

    // Hosting
    for (int s = 1; s < kMaxSlots; s++) {
        Peer& p = d->peers[s];
        if (!p.used) continue;
        if (!mMatchActive && now - p.lastRecv > kLobbyTimeoutMs) {
            p = Peer(); mRoster[s] = RosterEntry(); d->rosterDirty = true; continue;
        }
        if (mMatchActive && (mMatchMask & (1 << s))) {
            uint64_t lim = mReleased ? kMatchTimeoutMs : kLoadTimeoutMs;
            if (now - p.lastRecv > lim) {
                d->aborted = true; d->abortReason = "Lost connection to " + std::string(mRoster[s].name) + ".";
                d->abortSends = 10; d->lastAbortSend = 0;
                EndMatch();
            }
        }
    }
    if (d->rosterDirty || mRosterDirtyFlag || now - d->lastRoster >= kRosterEveryMs) {
        d->rosterDirty = false; mRosterDirtyFlag = false; d->lastRoster = now;
        Writer w; header(w, T_ROSTER, 0); w.u8(mMode); w.u8(mMaxPlayers);
        for (int s = 0; s < kMaxSlots; s++) {
            w.u8(mRoster[s].used); w.u8(mRoster[s].fighter); w.u8(mRoster[s].costume);
            char nm[kNameLen]; memset(nm, 0, sizeof(nm)); memcpy(nm, mRoster[s].name, kNameLen); w.bytes(nm, kNameLen);
        }
        for (int s = 1; s < kMaxSlots; s++) if (d->peers[s].used) sendRaw(this, d->sock, d->peers[s].addr, w, packetsSent);
    }
    if (mMatchActive) {
        // config resend
        for (int s = 1; s < kMaxSlots; s++) {
            Peer& p = d->peers[s];
            if (!p.used || !(mMatchMask & (1 << s)) || p.configAcked) continue;
            if (now - p.lastConfigSend < kResendEveryMs) continue;
            p.lastConfigSend = now;
            Writer w; header(w, T_CONFIG, 0); w.u32(d->matchId); w.u8(d->cfgMask); w.u8(d->cfgDelay); w.u16((uint32_t)d->blob.size());
            w.bytes(d->blob.data(), d->blob.size());
            sendRaw(this, d->sock, p.addr, w, packetsSent);
        }
        // barrier
        if (!mReleased && d->localReady) {
            bool all = true;
            for (int s = 1; s < kMaxSlots; s++) if ((mMatchMask & (1 << s)) && !d->peers[s].ready) all = false;
            if (all) { mReleased = true; d->releasedAt = now; d->startSentUntil = now + 1500; mStatus = "Match running."; }
        }
        if (!mReleased && now - d->matchStart > kLoadTimeoutMs) {
            d->aborted = true; d->abortReason = "Timed out waiting for all players to load.";
            d->abortSends = 10; d->lastAbortSend = 0;
            EndMatch();
        }
        if (mReleased && now < d->startSentUntil && now - d->lastStartSend >= 50) {
            d->lastStartSend = now;
            Writer w; header(w, T_START, 0); w.u32(d->matchId);
            for (int s = 1; s < kMaxSlots; s++) if (d->peers[s].used && (mMatchMask & (1 << s))) sendRaw(this, d->sock, d->peers[s].addr, w, packetsSent);
        }
    }
    if (mMatchActive || now < d->lingerUntil) {
        for (int s = 1; s < kMaxSlots; s++) {
            Peer& p = d->peers[s];
            if (!p.used || !(mMatchMask & (1 << s)) || !p.configAcked) continue;
            Writer w; header(w, T_INPUT, 0);
            writeInputBlocks(d, w, s, 0, true, p.ack, mMatchMask);
            sendRaw(this, d->sock, p.addr, w, packetsSent);
        }
    }
    if (d->abortSends > 0 && now - d->lastAbortSend >= 50) {
        d->abortSends--; d->lastAbortSend = now;
        Writer w; header(w, T_ABORT, 0);
        for (int s = 1; s < kMaxSlots; s++) if (d->peers[s].used) sendRaw(this, d->sock, d->peers[s].addr, w, packetsSent);
    }
}

std::string Session::DebugString() const {
    char b[512];
    snprintf(b, sizeof(b), "slot=%d next=[%u %u %u %u] hostAck=[%u %u %u %u] peerAck1=[%u %u %u %u] cfgAck=%d%d%d",
             mSlot, d->rings[0].next, d->rings[1].next, d->rings[2].next, d->rings[3].next,
             d->hostAck[0], d->hostAck[1], d->hostAck[2], d->hostAck[3],
             d->peers[1].ack[0], d->peers[1].ack[1], d->peers[1].ack[2], d->peers[1].ack[3],
             d->peers[1].configAcked, d->peers[2].configAcked, d->peers[3].configAcked);
    return b;
}

} // namespace ssb64net
