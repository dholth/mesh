/**
 * Event driven sleepy MAC for nRF24.
 *
 * Wake every period.
 * Listen a little.
 * Sleep.
 * Take over certain processor resources.
 */

#pragma once

#include <stdint.h>

#include "radio.h"
#include "state/StateMachine.h"

class MAC : public StateMachine {

public:
  // Parameters for the MAC
  enum {
    PACKET_US = 500, // this long to transmit
    SILENT_US = 500, // wait this long afterwards
    STROBE_HZ = 256,
    LISTEN_HZ = 8,
    F_TICK = 256,
    MTU = 32,
    TICKS_PER_PERIOD = F_TICK / LISTEN_HZ,
    RX_TICKS = 3 // at least 3x packet transmission time

    // SSR resolution is 1/256s by default...
    // PREDIV_A = 0x7f, PREDIV_S = 0xff
  } mac_config;

private:
  uint8_t strobe_count;
  // maybe a packet buffer. how about two? static_vector?
  uint8_t buffer[MTU];
  uint8_t buffer_len;
  uint32_t _timeout;
  uint32_t _last_tick;

  Radio &radio;

public:
  MAC(Radio &radio)
      : StateMachine(ST_MAX_STATES), _timeout(0), _last_tick(-1),
        radio(radio){};

  // just enqueue, not an event
  void broadcast(void *data, uint32_t len); // MTU of 32 bytes expected

  // called every timer wakeup
  // main external event
  // way to request fast or slow tick?
  void tick(uint32_t time_now); // in binary milliseconds that wrap

private:
  // Maybe just a uint32_t...
  void ST_Idle(uint32_t);
  void ST_Rx(uint32_t);
  void ST_Tx(uint32_t);

  BEGIN_STATE_MAP
  STATE_MAP_ENTRY(&MAC::ST_Idle)
  STATE_MAP_ENTRY(&MAC::ST_Rx)
  // transfer_rx ~256 cycles 32*8 or 1228 cycles @ 48Mhz?
  STATE_MAP_ENTRY(&MAC::ST_Tx)
  // transfer_tx
  END_STATE_MAP

public:
  /**
   * Compare a wrapping timer
   */
  bool timer_older_than(uint32_t time, uint32_t ref) {
    // essentially, is the result negative?
    // true if time is older than ref
    return (time - ref) > UINT32_MAX / 2;
  }

  bool txne() { return buffer_len != 0; }

  // state enumeration order must match the order of state
  // method entries in the state map
  enum E_States {
    ST_IDLE = 0,
    ST_RX,
    ST_TX,
    // enter_tx, repeat_tx, stop_tx, enter_rx, wait_rx, ...
    ST_MAX_STATES
  };
};