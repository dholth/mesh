#include "nrfmac.h"

void MAC::tick(uint32_t time_now) {
  if (_last_tick == time_now) {
    cout << "double tick\n";
    return;
  }
  _last_tick = time_now;

  // given the Halt event, transition to a new state based upon
  // the current state of the state machine
  BEGIN_TRANSITION_MAP()        // - Current State -
  TRANSITION_MAP_ENTRY(ST_IDLE) // ST_Idle
  TRANSITION_MAP_ENTRY(ST_RX)   // ST_Rx
  TRANSITION_MAP_ENTRY(ST_TX)   // ST_Tx
  END_TRANSITION_MAP(time_now)
}

void MAC::ST_Idle(uint32_t pData) {
  if (!timer_older_than(pData, _timeout)) {
    if (this->txne()) {
      // transmit for entire time slot...
      // check that we will begin rx at next time slot...
      _timeout = TICKS_PER_PERIOD * ((pData / TICKS_PER_PERIOD) + 1);
      radio.ptx();
      InternalEvent(ST_TX, pData);
    } else {
      // enter rx state... turn on radio etc.
      _timeout = pData + RX_TICKS;
      radio.prx();
      InternalEvent(ST_RX, pData);
    }
  }
  // if queued tx and tick_slow
  // _timeout = (uint32_t)pData + TX_TICKS
  // request fast ticks
  // (1037 - (1037%256) + 256)
  // InternalEvent(ST_TX, pData); // could set timeout here
  // if timer % divisor == 0
}

void MAC::ST_Tx(uint32_t pData) {
  if (!timer_older_than(pData, _timeout)) {
    radio.off();
    buffer_len = 0; // clear_buffer() ?
    InternalEvent(ST_IDLE, pData);
  } else {
    radio.transmit(buffer, buffer_len);
  }
}

void MAC::ST_Rx(uint32_t pData) {
  // skip if we just turned on?
  // if interrupts, check only at end? or not at all?
  radio.listen();
  // if we got a packet, read into a buffer and go to idle
  if (!timer_older_than(pData, _timeout)) {
    // turn off radio
    // we could only check for rx packet here, plus irq
    radio.off();
    cout << _timeout << " next ";
    _timeout = TICKS_PER_PERIOD * ((pData / TICKS_PER_PERIOD) + 1);
    cout << _timeout << "\n";
    InternalEvent(ST_IDLE, pData);
  }
}

void MAC::broadcast(void *data, uint32_t len) { buffer_len = len; }

int main(int argc, char **argv) {
  auto r = Radio();
  auto m = MAC(r);

  for (int i = 0; i < 1024; i++) {
    r.now = i;
    m.tick(i);
    if (i == 128) {
      m.broadcast(nullptr, 31);
    }
    auto pData = i;
    auto _timeout =
        MAC::TICKS_PER_PERIOD * ((pData / MAC::TICKS_PER_PERIOD) + 1);
    // cout << pData << " " << _timeout << " pData older than _timeout? " <<
    // m.timer_older_than(pData, _timeout) << "\n";
  }

  return 0;
}