#include <stdio.h>

#include <Kokkos_Core.hpp>


int
plus(int x, int y)
{
  return x + y;
}

void
say_hello(void)
{
  printf("Hello from CPU!\n");

  int x = plus(1, 2);

  Kokkos::parallel_for(
      "PrintHello", 10,
      KOKKOS_LAMBDA(int) { printf("Hello from GPU (if enabled)!\n"); });
}

int
deux_plus_deux(void)
{
  int x = plus(2, 2);
  return x;
}

int
main(void)
{
  Kokkos::initialize();

  say_hello();
  int x = deux_plus_deux();

  Kokkos::finalize();
  return 0;
}
