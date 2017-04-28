
typedef struct
{
    uint32_t        t;
    uint32_t        i;
    uint8_t         i_doublings;
    uint8_t         c;
}  trickle_t;

void trickle_setup(uint32_t i_min, uint32_t i_max, uint8_t k);
void trickle_rx_consistent(trickle_t* id, uint32_t time_now);
void trickle_rx_inconsistent(trickle_t* id, uint32_t time_now);
void trickle_timer_reset(trickle_t* trickle, uint32_t time_now);
void trickle_tx_register(trickle_t* trickle, uint32_t time_now);
void trickle_tx_timeout(trickle_t* trickle, _Bool* out_do_tx, uint32_t time_now);
void trickle_disable(trickle_t* trickle);
void trickle_enable(trickle_t* trickle);
_Bool trickle_is_enabled(trickle_t* trickle);
