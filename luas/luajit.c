/** @file lua_55.c
 *  @brief lua_55 -- A wrapper around pdlua.c to allow building against the shipped LuaJIT 2.1 + compat53 submodule
 *  @author Timothy Schoen
 *  @date 2026
 *
 * Copyright (C) 2026 Timothy Schoen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "luajit/src/lua.h"
#include "luajit/src/lauxlib.h"
#include "luajit/src/lualib.h"

#define LUAMOD_API

// Include 5.3 C-API compatibility layer
#include "lua-compat-5.3/c-api/compat-5.3.h"

// Include 5.3 lua compatibility layer
#include "lua-compat-headers/compat53_init.h"
#include "lua-compat-headers/compat53_module.h"
#include "lua-compat-headers/compat53_file_mt.h"

static void preload_compat53(lua_State *L) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");

    luaL_loadbuffer(L, (const char*)lua_compat_5_3_compat53_init_lua,
                       lua_compat_5_3_compat53_init_lua_len, "compat53");
    lua_setfield(L, -2, "compat53");

    luaL_loadbuffer(L, (const char*)lua_compat_5_3_compat53_module_lua,
                       lua_compat_5_3_compat53_module_lua_len, "compat53.module");
    lua_setfield(L, -2, "compat53.module");

    luaL_loadbuffer(L, (const char*)lua_compat_5_3_compat53_file_mt_lua,
                       lua_compat_5_3_compat53_file_mt_lua_len, "compat53.file_mt");
    lua_setfield(L, -2, "compat53.file_mt");

    lua_pop(L, 2);

    lua_getglobal(L, "require");
    lua_pushstring(L, "compat53");
    lua_call(L, 1, 0);
}


#undef lua_load
#undef lua_gc
#define lua_gc(L, what) lua_gc(L, what, 0)

#define pdlua_gfx_free pdluajit_gfx_free
#define pdlua_gfx_mouse_down pdluajit_gfx_mouse_down
#define pdlua_gfx_mouse_drag pdluajit_gfx_mouse_drag
#define pdlua_gfx_mouse_enter pdluajit_gfx_mouse_enter
#define pdlua_gfx_mouse_event pdluajit_gfx_mouse_event
#define pdlua_gfx_mouse_exit pdluajit_gfx_mouse_exit
#define pdlua_gfx_mouse_move pdluajit_gfx_mouse_move
#define pdlua_gfx_mouse_up pdluajit_gfx_mouse_up
#define pdlua_gfx_repaint pdluajit_gfx_repaint
#define pdlua_gfx_setup pdluajit_gfx_setup
#define pdlua_instance_setup pdluajit_instance_setup
#define pdlua_setup pdluajit_setup
#define pdlua_datadir pdluajit_datadir

#ifdef PLUGDATA
#define plugdata_register_class plugdata_register_class_jit
#endif

#define LUA_FILE_EXTENSION ".pd_luajit"
#define LUA_USE_JIT 1

#include "../pdlua.c"
