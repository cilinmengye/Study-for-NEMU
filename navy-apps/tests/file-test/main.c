#include <stdio.h>
#include <assert.h>
#include <unistd.h>

int main() {
  write(1, "start?\n", 7);
  FILE *fp = fopen("/share/files/num", "r+");
  write(1, "before success?\n", 16);
  assert(fp);
  write(1, "success?\n", 9);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  write(1, "before success?\n", 16);
  assert(size == 5000);
  write(1, "success?\n", 9);

  fseek(fp, 500 * 5, SEEK_SET);
  int i, n;
  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    assert(n == i + 1);
  }
  write(1, "success?\n", 9);

  fseek(fp, 0, SEEK_SET);
  for (i = 0; i < 500; i ++) {
    fprintf(fp, "%4d\n", i + 1 + 1000);
  }
  write(1, "success?\n", 9);

  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    assert(n == i + 1);
  }
  write(1, "success?\n", 9);

  fseek(fp, 0, SEEK_SET);
  for (i = 0; i < 500; i ++) {
    fscanf(fp, "%d", &n);
    assert(n == i + 1 + 1000);
  }
  write(1, "success?\n", 9);

  fclose(fp);

  printf("PASS!!!\n");

  return 0;
}
