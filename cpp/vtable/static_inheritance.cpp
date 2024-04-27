#include <cassert>
#include <cstdint>
#include <sys/types.h>

class A {
public:
  int a = 1;
};

class B {
public:
  int b = 2;
};

class C : public A, public B {
public:
  int c = 3;
};

class D : public A, public B, public C {
public:
  int d = 4;
};

int main() {
  D d;

  // Upcast
  A *a = (A *)(C *)&d;
  assert(a->a == 1);

  B *b = (B *)(C *)&d;
  assert(b->b == 2);

  C *c = (C *)&d;
  assert(c->A::a == 1);
  assert(c->B::b == 2);
  assert(c->c == 3);

  // Downcast
  D *d2 = (D *)(C *)b;
  assert(d2->C::a == 1);
  assert(d2->C::b == 2);
  assert(d2->c == 3);
  assert(d2->d == 4);

  // Dump memory
  auto p = (uint64_t)&d;
  assert(*(int *)(p + 0) == 1);  // A
  assert(*(int *)(p + 4) == 2);  // B
  assert(*(int *)(p + 8) == 1);  // A
  assert(*(int *)(p + 12) == 2); // B
  assert(*(int *)(p + 16) == 3); // C
  assert(*(int *)(p + 20) == 4); // D

  return 0;
}
