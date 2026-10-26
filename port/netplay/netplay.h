/* C API over ssb64net::Session, callable from the decomp C code and the ImGui menu. */
#ifndef PORT_NETPLAY_H
#define PORT_NETPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NP_MAX_BLOB 512

enum { NP_IDLE = 0, NP_HOSTING, NP_JOINING, NP_CONNECTED, NP_FAILED };

int np_host(uint16_t port, const char *name);
int np_join(const char *host, uint16_t port, const char *name);
void np_stop(void);
void np_poll(void);                          /* once per frame */

int np_state(void);
int np_is_host(void);
int np_local_slot(void);
const char *np_status(void);
int np_slot_used(int slot);
const char *np_slot_name(int slot);
void np_slot_choice(int slot, int *fighter, int *costume);
int np_player_count(void);
void np_set_choice(int fighter, int costume);
void np_set_name(const char *name);
void np_set_input_delay(int frames);
int np_get_input_delay(void);

int np_host_begin_match(const void *blob, int size);
int np_client_take_match(void *out, int cap);
int np_match_active(void);
int np_match_mask(void);
int np_match_delay(void);
void np_ready(void);
int np_start_released(void);
void np_end_match(void);
int np_consume_abort(void);                  /* 1 once after an abort */
const char *np_abort_reason(void);

void np_put_input(int slot, uint32_t tick, uint16_t buttons, int8_t sx, int8_t sy);
int np_get_input(int slot, uint32_t tick, uint16_t *buttons, int8_t *sx, int8_t *sy);

#ifdef __cplusplus
}
#endif
#endif
