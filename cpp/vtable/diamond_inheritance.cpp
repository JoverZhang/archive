#include <cassert>
#include <cstdint>

namespace diamond {

class A {
public:
  A(int n) : n(n) {}

  int a = 1;
  int n;
};

class B : public A {
public:
  B(int n) : A(n) {}

  int b = 2;
};

class C : public A {
public:
  C(int n) : A(n) {}

  int c = 3;
};

class D : public B, public C {
public:
  D() : B(10), C(20) {}

  int d = 4;
};

void test() {
  D d;

  assert(d.B::a == 1);
  assert(d.B::n == 10);
  assert(d.b == 2);
  assert(d.C::a == 1);
  assert(d.C::n == 20);
  assert(d.c == 3);
  assert(d.d == 4);

  // Dump memory
  auto p = (uint64_t)&d;
  assert(*(int *)(p + 0) == 1);   // A
  assert(*(int *)(p + 4) == 10);  // .
  assert(*(int *)(p + 8) == 2);   // B
  assert(*(int *)(p + 12) == 1);  // A
  assert(*(int *)(p + 16) == 20); // .
  assert(*(int *)(p + 20) == 3);  // C
  assert(*(int *)(p + 24) == 4);  // D
}

} // namespace diamond

namespace resolution {

class A {
public:
  A(int a) : n(a) {}

  int a = 1;
  int n;
};

class B : virtual public A {
public:
  B(int n) : A(n) {}

  int b = 2;
};

class C : virtual public A {
public:
  C(int n) : A(n) {}

  int c = 3;
};

class D : public B, public C {
public:
  D() : A(30), B(10), C(20) {}

  int d = 4;
};

void test() {
  /* Inside D */
  {
    D d;
    assert(d.A::n == 30);
    assert(d.B::n == 30);
    assert(d.C::n == 30);
    assert(d.n == 30);

    assert(d.b == 2);
    assert(d.c == 3);
    assert(d.d == 4);
    assert(d.a == 1);
    assert(d.n == 30);

    // Dump memory
    auto p = (uint64_t)&d;
    assert(*(int *)(p + 8) == 2);   // B
    assert(*(int *)(p + 24) == 3);  // C
    assert(*(int *)(p + 28) == 4);  // D
    assert(*(int *)(p + 32) == 1);  // A
    assert(*(int *)(p + 36) == 30); // .
  }

  /* Inside B */
  {
    B b(30);

    assert(b.a == 1);
    assert(b.b == 2);
    assert(b.n == 30);

    // Dump memory
    auto p = (uint64_t)&b;
    assert(*(int *)(p + 8) == 2);   // B
    assert(*(int *)(p + 12) == 1);  // A
    assert(*(int *)(p + 16) == 30); // .
  }
}

} // namespace resolution

int main() {
  diamond::test();
  resolution::test();

  return 0;
}
