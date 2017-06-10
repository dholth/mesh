#include "container.cpp"

int main(int argc, char **argv) {

  for (int i = 1; (i < argc) && (fill < NFOOBARS); i++) {
    const char *key;
    if (i < argc) {
      key = argv[i];
    } else {
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

  if (fill > 4) {
    foobars[4].value->set_priority(16);

    reprioritize(foobars[4].value);
  }

  print_fbk();

  printf("\n");
  printf("sizeof Container %lu\n", sizeof(Container));
  printf("sizeof Contained %lu\n", sizeof(Contained));
  printf("sizeof trickle_t %lu\n", sizeof(trickle_t));
  printf("sizeof mesh_payload %lu\n", sizeof(mesh_payload));
  printf("sizeof p_string %lu\n", sizeof(p_string));
}
