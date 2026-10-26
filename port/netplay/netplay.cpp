// SPDX-License-Identifier: MIT
#include "netplay.h"
#include "netplay_session.hpp"

#include <string>

using namespace ssb64net;

static Session& S() {
    static Session s;
    return s;
}
static std::string sAbortReason;

extern "C" {

int np_host(uint16_t port, const char *name) { return S().Host(port, name) ? 1 : 0; }
int np_join(const char *host, uint16_t port, const char *name) { return S().Join(host, port, name) ? 1 : 0; }
void np_stop(void) { S().Stop(); }
void np_poll(void) { S().Poll(); }

int np_state(void) {
    switch (S().GetState()) {
        case State::Hosting: return NP_HOSTING;
        case State::Joining: return NP_JOINING;
        case State::Connected: return NP_CONNECTED;
        case State::Failed: return NP_FAILED;
        default: return NP_IDLE;
    }
}
int np_is_host(void) { return S().IsHost() ? 1 : 0; }
int np_local_slot(void) { return S().LocalSlot(); }
const char *np_status(void) { return S().Status(); }
int np_slot_used(int slot) { return (slot >= 0 && slot < kMaxSlots && S().Roster(slot).used) ? 1 : 0; }
const char *np_slot_name(int slot) { return (slot >= 0 && slot < kMaxSlots) ? S().Roster(slot).name : ""; }
void np_slot_choice(int slot, int *f, int *c) {
    if (slot < 0 || slot >= kMaxSlots) return;
    if (f) *f = S().Roster(slot).fighter;
    if (c) *c = S().Roster(slot).costume;
}
int np_player_count(void) { return S().ConnectedCount(); }
void np_set_choice(int f, int c) { S().SetChoice(f, c); }
void np_set_name(const char *n) { S().SetLocalName(n); }
void np_set_input_delay(int f) { S().SetInputDelay(f); }
int np_get_input_delay(void) { return S().InputDelay(); }
void np_set_mode(int m) { S().SetMode(m); }
int np_get_mode(void) { return S().Mode(); }
void np_set_max_players(int n) { S().SetMaxPlayers(n); }
int np_get_max_players(void) { return S().MaxPlayers(); }

static int sRunDifficulty = 2; // Normal
static int sRunStocks = 2;     // 3 lives
void np_set_run_options(int difficulty, int stock_count) {
    sRunDifficulty = difficulty < 0 ? 0 : (difficulty > 4 ? 4 : difficulty);
    sRunStocks = stock_count < 0 ? 0 : (stock_count > 4 ? 4 : stock_count);
}
int np_get_run_difficulty(void) { return sRunDifficulty; }
int np_get_run_stocks(void) { return sRunStocks; }

int np_host_begin_match(const void *b, int n) { return S().HostBeginMatch(b, n) ? 1 : 0; }
int np_client_take_match(void *out, int cap) { return S().ClientTakeMatch(out, cap); }
int np_match_active(void) { return S().MatchActive() ? 1 : 0; }
int np_match_mask(void) { return S().MatchMask(); }
int np_match_delay(void) { return S().MatchDelay(); }
void np_ready(void) { S().Ready(); }
int np_start_released(void) { return S().StartReleased() ? 1 : 0; }
void np_end_match(void) { S().EndMatch(); }
int np_consume_abort(void) { return S().ConsumeAbort(&sAbortReason) ? 1 : 0; }
const char *np_abort_reason(void) { return sAbortReason.c_str(); }

void np_put_input(int slot, uint32_t tick, uint16_t b, int8_t sx, int8_t sy) {
    InputFrame f; f.buttons = b; f.stickX = sx; f.stickY = sy;
    S().PutInput(slot, tick, f);
}
int np_get_input(int slot, uint32_t tick, uint16_t *b, int8_t *sx, int8_t *sy) {
    InputFrame f;
    if (!S().GetInput(slot, tick, &f)) return 0;
    if (b) *b = f.buttons;
    if (sx) *sx = f.stickX;
    if (sy) *sy = f.stickY;
    return 1;
}

} // extern "C"
