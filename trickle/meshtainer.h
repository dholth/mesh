/// Classy trickle mesh container
/// Key-Value store plus reorderable priority queue

#include "container.h"

#include <array>
#include <queue>
#include <stdint.h>

template <uint8_t CAPACITY> class Meshtainer {
  /// Compare a wrapping timer
  inline bool timer_older_than(uint32_t time, uint32_t ref) {
    // essentially, is the result negative?
    // true if time is older than ref
    return (time - ref) > UINT32_MAX / 2;
  }

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

  class Container {
  public:
    Contained *value;

    Container(){};
    Container(Contained *v) : value(v){};

    Container &operator=(const Container &other);
  };

  // static_vector would be quite useful
  // note pointers are twice as long on the laptop
  std::array<uint32_t, CAPACITY> by_priority;
  std::array<uint32_t *, CAPACITY> by_key;
  int fill = 0;
};
