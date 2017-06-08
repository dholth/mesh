#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <utility>
#include <array>
#include <queue>
#include <algorithm>
#include <iostream>

#include "trickle.h"

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

/**
 * Compare a wrapping timer
 */
bool timer_older_than(uint32_t time, uint32_t ref) {
    // essentially, is the result negative?
    // true if time is older than ref
    return time < ref;
    // return (time - ref) > UINT32_MAX / 2;
}

// A short counted string
typedef struct p_string
{
    uint8_t len;
    char value[];

    bool operator==(const struct p_string &rhs);
    void operator=(const char *c_string);
} p_string;

bool operator<(const p_string &l, const p_string &r)
{
    auto cmp = strncmp(l.value, r.value, std::min(l.len, r.len));
    if (cmp == 0)
    {
        return l.len < r.len;
    }
    return cmp < 0;
}

std::ostream &operator<<(std::ostream &os, p_string const &m)
{
    os << "(";
    return os.write(m.value, m.len) << ")";
}

std::ostream &operator<<(std::ostream &os, p_string const *m)
{
    return os << *m;
}

void p_string::operator=(const char *c_string)
{
    strcpy(this->value, c_string);
    this->len = strlen(c_string);
}

bool p_string::operator==(const p_string &rhs)
{
    return this->len == rhs.len && strncmp(this->value, rhs.value, this->len) == 0;
}

struct mesh_payload
{
    uint8_t len; // p_string all, overlap?
    uint16_t version;
    p_string key;
} __packed_gcc;

// will be malloc() allocated
struct Contained
{
    uint32_t index; // index inside the heap. could be a byte.
    trickle_t trickle;
    struct mesh_payload payload;

    uint32_t get_priority()
    {
        return this->trickle.t;
    }

    uint32_t pri()
    {
        return this->get_priority();
    }

    void set_priority(uint32_t priority)
    {
        this->trickle.t = priority;
    }

    p_string *get_key() const
    {
        return (p_string *)&(this->payload.key);
    }
} __packed_gcc;

class Container
{
  public:
    Contained *value;

    Container(){};
    Container(Contained *v) : value(v){};

    Container &operator=(const Container &other);
};

// TODO use a lambda
struct LessThanByPriority
{
    bool operator()(Container const &lhs, Container const &rhs)
    {
        if (lhs.value != NULL && rhs.value != NULL)
        {
            return lhs.value->get_priority() > rhs.value->get_priority();
        }
        return lhs.value != NULL;
    }
};

// TODO use a lambda
struct LessThanString
{
    bool operator()(Contained const *lhs, Contained const *rhs)
    {
        return *lhs->get_key() < *rhs->get_key();
    }
};

// static_vector would be quite useful
std::array<Container, NFOOBARS> foobars;
std::array<Contained *, NFOOBARS> foobars_by_key;
int fill = 0;

Container &Container::operator=(const Container &other)
{
    this->value = other.value;
    if (this->value != nullptr)
    {
        this->value->index = std::distance(foobars.begin(), this);
    }
    return *this;
}

void print_it()
{
    for (int i = 0; i < fill; i++)
    {
        auto value = foobars[i].value;
        printf("i %2d ti %2u pri %2d ", i, value->index, value->get_priority());
        std::cout << value->get_key() << std::endl;
    }
    printf("\n");
}

void print_fbk()
{
    printf("By-key 0 to %d\n", fill - 1);
    for (int i = 0; i < fill; i++)
    {
        std::cout << i << " "
                  << " " << foobars_by_key[i]->get_key() << std::endl;
    }
}

/*
 * Insert element into container, maintaining heap, sorted invariants
 */
bool c_add(Contained *element)
{
    if (fill == foobars.size())
    {
        return false;
    }

    // here we might check that it is not already in there...

    // TODO this tries to compare nulls. Check for empty first?
    // Returns an iterator pointing to the first element in the range [first, last) that
    // is not less than (i.e. greater or equal to) value.

    // Iterator pointing to the first element that is not less than value,
    // or last if no such element is found.
    auto begin = foobars_by_key.begin();
    auto end = begin + fill;
    auto it = std::lower_bound(begin, end, element, LessThanString());

    if (fill && (it != end))
    { // ==begin might be okay
        if (*(*it)->get_key() == *element->get_key())
        {
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
    std::push_heap(foobars.begin(), foobars.begin() + fill + 1, LessThanByPriority());

    fill++;

    return true;
}

bool c_remove(Contained *element)
{
    // assert element in container...
    std::pop_heap(foobars.begin() + element->index, foobars.begin() + fill, LessThanByPriority());

    auto begin = foobars_by_key.begin();
    auto end = begin + fill;
    auto it = std::lower_bound(begin, end, element, LessThanString());

    std::move_backward(it + 1, end, it);

    fill--;
    return true;
}

// Adjust an out-of-order element that knows its index inside foobars[]
// so that it is back where it belongs in our heap.
void reprioritize(Container &c)
{
    auto begin = foobars.begin() + c.value->index;
    printf("OOO pri:\n");
    print_it();
    std::pop_heap(begin, foobars.begin() + fill, LessThanByPriority());
    printf("Pop pri:\n");
    print_it();
    std::push_heap(foobars.begin(), foobars.begin() + fill, LessThanByPriority());
    printf("New pri:\n");
    print_it();
}

int main(int argc, char **argv)
{

    for (int i = 1; (i < argc) && (fill < NFOOBARS); i++)
    {
        const char *key;
        if (i < argc)
        {
            key = argv[i];
        }
        else
        {
            key = "foobar";
        }
        printf("Contained<%lu>\n", sizeof(Contained) + strlen(key));
        Contained *value = (Contained *)malloc(sizeof(Contained) + strlen(key));
        value->payload.key = key;
        value->set_priority(rand() % 64);
        c_add(value);
    }

    std::swap(foobars[0], foobars[3]);

    printf("\n");

    print_it();

    printf("\n");

    std::make_heap(foobars.begin(), foobars.begin() + fill, LessThanByPriority());

    print_it();

    if (fill > 4)
    {
        foobars[4].value->set_priority(16);

        reprioritize(foobars[4]);
    }

    print_fbk();

    printf("\n");
    printf("sizeof Container %lu\n", sizeof(Container));
    printf("sizeof Contained %lu\n", sizeof(Contained));
    printf("sizeof trickle_t %lu\n", sizeof(trickle_t));
    printf("sizeof mesh_payload %lu\n", sizeof(mesh_payload));
    printf("sizeof p_string %lu\n", sizeof(p_string));
}
