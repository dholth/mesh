/**
 * Packet radio abstraction.
 */

#pragma once

#include <iostream>
#include <stdint.h>

using namespace std;

class Radio {
public:
  uint32_t now;
  void ptx() { cout << now << " ptx\n"; };
  void prx() { cout << now << " prx\n"; };
  void idle() { cout << now << " idle\n"; }
  void off() { cout << now << " off\n"; }
  void listen() {
    cout << now << " listen\n";
  } // or 'check for packet', see also interrupts
  // enqueue, transmit_now, reuse tx payload?
  void transmit(const uint8_t buffer[], uint32_t len) {
    cout << now << " tx\n";
  }
  uint8_t status() {
    cout << now << " status\n";
    return 0;
  }
};
