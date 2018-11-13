
#include "../src/base/bignum.h"
#include "../src/base/util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static int allocate_test(void);
static int trivial_compare_test(void);
static int set_long_test(void);
static int set_limit_long_test(void);
static int internal_allocate_test(void);
static int get_text_test(void);
static int set_text_test(void);

struct test_struct {
  int (*fn)(void);
  char const* name;
};
struct test_struct test_array[] = {
  { allocate_test, "allocate" },
  { trivial_compare_test, "trivial compare" },
  { internal_allocate_test, "internal allocate" },
  { set_long_test, "set long" },
  { set_limit_long_test, "set limit long" },
  { get_text_test, "get text" },
  { set_text_test, "set text" }
};

int allocate_test(void){
  struct ledger_bignum* ptr;
  ptr = ledger_bignum_new();
  if (ptr == NULL) return 0;
  ledger_bignum_free(ptr);
  return 1;
}

int internal_allocate_test(void){
  int result = 0;
  struct ledger_bignum* ptr;
  ptr = ledger_bignum_new();
  if (ptr == NULL) return 0;
  do {
    int ok;
    if (ledger_bignum_count_digits(ptr) != 0) break;
    if (ledger_bignum_find_point(ptr) != 0) break;
    /* invalid allocation */
    ok = ledger_bignum_alloc(ptr,4,5);
    if (ok) break;
    if (ledger_bignum_count_digits(ptr) != 0) break;
    if (ledger_bignum_find_point(ptr) != 0) break;
    /* valid allocation */
    ok = ledger_bignum_alloc(ptr,4,2);
    if (!ok) break;
    if (ledger_bignum_count_digits(ptr) != 4) break;
    if (ledger_bignum_find_point(ptr) != 2) break;
    result = 1;
  } while (0);
  ledger_bignum_free(ptr);
  return result;
}

int trivial_compare_test(void){
  int result = 0;
  struct ledger_bignum* ptr, * other_ptr;
  ptr = ledger_bignum_new();
  if (ptr == NULL) return 0;
  other_ptr = ledger_bignum_new();
  if (other_ptr == NULL){
    ledger_bignum_free(ptr);
    return 0;
  }
  do {
    if (ledger_bignum_compare(ptr,other_ptr) != 0) break;
    if (ledger_bignum_compare(other_ptr, ptr) != 0) break;
    result = 1;
  } while (0);
  ledger_bignum_free(other_ptr);
  ledger_bignum_free(ptr);
  return result;
}
int set_long_test(void){
  int result = 0;
  struct ledger_bignum* ptr;
  ptr = ledger_bignum_new();
  if (ptr == NULL) return 0;
  do {
    if (!ledger_bignum_set_long(ptr,378)) break;
    if (ledger_bignum_get_long(ptr) != 378) break;
    if (!ledger_bignum_set_long(ptr,-96058l)) break;
    if (ledger_bignum_get_long(ptr) != -96058l) break;
    if (!ledger_bignum_set_long(ptr,392050493l)) break;
    if (ledger_bignum_get_long(ptr) != 392050493l) break;
    if (!ledger_bignum_set_long(ptr,-2092050493l)) break;
    if (ledger_bignum_get_long(ptr) != -2092050493l) break;
    result = 1;
  } while (0);
  ledger_bignum_free(ptr);
  return result;
}
int set_limit_long_test(void){
  int result = 0;
  struct ledger_bignum* ptr;
  ptr = ledger_bignum_new();
  if (ptr == NULL) return 0;
  do {
    if (!ledger_bignum_set_long(ptr,LONG_MIN)) break;
    if (ledger_bignum_get_long(ptr) != LONG_MIN) break;
    if (!ledger_bignum_set_long(ptr,LONG_MIN+1)) break;
    if (ledger_bignum_get_long(ptr) != LONG_MIN+1) break;
    if (!ledger_bignum_set_long(ptr,LONG_MAX)) break;
    if (ledger_bignum_get_long(ptr) != LONG_MAX) break;
    if (!ledger_bignum_set_long(ptr,LONG_MAX-1)) break;
    if (ledger_bignum_get_long(ptr) != LONG_MAX-1) break;
    result = 1;
  } while (0);
  ledger_bignum_free(ptr);
  return result;
}

int get_text_test(void){
  int result = 0;
  struct ledger_bignum* ptr;
  unsigned char buffer[16];
  ptr = ledger_bignum_new();
  if (ptr == NULL) return 0;
  do {
    if (!ledger_bignum_set_long(ptr,-378)) break;
    if (ledger_bignum_get_long(ptr) != -378) break;
    if (ledger_bignum_get_text(ptr,NULL,0,0) != 4) break;
    if (ledger_bignum_get_text(ptr,buffer,sizeof(buffer),0) != 4) break;
    if (ledger_util_ustrcmp(
        (unsigned char const*)"-378",buffer) != 0) break;
    if (!ledger_bignum_alloc(ptr,8,4)) break;
    if (!ledger_bignum_set_long(ptr,9515)) break;
    if (ledger_bignum_get_text(ptr,NULL,0,0) != 13) break;
    if (ledger_bignum_get_text(ptr,buffer,sizeof(buffer),0) != 13) break;
    if (ledger_util_ustrcmp(
        (unsigned char const*)"9515.00000000",buffer) != 0) break;
    if (ledger_bignum_get_text(ptr,NULL,0,1) != 14) break;
    if (ledger_bignum_get_text(ptr,buffer,sizeof(buffer),1) != 14) break;
    if (ledger_util_ustrcmp(
        (unsigned char const*)"+9515.00000000",buffer) != 0) break;
    result = 1;
  } while (0);
  ledger_bignum_free(ptr);
  return result;
}

int set_text_test(void){
  int result = 0;
  struct ledger_bignum* ptr;
  unsigned char buffer[24];
  ptr = ledger_bignum_new();
  if (ptr == NULL) return 0;
  do {
    if (!ledger_bignum_set_text(
        ptr,(unsigned char const*)"-378",NULL)) break;
    if (ledger_bignum_get_long(ptr) != -378) break;
    if (!ledger_bignum_set_text(
        ptr,(unsigned char const*)"-378.",NULL)) break;
    if (ledger_bignum_get_long(ptr) != -378) break;
    if (!ledger_bignum_set_text(
        ptr,(unsigned char const*)"-00000378.0",NULL)) break;
    if (ledger_bignum_get_long(ptr) != -378) break;
    if (ledger_bignum_get_text(ptr,NULL,0,0) != 7) break;
    if (ledger_bignum_get_text(ptr,buffer,sizeof(buffer),0) != 7) break;
    if (ledger_util_ustrcmp(
        (unsigned char const*)"-378.00",buffer) != 0) break;
    if (!ledger_bignum_alloc(ptr,8,4)) break;
    /* endptr check */{
      unsigned char const* text9515 = (unsigned char const*)"9515";
      unsigned char* text_ptr;
      if (!ledger_bignum_set_text(
          ptr,text9515,&text_ptr)) break;
      if (text_ptr != text9515+4) break;
    }
    if (ledger_bignum_get_text(ptr,NULL,0,0) != 13) break;
    if (ledger_bignum_get_text(ptr,buffer,sizeof(buffer),0) != 13) break;
    if (ledger_util_ustrcmp(
        (unsigned char const*)"9515.00000000",buffer) != 0) break;
    if (ledger_bignum_get_text(ptr,NULL,0,1) != 14) break;
    if (ledger_bignum_get_text(ptr,buffer,sizeof(buffer),1) != 14) break;
    if (ledger_util_ustrcmp(
        (unsigned char const*)"+9515.00000000",buffer) != 0) break;
    if (ledger_bignum_get_long(ptr) != +9515) break;
    result = 1;
  } while (0);
  ledger_bignum_free(ptr);
  return result;
}


int main(int argc, char **argv){
  int pass_count = 0;
  int const test_count = sizeof(test_array)/sizeof(test_array[0]);
  int i;
  printf("Running %i tests...\n", test_count);
  for (i = 0; i < test_count; ++i){
    int pass_value;
    printf("\t%s... ", test_array[i].name);
    pass_value = ((*test_array[i].fn)())?1:0;
    printf("%s\n",pass_value==0?"FAILED":"PASSED");
    pass_count += pass_value;
  }
  printf("...%i out of %i tests passed.\n", pass_count, test_count);
  return pass_count==test_count?EXIT_SUCCESS:EXIT_FAILURE;
}
