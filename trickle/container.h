/// Container for mesh values and trickle instances

#pragma once

#include "trickle.h"

#ifndef __packed_gcc
#define __packed_gcc __attribute__((packed))
#endif

/**
 * Compare a wrapping timer
 */
inline bool timer_older_than(uint32_t time, uint32_t ref) {
  // essentially, is the result negative?
  // true if time is older than ref
  return (time - ref) > UINT32_MAX / 2;
}

// A short counted string
typedef struct p_string {
  uint8_t len;
  char value[];

  bool operator==(const struct p_string &rhs);
  void operator=(const char *c_string);
} p_string;


struct mesh_payload {
  uint8_t len; // p_string all, overlap?
  uint16_t version;
  p_string key;
} __packed_gcc;

// will be malloc() allocated
struct Contained {
  uint32_t index; // index inside the heap. could be a byte.
  trickle_t trickle;
  struct mesh_payload payload;

  uint32_t get_priority() { return this->trickle.t; }
  trickle_t *get_trickle() { return &this->trickle; }

  void set_priority(uint32_t priority) { this->trickle.t = priority; }

  p_string *get_key() const { return (p_string *)&(this->payload.key); }
} __packed_gcc;

Contained **c_find(p_string *key);
Contained *c_find(Contained *element);
bool c_add(Contained *element);
bool c_remove(Contained *element);
bool c_remove(Contained **it);

// replace given an iterator and the replacement
void c_replace(Contained **existing, Contained *replacement);

// but heap operations happen on Container