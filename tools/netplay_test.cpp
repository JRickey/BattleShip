// Loopback test: 1 host + 3 clients in one process, lockstep with packet loss.
// Build: g++ -std=c++17 -I../port/netplay netplay_test.cpp ../port/netplay/netplay_session.cpp -o netplay_test
#include "netplay_session.hpp"
#include <chrono>
#include <cstdio>
#include <thread>
using namespace ssb64net;

static uint32_t h(int slot, uint32_t t) { uint32_t x = (t + 1) * 2654435761u ^ (slot * 40503u); x ^= x >> 13; return x; }

int main(int argc, char** argv) {
    float loss = argc > 1 ? (float)atof(argv[1]) : 0.2f;
    const int N = 4, D = 5, TICKS = 1500;
    Session s[N];
    s[0].Host(47777, "host");
    for (int i = 1; i < N; i++) { s[i].Join("127.0.0.1", 47777, "cli"); s[i].SetChoice(i, i); }
    for (auto& x : s) x.dropRate = loss;
    auto pump = [&] { for (auto& x : s) x.Poll(); std::this_thread::sleep_for(std::chrono::milliseconds(2)); };
    for (int i = 0; i < 5000; i++) { pump(); if (s[0].ConnectedCount() == 4 && s[1].GetState() == State::Connected && s[2].GetState() == State::Connected && s[3].GetState() == State::Connected && s[0].Roster(s[1].LocalSlot()).fighter == 1 && s[0].Roster(s[2].LocalSlot()).fighter == 2 && s[0].Roster(s[3].LocalSlot()).fighter == 3) break; }
    printf("lobby: %d players; client slots %d %d %d\n", s[0].ConnectedCount(), s[1].LocalSlot(), s[2].LocalSlot(), s[3].LocalSlot());
    if (s[0].ConnectedCount() != 4 || s[0].Roster(s[3].LocalSlot()).fighter != 3 || s[0].Roster(s[1].LocalSlot()).fighter != 1) { puts("FAIL lobby"); return 1; }
    s[0].SetInputDelay(D);
    const char blob[] = "match-config";
    if (!s[0].HostBeginMatch(blob, sizeof(blob))) { puts("FAIL begin"); return 1; }
    char out[64];
    for (int i = 1; i < N; i++) { int n = 0; for (int k = 0; k < 400 && !(n = s[i].ClientTakeMatch(out, 64)); k++) pump(); if (n != sizeof(blob)) { puts("FAIL cfg"); return 1; } }
    for (auto& x : s) { x.Ready(); for (int sl = 0; sl < N; sl++) for (int t = 0; t < D; t++) if (x.LocalSlot() == sl) x.PutInput(sl, t, {}); }
    for (int i = 0; i < 400 && !(s[0].StartReleased() && s[1].StartReleased() && s[2].StartReleased() && s[3].StartReleased()); i++) pump();
    if (!s[3].StartReleased()) { puts("FAIL barrier"); return 1; }
    uint32_t tick[N] = {0}; uint64_t sum[N] = {0}; int stalls = 0;
    for (int it = 0; it < 200000; it++) {
        bool done = true;
        for (int i = 0; i < N; i++) {
            if (tick[i] >= TICKS) continue;
            done = false;
            bool ok = true; InputFrame f[N];
            for (int sl = 0; sl < N; sl++) if (!s[i].GetInput(sl, tick[i], &f[sl])) ok = false;
            if (!ok) { stalls++; continue; }
            for (int sl = 0; sl < N; sl++) sum[i] = sum[i] * 31 + f[sl].buttons + f[sl].stickX * 7;
            uint32_t v = h(i, tick[i] + D);
            s[i].PutInput(s[i].LocalSlot(), tick[i] + D, {(uint16_t)v, (int8_t)(v >> 16), (int8_t)(v >> 24)});
            tick[i]++;
        }
        pump();
        if (done) break;
    }
    printf("ticks %u %u %u %u stalls=%d\n", tick[0], tick[1], tick[2], tick[3], stalls);
    for (int i = 1; i < N; i++) if (sum[i] != sum[0]) { puts("FAIL desync"); return 1; }
    if (tick[0] < TICKS) { puts("FAIL incomplete"); return 1; }
    puts("PASS");
    return 0;
}
