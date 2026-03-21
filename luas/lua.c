/** @file lua.c
 *  @brief lua.c -- A wrapper around pdlua.c to allow building against the shipped Lua 5.5 submodule
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

#define luaL_addgsub lua55_luaL_addgsub
#define luaL_addlstring lua55_luaL_addlstring
#define luaL_addstring lua55_luaL_addstring
#define luaL_addvalue lua55_luaL_addvalue
#define luaL_argerror lua55_luaL_argerror
#define luaL_buffinit lua55_luaL_buffinit
#define luaL_buffinitsize lua55_luaL_buffinitsize
#define luaL_callmeta lua55_luaL_callmeta
#define luaL_checkany lua55_luaL_checkany
#define luaL_checkinteger lua55_luaL_checkinteger
#define luaL_checklstring lua55_luaL_checklstring
#define luaL_checknumber lua55_luaL_checknumber
#define luaL_checkoption lua55_luaL_checkoption
#define luaL_checkstack lua55_luaL_checkstack
#define luaL_checktype lua55_luaL_checktype
#define luaL_checkudata lua55_luaL_checkudata
#define luaL_checkversion_ lua55_luaL_checkversion_
#define luaL_error lua55_luaL_error
#define luaL_execresult lua55_luaL_execresult
#define luaL_fileresult lua55_luaL_fileresult
#define luaL_getmetafield lua55_luaL_getmetafield
#define luaL_getsubtable lua55_luaL_getsubtable
#define luaL_gsub lua55_luaL_gsub
#define luaL_len lua55_luaL_len
#define luaL_loadbufferx lua55_luaL_loadbufferx
#define luaL_loadfilex lua55_luaL_loadfilex
#define luaL_loadstring lua55_luaL_loadstring
#define luaL_newmetatable lua55_luaL_newmetatable
#define luaL_newstate lua55_luaL_newstate
#define luaL_openselectedlibs lua55_openselectedlibs
#define luaL_optinteger lua55_luaL_optinteger
#define luaL_optlstring lua55_luaL_optlstring
#define luaL_optnumber lua55_luaL_optnumber
#define luaL_prepbuffsize lua55_luaL_prepbuffsize
#define luaL_pushresult lua55_luaL_pushresult
#define luaL_pushresultsize lua55_luaL_pushresultsize
#define luaL_ref lua55_luaL_ref
#define luaL_requiref lua55_luaL_requiref
#define luaL_setfuncs lua55_luaL_setfuncs
#define luaL_setmetatable lua55_luaL_setmetatable
#define luaL_testudata lua55_luaL_testudata
#define luaL_tolstring lua55_luaL_tolstring
#define luaL_traceback lua55_luaL_traceback
#define luaL_typeerror lua55_luaL_typeerror
#define luaL_unref lua55_luaL_unref
#define luaL_where lua55_luaL_where
#define lua_absindex lua55_lua_absindex
#define lua_arith lua55_lua_arith
#define lua_atpanic lua55_lua_atpanic
#define lua_callk lua55_lua_callk
#define lua_checkstack lua55_lua_checkstack
#define lua_close lua55_lua_close
#define lua_closeslot lua55_lua_closeslot
#define lua_closethread lua55_lua_closethread
#define lua_compare lua55_lua_compare
#define lua_concat lua55_lua_concat
#define lua_copy lua55_lua_copy
#define lua_createtable lua55_lua_createtable
#define lua_dump lua55_lua_dump
#define lua_error lua55_lua_error
#define lua_gc lua55_lua_gc
#define lua_getallocf lua55_lua_getallocf
#define lua_getfield lua55_lua_getfield
#define lua_getglobal lua55_lua_getglobal
#define lua_gethook lua55_lua_gethook
#define lua_gethookcount lua55_lua_gethookcount
#define lua_gethookmask lua55_lua_gethookmask
#define lua_geti lua55_lua_geti
#define lua_getinfo lua55_lua_getinfo
#define lua_getiuservalue lua55_lua_getiuservalue
#define lua_getlocal lua55_lua_getlocal
#define lua_getmetatable lua55_lua_getmetatable
#define lua_getstack lua55_lua_getstack
#define lua_gettable lua55_lua_gettable
#define lua_gettop lua55_lua_gettop
#define lua_getupvalue lua55_lua_getupvalue
#define lua_iscfunction lua55_lua_iscfunction
#define lua_isinteger lua55_lua_isinteger
#define lua_isnumber lua55_lua_isnumber
#define lua_isstring lua55_lua_isstring
#define lua_isuserdata lua55_lua_isuserdata
#define lua_isyieldable lua55_lua_isyieldable
#define lua_len lua55_lua_len
#define lua_load lua55_lua_load
#define lua_newstate lua55_lua_newstate
#define lua_newthread lua55_lua_newthread
#define lua_newuserdatauv lua55_lua_newuserdatauv
#define lua_next lua55_lua_next
#define lua_pcallk lua55_lua_pcallk
#define lua_pushboolean lua55_lua_pushboolean
#define lua_pushcclosure lua55_lua_pushcclosure
#define lua_pushfstring lua55_lua_pushfstring
#define lua_pushinteger lua55_lua_pushinteger
#define lua_pushlightuserdata lua55_lua_pushlightuserdata
#define lua_pushlstring lua55_lua_pushlstring
#define lua_pushnil lua55_lua_pushnil
#define lua_pushnumber lua55_lua_pushnumber
#define lua_pushstring lua55_lua_pushstring
#define lua_pushthread lua55_lua_pushthread
#define lua_pushvalue lua55_lua_pushvalue
#define lua_pushvfstring lua55_lua_pushvfstring
#define lua_rawequal lua55_lua_rawequal
#define lua_rawget lua55_lua_rawget
#define lua_rawgeti lua55_lua_rawgeti
#define lua_rawgetp lua55_lua_rawgetp
#define lua_rawlen lua55_lua_rawlen
#define lua_rawset lua55_lua_rawset
#define lua_rawseti lua55_lua_rawseti
#define lua_rawsetp lua55_lua_rawsetp
#define lua_resume lua55_lua_resume
#define lua_rotate lua55_lua_rotate
#define lua_setallocf lua55_lua_setallocf
#define lua_setcstacklimit lua55_lua_setcstacklimit
#define lua_setfield lua55_lua_setfield
#define lua_setglobal lua55_lua_setglobal
#define lua_sethook lua55_lua_sethook
#define lua_seti lua55_lua_seti
#define lua_setiuservalue lua55_lua_setiuservalue
#define lua_setlocal lua55_lua_setlocal
#define lua_setmetatable lua55_lua_setmetatable
#define lua_settable lua55_lua_settable
#define lua_settop lua55_lua_settop
#define lua_setupvalue lua55_lua_setupvalue
#define lua_setwarnf lua55_lua_setwarnf
#define lua_status lua55_lua_status
#define lua_stringtonumber lua55_lua_stringtonumber
#define lua_toboolean lua55_lua_toboolean
#define lua_tocfunction lua55_lua_tocfunction
#define lua_toclose lua55_lua_toclose
#define lua_tointegerx lua55_lua_tointegerx
#define lua_tolstring lua55_lua_tolstring
#define lua_tonumberx lua55_lua_tonumberx
#define lua_topointer lua55_lua_topointer
#define lua_tothread lua55_lua_tothread
#define lua_touserdata lua55_lua_touserdata
#define lua_type lua55_lua_type
#define lua_typename lua55_lua_typename
#define lua_upvalueid lua55_lua_upvalueid
#define lua_upvaluejoin lua55_lua_upvaluejoin
#define lua_version lua55_lua_version
#define lua_warning lua55_lua_warning
#define lua_xmove lua55_lua_xmove
#define lua_yieldk lua55_lua_yieldk
#define luaopen_base lua55_luaopen_base
#define luaopen_coroutine lua55_luaopen_coroutine
#define luaopen_debug lua55_luaopen_debug
#define luaopen_io lua55_luaopen_io
#define luaopen_math lua55_luaopen_math
#define luaopen_os lua55_luaopen_os
#define luaopen_package lua55_luaopen_package
#define luaopen_string lua55_luaopen_string
#define luaopen_table lua55_luaopen_table
#define luaopen_utf8 lua55_luaopen_utf8
#define l_likely(x)	luai_likely(x)
#define l_unlikely(x) luai_unlikely(x)

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

#define LUA_FILE_EXTENSION ".pd_lua"

#include "../pdlua.c"

#ifdef PURR_DATA
#define error lua55_error
#endif

// Fix unistd namespace clash
#define getmode lua55_getmode

#include "lua/onelua.c"
