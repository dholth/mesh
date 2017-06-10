#include <algorithm>
#include <array>
#include <iostream>
#include <queue>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility>

#include "container.h"

// Maximum number of keys in system
#define NFOOBARS (32)

/**
 * Priority queue and string-keyed mapping.
 * For efficient reprioritization.
 * Operations:
 * - Get highest priority item
 * - Get by string key
 * - Reprioritize item
 */

bool operator<(const p_string &l, const p_string &r) {
  auto cmp = strncmp(l.value, r.value, std::min(l.len, r.len));
  if (cmp == 0) {
    return l.len < r.len;
  }
  return cmp < 0;
}

std::ostream &operator<<(std::ostream &os, p_string const &m) {
  os << "(";
  return os.write(m.value, m.len) << ")";
}

std::ostream &operator<<(std::ostream &os, p_string const *m) {
  return os << *m;
}

void p_string::operator=(const char *c_string) {
  strcpy(this->value, c_string);
  this->len = strlen(c_string);
}

bool p_string::operator==(const p_string &rhs) {
  return this->len == rhs.len &&
         strncmp(this->value, rhs.value, this->len) == 0;
}

class Container {
public:
  Contained *value;

  Container(){};
  Container(Contained *v) : value(v){};

  Container &operator=(const Container &other);
};

struct LessThanByPriority {
  bool operator()(Container const &lhs, Container const &rhs) {
    if (lhs.value != NULL && rhs.value != NULL) {
      return timer_older_than(rhs.value->get_priority(),
                              lhs.value->get_priority());
    }
    return lhs.value != NULL;
  }
};

struct LessThanString {
  bool operator()(Contained const *lhs, Contained const *rhs) {
    return *lhs->get_key() < *rhs->get_key();
  }

  bool operator()(Contained const *lhs, const p_string *rhs) {
    return *lhs->get_key() < *rhs;
  }

  bool operator()(const p_string *lhs, Contained const *rhs) {
    return *lhs < *rhs->get_key();
  }
};

// static_vector would be quite useful
std::array<Container, NFOOBARS> foobars;
std::array<Contained *, NFOOBARS> foobars_by_key;
int fill = 0;

Container &Container::operator=(const Container &other) {
  this->value = other.value;
  if (this->value != nullptr) {
    this->value->index = std::distance(foobars.begin(), this);
  }
  return *this;
}

void print_it() {
  for (int i = 0; i < fill; i++) {
    auto value = foobars[i].value;
    printf("i %2d ti %2u pri %2d ", i, value->index, value->get_priority());
    std::cout << value->get_key() << std::endl;
  }
  printf("\n");
}

void print_fbk() {
  printf("By-key 0 to %d\n", fill - 1);
  for (int i = 0; i < fill; i++) {
    std::cout << i << " "
              << " " << foobars_by_key[i]->get_key() << std::endl;
  }
}

Contained **c_find(p_string *key) {
  Contained **begin = foobars_by_key.begin();
  auto end = begin + fill;
  auto it = std::lower_bound(begin, end, key, LessThanString());

  if (fill && (it != end)) {
    if (*(*it)->get_key() == *key) {
      return it;
    }
  }

  return nullptr;
}

/// Find with key matching element or nullptr if not found
Contained *c_find(Contained *element) { return *c_find(element->get_key()); }

/*
 * Insert element into container, maintaining heap, sorted invariants
 */
bool c_add(Contained *element) {
  if (fill == foobars.size()) {
    return false;
  }

  // here we might check that it is not already in there...

  // TODO this tries to compare nulls. Check for empty first?
  // Returns an iterator pointing to the first element in the range [first,
  // last) that
  // is not less than (i.e. greater or equal to) value.

  // Iterator pointing to the first element that is not less than value,
  // or last if no such element is found.
  Contained **begin = foobars_by_key.begin();
  auto end = begin + fill;
  auto it = std::lower_bound(begin, end, element, LessThanString());

  if (fill && (it != end)) { // ==begin might be okay
    if (*(*it)->get_key() == *element->get_key()) {
      std::cout << "Skip equal " << element->get_key() << std::endl;
      return false;
    }
  }

  std::move_backward(it, end, end + 1);
  *it = element;

  // If we know it's new, append to heap
  // Otherwise we might need to reprioritize an existing element
  // element->index = fill;
  foobars[fill] = Container(element);
  std::push_heap(foobars.begin(), foobars.begin() + fill + 1,
                 LessThanByPriority());

  fill++;

  return true;
}

bool c_remove(Contained **it) {
  // assert element in container...
  std::pop_heap(foobars.begin() + (*it)->index, foobars.begin() + fill,
                LessThanByPriority());

  auto begin = foobars_by_key.begin();
  auto end = begin + fill;
  // XXX assert *it in array
  std::move_backward(it + 1, end, it);
  fill--;
  return true;
}

bool c_remove(Contained *element) {
  auto begin = foobars_by_key.begin();
  auto end = begin + fill;
  auto it = std::lower_bound(begin, end, element, LessThanString());

  return c_remove(it);
}

void c_replace(Contained **existing, Contained *replacement) {
  auto index = (*existing)->index;
  *existing = replacement;
  // updates replacement->index as a side effect:
  foobars[index] = Container(replacement);
}

// Adjust an out-of-order element that knows its index inside foobars[]
// so that it is back where it belongs in our heap.
void reprioritize(Contained *c) {
  auto begin = foobars.begin() + c->index;
  printf("OOO pri:\n");
  print_it();
  std::pop_heap(begin, foobars.begin() + fill, LessThanByPriority());
  printf("Pop pri:\n");
  print_it();
  std::push_heap(foobars.begin(), foobars.begin() + fill, LessThanByPriority());
  printf("New pri:\n");
  print_it();
}
