#include <stdio.h>
#include <assert.h>

int main() {
  printf("start?");
  FILE *fp = fopen("/share/files/num", "r+");
  assert(fp);
  printf("success?");

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  assert(size == 5000);
  printf("success?");

  fseek(fp, 500 * 5, SEEK_SET);
  int i, n;
  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    assert(n == i + 1);
  }
  printf("success?");

  fseek(fp, 0, SEEK_SET);
  for (i = 0; i < 500; i ++) {
    fprintf(fp, "%4d\n", i + 1 + 1000);
  }
  printf("success?");

  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    assert(n == i + 1);
  }
  printf("success?");

  fseek(fp, 0, SEEK_SET);
  for (i = 0; i < 500; i ++) {
    fscanf(fp, "%d", &n);
    assert(n == i + 1 + 1000);
  }
  printf("success?");

  fclose(fp);

  printf("PASS!!!\n");

  return 0;
}
