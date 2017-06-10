// Mesh operations

#include "container.h"
#include "nrfmac.h"
#include "trickle.h"
#include <iostream>
#include <stdint.h>
#include <math.h>
#include <string.h>

namespace kvmesh {
auto init() -> void { trickle_setup(200, 2048, 1); }

// check packet, returning same memory as *buf
bool check_packet(uint8_t *buf, size_t len) {
  auto payload = reinterpret_cast<mesh_payload *>(buf);

  // validate packet
  if (payload->len != len) {
    return false;
  }
  if (payload->key.len + sizeof(mesh_payload) > len) {
    return false;
  }
  // maybe check for zeroes in key

  return true;
}

// Compare *busted to *hotness assuming keys match
// Return 0 if consistent, -1 if busted should be kept, 1 if hotness should replace busted.
// simple version-only compare for now, without wraparound or slack
int consistent(mesh_payload *busted, mesh_payload *hotness) {
  if(busted->version < hotness->version) { // with wraparound & tolerance please
    return 1;
  } else if(busted->version > hotness->version) {
    return -1;
  }
  return 0;
}

// copy packet into system
// XXX 'consistent' function callback...
bool rx(mesh_payload *packet, uint32_t time_now) {
  Contained **existing = c_find(&(packet->key));

  auto element = reinterpret_cast<Contained *>(malloc(packet->len));
  // XXX handle failed malloc
  memcpy(&(element->payload), packet, packet->len);

  if(*existing == nullptr) {
    // add new thing
    trickle_timer_reset(element->get_trickle(), time_now); // XXX correct for new?
    if(!c_add(element)) {
      free(element);  // probably was full
      // XXX other strategies for dealing with full, e.g. delete oldest
    }
  } else {  // delete old?
    int consistency = consistent(&((*existing)->payload), packet);
    if (consistency == 0) { // they are the same
      free(element);
      trickle_rx_consistent((*existing)->get_trickle(), time_now);
    } else if (consistency == -1) { // old one is better
      trickle_rx_inconsistent((*existing)->get_trickle(), time_now);
    } else {  // new one is better
      auto old = *existing; // pointer overwritten by c_replace
      element->trickle = *(*existing)->get_trickle();
      trickle_rx_inconsistent(element->get_trickle(), time_now);
      c_replace(existing, element);
      free(old);
    }
  }

  return true;
}

}

int main(int argc, char **argv) {
  kvmesh::init();

  uint8_t buf[32];
  mesh_payload *payload = reinterpret_cast<mesh_payload *>(buf);

  std::cout << "Zero payload OK? " << kvmesh::check_packet(buf, sizeof(mesh_payload))
            << "\n";
  payload->len = sizeof(mesh_payload);
  std::cout << "With empty key OK? " << kvmesh::check_packet(buf, sizeof(mesh_payload))
            << "\n";

  return 0;
}