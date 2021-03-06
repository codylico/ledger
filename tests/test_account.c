
#include "../src/base/account.h"
#include "../src/base/table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int allocate_test(void);
static int allocate_table_test(void);
static int acquire_table_test(void);
static int description_test(void);
static int null_description_test(void);
static int name_test(void);
static int id_test(void);
static int null_name_test(void);
static int equal_test(void);
static int trivial_equal_test(void);

struct test_struct {
  int (*fn)(void);
  char const* name;
};

struct test_struct test_array[] = {
  { allocate_test, "allocate" },
  { allocate_table_test, "table pre-allocation" },
  { acquire_table_test, "table acquisition" },
  { description_test, "description" },
  { null_description_test, "null_description" },
  { name_test, "name" },
  { null_name_test, "null_name" },
  { id_test, "id" },
  { equal_test, "equal" },
  { trivial_equal_test, "trivial_equal" }
};


int allocate_test(void){
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  ledger_account_free(ptr);
  return 1;
}

int allocate_table_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    if (ledger_account_get_table(ptr) == NULL) break;
    if (ledger_account_get_table_c(ptr) == NULL) break;
    /* check the table */{
      struct ledger_table const* table = ledger_account_get_table_c(ptr);
      if (ledger_table_get_column_count(table) != 5) break;
      if (ledger_table_get_column_type(table,0) != 1) break;
      if (ledger_table_get_column_type(table,1) != 1) break;
      if (ledger_table_get_column_type(table,2) != 2) break;
      if (ledger_table_get_column_type(table,3) != 3) break;
      if (ledger_table_get_column_type(table,4) != 3) break;
    }
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  return result;
}

int acquire_table_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    if (ledger_account_get_table(ptr) == NULL) break;
    if (ledger_account_acquire(ptr) != ptr) break;
    if (ledger_account_get_table_c(ptr) == NULL) break;
    ledger_account_free(ptr);
    /* check the table */{
      struct ledger_table* table;
      table = ledger_account_get_table(ptr);
      if (table == NULL) break;
      if (ledger_table_acquire(table) != table) break;
      ledger_account_free(ptr);
      ptr = NULL;
      if (ledger_table_get_column_count(table) != 5) break;
      if (ledger_table_get_column_type(table,0) != 1) break;
      if (ledger_table_get_column_type(table,1) != 1) break;
      if (ledger_table_get_column_type(table,2) != 2) break;
      if (ledger_table_get_column_type(table,3) != 3) break;
      if (ledger_table_get_column_type(table,4) != 3) break;
      ledger_table_free(table);
    }
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  return result;
}

int description_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    char const* description = "new description";
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    if (ledger_account_set_description
        (ptr, (unsigned char const*)description) == 0)
      break;
    if (ledger_account_get_description(ptr) == NULL) break;
    if (strcmp((char const*)ledger_account_get_description(ptr),
        description) != 0)
      break;
    if (ledger_account_get_name(ptr) != NULL) break;
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  return result;
}

int equal_test(void){
  int result = 0;
  struct ledger_account* ptr, * other_ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  other_ptr = ledger_account_new();
  if (other_ptr == NULL){
    ledger_account_free(ptr);
    return 0;
  } else do {
    int ok;
    unsigned char const* description =
      (unsigned char const*)"new description";
    unsigned char const* description2 =
      (unsigned char const*)"other description";
    /* different descriptions */
    ok = ledger_account_set_description(ptr,description);
    if (!ok) break;
    ok = ledger_account_set_description(other_ptr,description2);
    if (!ok) break;
    if (ledger_account_is_equal(ptr,other_ptr)) break;
    if (ledger_account_is_equal(other_ptr,ptr)) break;
    /* same descriptions */
    ok = ledger_account_set_description(other_ptr,description);
    if (!ok) break;
    if (!ledger_account_is_equal(ptr,other_ptr)) break;
    if (!ledger_account_is_equal(other_ptr,ptr)) break;
    /* null note versus non-empty note */
    ok = ledger_account_set_name(other_ptr,description);
    if (!ok) break;
    if (ledger_account_is_equal(ptr,other_ptr)) break;
    if (ledger_account_is_equal(other_ptr,ptr)) break;
    /* different name */
    ok = ledger_account_set_name(ptr,description2);
    if (!ok) break;
    if (ledger_account_is_equal(other_ptr,ptr)) break;
    if (ledger_account_is_equal(ptr,other_ptr)) break;
    /* same name */
    ok = ledger_account_set_name(other_ptr,description2);
    if (!ok) break;
    if (!ledger_account_is_equal(other_ptr,ptr)) break;
    if (!ledger_account_is_equal(ptr,other_ptr)) break;
    /* add a row */{
      struct ledger_table *const table = ledger_account_get_table(ptr);
      struct ledger_table_mark *mark;
      ok = 0;
      do {
        mark = ledger_table_begin(table);
        if (mark == NULL) break;
        if (!ledger_table_add_row(mark))
          break;
        ok = 1;
      } while (0);
      ledger_table_mark_free(mark);
    }
    if (!ok) break;
    if (ledger_account_is_equal(ptr,other_ptr)) break;
    if (ledger_account_is_equal(other_ptr,ptr)) break;
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  ledger_account_free(other_ptr);
  return result;
}

int trivial_equal_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    if (ledger_account_is_equal(ptr,NULL)) break;
    if (!ledger_account_is_equal(NULL,NULL)) break;
    if (!ledger_account_is_equal(ptr,ptr)) break;
    if (ledger_account_is_equal(NULL,ptr)) break;
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  return result;
}

int null_description_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    char const* description = NULL;
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    if (ledger_account_set_description
        (ptr, (unsigned char const*)description) == 0)
      break;
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  return result;
}

int name_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    char const* name = "new name";
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    if (ledger_account_set_name
        (ptr, (unsigned char const*)name) == 0)
      break;
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) == NULL) break;
    if (strcmp((char const*)ledger_account_get_name(ptr),
        name) != 0)
      break;
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  return result;
}

int id_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    if (ledger_account_get_id(ptr) != -1) break;
    ledger_account_set_id(ptr, 80);
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    if (ledger_account_get_id(ptr) != 80) break;
    result = 1;
  } while (0);
  ledger_account_free(ptr);
  return result;
}

int null_name_test(void){
  int result = 0;
  struct ledger_account* ptr;
  ptr = ledger_account_new();
  if (ptr == NULL) return 0;
  else do {
    char const* name = NULL;
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    if (ledger_account_set_name
        (ptr, (unsigned char const*)name) == 0)
      break;
    if (ledger_account_get_description(ptr) != NULL) break;
    if (ledger_account_get_name(ptr) != NULL) break;
    result = 1;
  } while (0);
  ledger_account_free(ptr);
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
