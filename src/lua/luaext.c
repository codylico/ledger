
#include "luaext.h"
#include "llbase.h"
#include "llio.h"
#include "llact.h"
#include "../base/util.h"
#include "../../deps/lua/src/lua.h"
#include "../../deps/lua/src/lualib.h"
#include "../../deps/lua/src/lauxlib.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

struct ledger_lua {
  int lib_type;
  lua_State* base;
  unsigned char* last_error;
  unsigned char* last_text;
  size_t last_text_length;
};

/*
 * String reader structure.
 */
struct ledger_lua_str_reader {
  /* Text to read. */
  unsigned char const* p;
};

/*
 * File reader structure.
 */
struct ledger_lua_file_reader {
  /* File to read. */
  FILE* f;
  /* Error encounter. */
  int err;
  /* read buffer */
  unsigned char buf[256];
};


/*
 * Add to the current text buffer.
 * - l Lua package to update
 * - ss new string chunk
 * @return one on success, zero otherwise
 */
int ledger_lua_add_last_text
  ( struct ledger_lua* l, unsigned char const* ss);

/*
 * Drop the current text buffer.
 * - l Lua package to update
 */
void ledger_lua_clear_last_text( struct ledger_lua* l);

/*
 * Close a Lua structure.
 * - l the Lua structure to close
 */
void ledger_lua_clear(struct ledger_lua* l);

/*
 * Initialize a Lua structure.
 * - l the Lua structure to initialize
 * @return one on success, zero otherwise
 */
int ledger_lua_init(struct ledger_lua* l);

/*
 * Allocate and free memory for the Lua state.
 * - ud user data
 * - ptr pointer to memory block to manage
 * - osize old block size when `ptr` is not NULL, otherwise
 *     a type code
 * - nsize new desired block size
 * @return a pointer to allocated memory on successful allocation,
 *   NULL on successful free, or NULL otherwise
 */
void* ledger_lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize);

/*
 * Load a text string of a certain length.
 * - l ledger-lua state
 * - ss NUL-terminated string to load
 * - chunk_name name to place on the string
 * - request_continue output field, set to one if more script lines
 *     are needed
 * @return one on success, zero otherwise
 */
static int ledger_lua_load_str
  ( struct ledger_lua* l, unsigned char const* ss,
    unsigned char const* chunk_name, int *request_continue);

/*
 * Load a text string of a certain length.
 * - l ledger-lua state
 * - fn NUL-terminated name of file to load
 * @return one on success, zero otherwise
 */
static int ledger_lua_load_file
  ( struct ledger_lua const* l, char const* fn,
    unsigned char const* chunk_name);

/*
 * Read a string for Lua.
 * - base Lua state
 * - data string reader structure
 * - size pointer to hold output size
 * @return the next chunk for processing on success, or NULL at end-of-string
 */
static char const* ledger_lua_str_read
  (lua_State *base, void *data, size_t *size);

/*
 * Read a file for Lua.
 * - base Lua state
 * - data file reader structure
 * - size pointer to hold output size
 * @return the next chunk for processing on success, or NULL at end-of-string
 */
static char const* ledger_lua_str_read
  (lua_State *base, void *data, size_t *size);

/*
 * Run the function at the top of the stack.
 * - l ledger-lua state
 * @return one on success, zero otherwise
 */
static int ledger_lua_exec_top( struct ledger_lua* l);

/*
 * from the default Lua 5.3.5 interpreter:
 *
 * Create the 'arg' table, which stores all arguments from the
 * command line ('argv'). It should be aligned so that, at index 0,
 * it has 'argv[script]', which is the script name. The arguments
 * to the script (everything after 'script') go to positive indices;
 * other arguments (before the script name) go to negative indices.
 * If there is no script name, assume interpreter's name as base.
 */
static int ledger_lua_set_arg_i(lua_State *L);

/*
 * Load the ledger library.
 * @param l library to load
 * @return zero
 */
static int ledger_lua_loadledgerlib(lua_State *l);

/*
 * Set the last error message for the Lua package.
 * - l the package to modify
 * - le last error string
 * @return one on success, zero otherwise
 */
static int ledger_lua_set_last_error
  (struct ledger_lua* l, char const* le);


/* BEGIN static implementation */

int ledger_lua_set_arg_i(lua_State *L){
  /*
   * Code based on default Lua 5.3.5 interpreter
   * Copyright (C) 1994-2018 Lua.org, PUC-Rio.
   * Licensed under the MIT License.
   */
  char **argv = (char **)lua_touserdata(L, 2);
  int argc = (int)lua_tointeger(L, 1);
  int script = (int)lua_tointeger(L, 3);
  int i, narg;
  if (script == argc) script = 0;  /* no script name? */
  narg = argc - (script + 1);  /* number of positive indices */
  lua_createtable(L, narg, script + 1);
  for (i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - script);
  }
  lua_setglobal(L, "arg");
  return 0;
}

void ledger_lua_clear(struct ledger_lua* l){
  if (l->base != NULL){
    lua_close(l->base);
    l->base = NULL;
  }
  if (l->last_error != NULL){
    ledger_util_free(l->last_error);
    l->last_error = NULL;
  }
  if (l->last_text != NULL){
    ledger_util_free(l->last_text);
    l->last_text = NULL;
  }
  l->last_text_length = 0;
  return;
}

int ledger_lua_init(struct ledger_lua* l){
  lua_State *new_state = lua_newstate(&ledger_lua_alloc, l);
  if (new_state == NULL){
    return 0;
  }
  l->base = new_state;
  l->lib_type = 0;
  l->last_error = NULL;
  l->last_text = NULL;
  l->last_text_length = 0;
  return 1;
}

void* ledger_lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize){
  struct ledger_lua* const l = (struct ledger_lua*)ud;
  (void)l;
  if (nsize == 0){
    ledger_util_free(ptr);
    return NULL;
  } else if (ptr != NULL){
    if (nsize == osize) return ptr;
    else if (nsize < osize){
      void* new_ptr = ledger_util_malloc(nsize);
      if (new_ptr != NULL){
        memcpy(new_ptr, ptr, nsize);
        ledger_util_free(ptr);
        return new_ptr;
      } else return new_ptr;
    } else {
      void* new_ptr = ledger_util_malloc(nsize);
      if (new_ptr != NULL){
        memcpy(new_ptr, ptr, osize);
        ledger_util_free(ptr);
        return new_ptr;
      } else return NULL;
    }
  } else switch (osize){
  case LUA_TSTRING:
    if (nsize >= 65534){
      /* artificial limit for strings */
      return NULL;
    } else return ledger_util_malloc(nsize);
  default:
    return ledger_util_malloc(nsize);
  }
}

const char * ledger_lua_str_read(lua_State *l, void *data, size_t *size){
  struct ledger_lua_str_reader *const reader =
      (struct ledger_lua_str_reader *)data;
  if (reader->p[0] != 0){
    char const* out = (char const*)(reader->p);
    size_t i;
    for (i = 0; i < 256 && reader->p[0] != 0; ++i){
      reader->p += 1;
    }
    *size = i;
    return out;
  } else {
    *size = 0;
    return NULL;
  }
}

const char * ledger_lua_file_read(lua_State *l, void *data, size_t *size){
  struct ledger_lua_file_reader *const reader =
      (struct ledger_lua_file_reader *)data;
  size_t len = fread
    (reader->buf,sizeof(unsigned char), sizeof(reader->buf), reader->f);
  if (ferror(reader->f)){
    *size = 0;
    reader->err = 1;
    return NULL;
  } else if (len > 0){
    *size = len;
    return (char const*)reader->buf;
  } else {
    *size = 0;
    return NULL;
  }
}

int ledger_lua_add_last_text
  ( struct ledger_lua* l, unsigned char const* ss)
{
  size_t add_len = strlen((char const*)ss);
  if (add_len >= ((~0u)-l->last_text_length-1)){
    return 0;
  } else {
    size_t next_offset = l->last_text_length;
    unsigned char* new_text =
      (unsigned char*)ledger_util_malloc(l->last_text_length+add_len+2);
    if (new_text == NULL) return 0;
    memcpy(new_text, l->last_text, l->last_text_length);
    if (next_offset > 0){
      new_text[next_offset] = '\n';
      next_offset += 1;
    }
    memcpy(new_text+next_offset, ss, add_len);
    new_text[next_offset+add_len] = 0;
    ledger_util_free(l->last_text);
    l->last_text = new_text;
    l->last_text_length = next_offset+add_len;
    return 1;
  }
}

void ledger_lua_clear_last_text( struct ledger_lua* l){
  ledger_util_free(l->last_text);
  l->last_text_length = 0;
  l->last_text = NULL;
  return;
}

int ledger_lua_load_str
  ( struct ledger_lua* l, unsigned char const* ss,
    unsigned char const* chunk_name, int *request_continue)
{
  int continue_wanted = 0;
  int l_result;
  struct ledger_lua_str_reader reader;
  if (ledger_lua_add_last_text(l, ss) == 0){
    lua_pushliteral(l->base, "ledger_lua: text too long");
    l_result = LUA_ERRMEM;
    continue_wanted = 0;
  } else {
    reader.p = l->last_text;
    l_result = lua_load(l->base,
        &ledger_lua_str_read, &reader, (char const*)chunk_name, "t");
    if (l_result == LUA_ERRSYNTAX && *ss != 0){
      size_t errlen;
      char const* errmsg;
      /* check <eof> token */{
        errmsg = lua_tolstring(l->base, -1, &errlen);
        if (errlen >= 5
        &&  memcmp("<eof>", errmsg+errlen-5, 5) == 0)
        {
          lua_pop(l->base, 1);
          l_result = LUA_OK;
          continue_wanted = 1;
        }
      }
    }
  }
  if (!continue_wanted)
    ledger_lua_clear_last_text(l);
  if (request_continue != NULL)
    *request_continue = continue_wanted;
  return l_result==LUA_OK?1:0;
}

int ledger_lua_load_file
  ( struct ledger_lua const* l, char const* fn,
    unsigned char const* chunk_name)
{
  int l_result;
  FILE *f = fopen(fn,"rt");
  if (f != NULL){
    struct ledger_lua_file_reader reader = { f, 0 };
    l_result = lua_load(l->base,
        &ledger_lua_file_read, &reader, (char const*)chunk_name, "t");
    fclose(f);
    if (l_result == LUA_OK && reader.err){
      /* reject successful partial compile */
      lua_pop(l->base, 1);
      return 0;
    } else return l_result==LUA_OK?1:0;
  } else return 0;
}

int ledger_lua_exec_top(struct ledger_lua* l){
  int l_result;
  if (!lua_isfunction(l->base, -1))
    return 0;
  else {
    l_result = lua_pcall(l->base, 0, 0, 0);
    if (l_result != LUA_OK){
      ledger_lua_set_last_error(l, lua_tostring(l->base, -1));
      lua_pop(l->base, 1);
    }
    return l_result==LUA_OK?1:0;
  }
}

unsigned char const* ledger_lua_get_last_error(struct ledger_lua const* l){
  return l->last_error;
}

int ledger_lua_loadledgerlib(struct lua_State *l){
  luaL_getsubtable(l, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
  lua_pushcfunction(l, luaopen_ledger);
  lua_setfield(l, -2, "ledger");
  lua_pop(l, 1);
  return 0;
}

int ledger_lua_set_last_error
  (struct ledger_lua* l, char const* le)
{
  int ok;
  unsigned char* next_error =
    ledger_util_ustrdup((unsigned char const*)le, &ok);
  if (ok){
    ledger_util_free(l->last_error);
    l->last_error = next_error;
  }
  return ok;
}

/* END   static implementation */

/* BEGIN implementation */

struct ledger_lua* ledger_lua_new(void){
  struct ledger_lua* a = (struct ledger_lua* )ledger_util_malloc
    (sizeof(struct ledger_lua));
  if (a != NULL){
    if (!ledger_lua_init(a)){
      ledger_util_free(a);
      a = NULL;
    }
  }
  return a;
}

void ledger_lua_close(struct ledger_lua* a){
  if (a != NULL){
    ledger_lua_clear(a);
    ledger_util_free(a);
  }
}

int ledger_lua_exec_str
  ( struct ledger_lua* l, unsigned char const* name, unsigned char const* s,
    int want_continue, int *request_continue)
{
  int result;
  int subrequest;
  if (!want_continue){
    ledger_lua_clear_last_text(l);
  }
  result = ledger_lua_load_str(l,s, name, &subrequest);
  if (request_continue != NULL) *request_continue = subrequest;
  if (result == 0){
    ledger_lua_set_last_error(l, lua_tostring(l->base, -1));
    return 0;
  } else if (!subrequest){
    return ledger_lua_exec_top(l);
  } else return 1;
}

int ledger_lua_openlibs(struct ledger_lua* l){
  int ok = 0;
  if (l->lib_type != 0){
    return l->lib_type == 1;
  } else do {
    int l_result;
    luaL_openlibs(l->base);
    lua_pushcfunction(l->base, &ledger_lua_loadledgerlib);
    l_result = lua_pcall(l->base,0,0,0);
    if (l_result != LUA_OK) break;
    ok = 1;
  } while (0);
  if (ok) l->lib_type = 1;
  else l->lib_type = -1;
  return ok;
}

int ledger_lua_exec_file(struct ledger_lua* l,
    unsigned char const* name, char const* f)
{
  int result;
  result = ledger_lua_load_file(l,f, name);
  if (result == 0){
    ledger_lua_set_last_error(l, lua_tostring(l->base, -1));
    lua_pop(l->base, 1);
    return 0;
  } else return ledger_lua_exec_top(l);
}

int ledger_lua_set_arg
  (struct ledger_lua* l, char **argv, int argc, int script)
{
  int status;
  lua_State *ls = l->base;
  /* use protected call technique from default Lua interpreter */
  /*
   * Code based on default Lua 5.3.5 interpreter
   * Copyright (C) 1994-2018 Lua.org, PUC-Rio.
   * Licensed under the MIT License.
   */
  lua_pushcfunction(ls, &ledger_lua_set_arg_i);
    /* to call '...set_arg_i' in protected mode */
  lua_pushinteger(ls, argc);  /* 1st argument */
  lua_pushlightuserdata(ls, argv); /* 2nd argument */
  lua_pushinteger(ls, script);  /* 3rd argument */
  status = lua_pcall(ls, 3, 0, 0);  /* do the call */
  return status==LUA_OK?1:0;
}

int luaopen_ledger(struct lua_State* l){
  /* create the `ledger` table */{
    lua_newtable(l);
  }
  /* load the base library */{
    ledger_luaopen_base(l);
  }
  /* load the input/output library */{
    ledger_luaopen_io(l);
  }
  /* load the action library */{
    ledger_luaopen_act(l);
  }
  return 1;
}

/* END   implementation */
