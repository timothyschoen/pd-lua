/** @file pdlua_gfx.h
 *  @brief pdlua_gfx -- an extension to pdlua that allows GUI rendering and interaction in pure-data and plugdata
 *  @author Timothy Schoen <timschoen123@gmail.com>
 *  @date 2023
 *
 * Copyright (C) 2023 Timothy Schoen <timschoen123@gmail.com>
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

#if !defined(PLUGDATA) && !defined(PURR_DATA)

#define NANOSVG_IMPLEMENTATION
#include "svg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "svg/nanosvgrast.h"
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "svg/stb_image.h"
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "svg/stb_image_write.h"
#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "svg/stb_image_resize2.h"
#endif

#ifdef PURR_DATA

// Port of the vanilla gfx interface to Purr Data. There are some differences
// in the zoom API which Purr Data does directly on the canvas, so we can just
// always assume a zoom factor of 1. Other API differences are dealt with on
// the spot below and in pdlua.c (look for #ifdef/#ifndef PURR_DATA).

#define glist_getzoom(x) 1

// this has an extra argument in vanilla (which we ignore)
static int xxsys_hostfontsize(int fontsize, int zoom)
{
  return sys_hostfontsize(fontsize);
}

#define sys_hostfontsize xxsys_hostfontsize

#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void mylua_error (lua_State *L, t_pdlua *o, const char *descr);

// Functions that need to be implemented separately for each Pd flavour
static int gfx_initialize(t_pdlua *obj);

static int set_size(lua_State *L);
static int get_size(lua_State *L);
static int start_paint(lua_State *L);
static int end_paint(lua_State *L);

static int set_color(lua_State *L);

static int fill_ellipse(lua_State *L);
static int stroke_ellipse(lua_State *L);
static int fill_all(lua_State *L);
static int fill_rect(lua_State *L);
static int stroke_rect(lua_State *L);
static int fill_rounded_rect(lua_State *L);
static int stroke_rounded_rect(lua_State *L);

static int draw_line(lua_State *L);
static int draw_text(lua_State *L);
static int draw_svg(lua_State *L);
static int draw_image(lua_State *L);

static int start_path(lua_State *L);
static int line_to(lua_State *L);
static int quad_to(lua_State *L);
static int cubic_to(lua_State *L);
static int close_path(lua_State *L);
static int stroke_path(lua_State *L);
static int fill_path(lua_State *L);

static int translate(lua_State *L);
static int scale(lua_State *L);
static int reset_transform(lua_State *L);

static int free_path(lua_State *L);

// pdlua_gfx_clear, pdlua_gfx_repaint and pdlua_gfx_mouse_* correspond to the various callbacks the user can assign

static void pdlua_gfx_clear(t_pdlua *obj, int layer, int removed); // only for pd-vanilla, to delete all tcl/tk items

static void pdlua_gfx_free(t_pdlua_gfx *gfx) {
#ifndef PLUGDATA
    for(int i = 0; i < gfx->num_layers; i++)
    {
        freebytes(gfx->layer_tags[i], 64);
    }
    freebytes(gfx->layer_tags, gfx->num_layers);
    if(gfx->transforms) freebytes(gfx->transforms, gfx->num_transforms * sizeof(gfx_transform));
#ifndef PURR_DATA
    for(int i = 0; i < gfx->num_images; i++)
    {
        char image_name[64];
        snprintf(image_name, 64, ".x%llupix%llu", (unsigned long long)gfx, (unsigned long long)gfx->images[i]);
        pdgui_vmess(0, "rrs", "image", "delete", image_name);
    }
    if(gfx->num_images) {
        freebytes(gfx->images, gfx->num_images * sizeof(uint64_t));
        freebytes(gfx->images_last_used, gfx->num_images * sizeof(uint32_t));
    }
#endif
#endif
}

// Trigger repaint callback in lua script
static void pdlua_gfx_repaint(t_pdlua *o, int firsttime) {
#ifndef PLUGDATA
    o->gfx.first_draw = firsttime;
#endif
    lua_getglobal(__L(), "pd");
    lua_getfield (__L(), -1, "_repaint");
    lua_pushlightuserdata(__L(), o);

    if (lua_pcall(__L(), 1, 0, 0))
    {
        mylua_error(__L(), o, "repaint");
    }

    lua_pop(__L(), 1); /* pop the global "pd" */
#ifndef PLUGDATA
    o->gfx.first_draw = 0;
#endif
}

// Pass mouse events to lua script
static void pdlua_gfx_mouse_event(t_pdlua *o, int x, int y, int type) {
    lua_getglobal(__L(), "pd");
    lua_getfield (__L(), -1, "_mouseevent");
    lua_pushlightuserdata(__L(), o);
    lua_pushinteger(__L(), x);
    lua_pushinteger(__L(), y);
    lua_pushinteger(__L(), type);

    if (lua_pcall(__L(), 4, 0, 0))
    {
        mylua_error(__L(), o, "mouseevent");
    }

    lua_pop(__L(), 1); /* pop the global "pd" */
}

// Pass mouse events to lua script (but easier to understand)
static void pdlua_gfx_mouse_down(t_pdlua *o, int x, int y) {
    pdlua_gfx_mouse_event(o, x, y, 0);
}

static void pdlua_gfx_mouse_up(t_pdlua *o, int x, int y) {
    pdlua_gfx_mouse_event(o, x, y, 1);
}

static void pdlua_gfx_mouse_move(t_pdlua *o, int x, int y) {
    pdlua_gfx_mouse_event(o, x, y, 2);
}

static void pdlua_gfx_mouse_drag(t_pdlua *o, int x, int y) {
    pdlua_gfx_mouse_event(o, x, y, 3);
}

static void pdlua_gfx_mouse_enter(t_pdlua *x, int xpos, int ypos) {
    pdlua_gfx_mouse_event(x, xpos, ypos, 4);
}

static void pdlua_gfx_mouse_exit(t_pdlua *x, int xpos, int ypos) {
    pdlua_gfx_mouse_event(x, xpos, ypos, 5);
}

// Represents a path object, created with path.new(x, y)
// for pd-vanilla, this contains all the points that the path contains. bezier curves are flattened out to points before being added
// for plugdata, it only contains a unique ID to the juce::Path that this is mapped to
typedef struct _path_state
{
    // Variables for managing vector paths
    float *path_segments;
    int num_path_segments;
    int num_path_segments_allocated;
    float path_start_x, path_start_y;
} t_path_state;


// Pops the graphics context off the argument list and returns it
static t_pdlua_gfx *pop_graphics_context(lua_State *L)
{
    t_pdlua_gfx **ud = (t_pdlua_gfx**)luaL_checkudata(L, 1, "GraphicsContext");
    lua_remove(L, 1);
    return *ud;
}

// Register functions with Lua
static const luaL_Reg gfx_lib[] = {
    {"set_size", set_size},
    {"get_size", get_size},
    {"start_paint", start_paint},
    {"end_paint", end_paint},
    {NULL, NULL} // Sentinel to end the list
};

static const luaL_Reg path_methods[] = {
    {"line_to", line_to},
    {"quad_to", quad_to},
    {"cubic_to", cubic_to},
    {"close", close_path},
    {"__gc", free_path},
    {NULL, NULL} // Sentinel to end the list
};

// Register functions with Lua
static const luaL_Reg gfx_methods[] = {
    {"set_color", set_color},
    {"fill_ellipse", fill_ellipse},
    {"stroke_ellipse", stroke_ellipse},
    {"fill_rect", fill_rect},
    {"stroke_rect", stroke_rect},
    {"fill_rounded_rect", fill_rounded_rect},
    {"stroke_rounded_rect", stroke_rounded_rect},
    {"draw_line", draw_line},
    {"draw_text", draw_text},
    {"draw_svg", draw_svg},
    {"draw_image", draw_image},
    {"stroke_path", stroke_path},
    {"fill_path", fill_path},
    {"fill_all", fill_all},
    {"translate", translate},
    {"scale", scale},
    {"reset_transform", reset_transform},
    {NULL, NULL} // Sentinel to end the list
};

static int pdlua_gfx_setup(lua_State *L) {
    // for Path(x, y) constructor
    lua_pushcfunction(L, start_path);
    lua_setglobal(L, "Path");

    luaL_newmetatable(L, "Path");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, path_methods, 0);

    luaL_newmetatable(L, "GraphicsContext");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, gfx_methods, 0);

    // Register functions with Lua
    luaL_newlib(L, gfx_lib);
    lua_setglobal(L, "_gfx_internal");

    return 1; // Number of values pushed onto the stack
}

static int get_size(lua_State *L)
{
    if (!lua_islightuserdata(L, 1)) {
        return 0;
    }

    t_pdlua *obj = (t_pdlua*)lua_touserdata(L, 1);
    lua_pushnumber(L, (lua_Number)obj->gfx.width);
    lua_pushnumber(L, (lua_Number)obj->gfx.height);
    return 2;
}

#if PLUGDATA

// we make this global because paths are disconnected from object, but still need to send messages to plugdata
// it really doesn't matter since all these function callbacks point to the same function anyway
static PERTHREAD void(*plugdata_draw_callback)(void*, int, t_symbol*, int, t_atom*) = NULL;

// Wrapper around draw callback to plugdata
static inline void plugdata_draw(t_pdlua *obj, int layer, t_symbol *sym, int argc, t_atom *argv)
{
    if(plugdata_draw_callback) {
        plugdata_draw_callback(obj, layer, sym, argc, argv);
    }
}

static inline void plugdata_draw_args(lua_State *L, const char *sym, const char *fmt, ...)
{
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_atom atoms[16];
    int argc = strlen(fmt);

    va_list defaults;
    va_start(defaults, fmt);

    for (int i = 0; i < argc; i++)
    {
        int lua_idx = i + 1;
        switch (fmt[i])
        {
            case 'f': SETFLOAT (&atoms[i], (t_float)luaL_checknumber(L, lua_idx)); break;
            case 's': SETSYMBOL(&atoms[i], gensym(luaL_checkstring(L, lua_idx))); break;
            case 'F': SETFLOAT (&atoms[i], (t_float)luaL_optnumber(L, lua_idx, va_arg(defaults, double))); break;
            case 'S': SETSYMBOL(&atoms[i], gensym(luaL_optstring(L, lua_idx, va_arg(defaults, const char*)))); break;
            default:  break;
        }
    }

    va_end(defaults);

    if (plugdata_draw_callback) {
        plugdata_draw_callback(gfx->object, gfx->current_layer, gensym(sym), argc, atoms);
    }
}

static void pdlua_gfx_clear(t_pdlua *obj, int layer, int removed) {
}

static int gfx_initialize(t_pdlua *obj)
{
    obj->gfx.object = obj;
    return 0;
}

static int set_size(lua_State *L)
{
    if (!lua_islightuserdata(L, 1))
        return 0;

    t_pdlua *obj = (t_pdlua*)lua_touserdata(L, 1);
    obj->gfx.width = luaL_checknumber(L, 2);
    obj->gfx.height = luaL_checknumber(L, 3);
    t_atom args[2];
    SETFLOAT(args, obj->gfx.width); // w
    SETFLOAT(args + 1, obj->gfx.height); // h
    plugdata_draw(obj, -1, gensym("lua_resized"), 2, args);
    return 0;
}

static int start_paint(lua_State *L) {
    if (!lua_islightuserdata(L, 1)) {
        lua_pushboolean(L, 0); // Return false if the argument is not a pointer
        return 1;
    }
    t_pdlua *obj = (t_pdlua*)lua_touserdata(L, 1);
    int layer = luaL_checknumber(L, 2);

    t_pdlua_gfx **ud = (t_pdlua_gfx**)lua_newuserdata(L, sizeof(t_pdlua_gfx*));
    *ud = &obj->gfx;
    luaL_setmetatable(L, "GraphicsContext");

    plugdata_draw_callback = obj->gfx.plugdata_draw_callback;
    obj->gfx.current_layer = layer;
    obj->gfx.object = obj;
    plugdata_draw(obj, obj->gfx.current_layer, gensym("lua_start_paint"), 0, NULL);
    return 1;
}

static int end_paint(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;
    plugdata_draw(obj, gfx->current_layer, gensym("lua_end_paint"), 0, NULL);
    gfx->current_layer = -1;
    return 0;
}

static int set_color(lua_State *L) {
    if (lua_gettop(L) == 2) { // Single argument: parse as color ID instead of RGB
        plugdata_draw_args(L, "lua_set_color", "f");
        return 0;
    }

    plugdata_draw_args(L, "lua_set_color", "fffF", 1.0);
    return 0;
}

static int fill_ellipse(lua_State *L) {
    plugdata_draw_args(L, "lua_fill_ellipse", "ffff");
    return 0;
}

static int stroke_ellipse(lua_State *L) {
    plugdata_draw_args(L, "lua_stroke_ellipse", "ffffF", 1.0);
    return 0;
}

static int fill_all(lua_State *L) {
    plugdata_draw_args(L, "lua_fill_all", "");
    return 0;
}

static int fill_rect(lua_State *L) {
    plugdata_draw_args(L, "lua_fill_rect", "ffff");
    return 0;
}

static int stroke_rect(lua_State *L) {
    plugdata_draw_args(L, "lua_stroke_rect", "ffffF", 1.0f);
    return 0;
}

static int fill_rounded_rect(lua_State *L) {
    plugdata_draw_args(L, "lua_fill_rounded_rect", "fffff");
    return 0;
}

static int stroke_rounded_rect(lua_State *L) {
    plugdata_draw_args(L, "lua_stroke_rounded_rect", "fffffF", 1.0f);
    return 0;
}

static int draw_line(lua_State *L) {
    plugdata_draw_args(L, "lua_draw_line", "ffffF", 1.0f);
    return 0;
}

static int draw_text(lua_State *L) {
    plugdata_draw_args(L, "lua_draw_text", "sfffFF", 12.f, 0.0f);
    return 0;
}

static int draw_svg(lua_State *L) {
    plugdata_draw_args(L, "lua_draw_svg", "sff");
    return 0;
}

static int draw_image(lua_State *L) {
    plugdata_draw_args(L, "lua_draw_image", "sff");
    return 0;
}

static int stroke_path(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    int stroke_width = luaL_optnumber(L, 2, 1.0f) * glist_getzoom(cnv); // optional, default to 1.0

    int coordinates_size = (2 * path->num_path_segments + 2) * sizeof(t_atom);
    t_atom* coordinates = getbytes(coordinates_size);
    SETFLOAT(coordinates, stroke_width);

    for (int i = 0; i < path->num_path_segments; i++) {
        float x = path->path_segments[i * 2], y = path->path_segments[i * 2 + 1];
        SETFLOAT(coordinates + (i * 2) + 1, x);
        SETFLOAT(coordinates + (i * 2) + 2, y);
    }

    plugdata_draw(gfx->object, gfx->current_layer, gensym("lua_stroke_path"), path->num_path_segments * 2 + 1, coordinates);
    freebytes(coordinates, coordinates_size);

    return 0;
}

static int fill_path(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");

    int coordinates_size = (2 * path->num_path_segments + 2) * sizeof(t_atom);
    t_atom* coordinates = getbytes(coordinates_size);

    for (int i = 0; i < path->num_path_segments; i++) {
        float x = path->path_segments[i * 2], y = path->path_segments[i * 2 + 1];
        SETFLOAT(coordinates + (i * 2), x);
        SETFLOAT(coordinates + (i * 2) + 1, y);
    }

    plugdata_draw(gfx->object, gfx->current_layer, gensym("lua_fill_path"), path->num_path_segments * 2, coordinates);
    freebytes(coordinates, coordinates_size);

    return 0;
}

static int translate(lua_State *L) {
    plugdata_draw_args(L, "lua_translate", "ff");
    return 0;
}

static int scale(lua_State *L) {
    plugdata_draw_args(L, "lua_scale", "ff");
    return 0;
}

static int reset_transform(lua_State *L) {
    plugdata_draw_args(L, "lua_reset_transform", "");
    return 0;
}

#else

static unsigned long long custom_rand() {
    // We use a custom random function to ensure proper randomness across all OS
#ifdef LUA_USE_JIT // Make sure they use different random seeds, to prevent name clashes
    static unsigned long long seed = 1;
#else
    static unsigned long long seed = 0;
#endif
    const unsigned long long a = 1664525;
    const unsigned long long c = 1013904223;
    const unsigned long long m = 4294967296;  // 2^32
    seed = (a * seed + c) % m;
    if(seed == 0) seed = 1; // We cannot return 0 since we use modulo on this. Having the rhs operator of % be zero leads to div-by-zero error on Windows

    return seed;
}

// Generate a new random alphanumeric string to be used as a ID for a tcl/tk drawing
static void generate_random_id(char *str, size_t len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t charset_len = strlen(charset);

    str[0] = '.';
    str[1] = 'x';

    for (size_t i = 2; i < len - 1; ++i) {
        int key = custom_rand() % charset_len;
        str[i] = charset[key];
    }

    str[len - 1] = '\0';
}

#ifndef PURR_DATA
static void pdlua_sweep_image_cache(t_pdlua_gfx *gfx)
{
    int i = 0;
    while (i < gfx->num_images) {
        uint32_t age = gfx->paint_generation - gfx->images_last_used[i];
        if (gfx->num_images > 15 && age > 5) { // If we have too many images, delete all images that havn't been drawn in 5 frames
            char image_name[64];
            snprintf(image_name, 64, ".x%llupix%llu", (unsigned long long)gfx, (unsigned long long)gfx->images[i]);
            pdgui_vmess(0, "rrs", "image", "delete", image_name);

            // Swap with last entry and shrink
            gfx->images[i] = gfx->images[gfx->num_images - 1];
            gfx->images_last_used[i] = gfx->images_last_used[gfx->num_images - 1];

            gfx->images = resizebytes(gfx->images, gfx->num_images * sizeof(uint64_t), (gfx->num_images - 1) * sizeof(uint64_t));
            gfx->images_last_used = resizebytes(gfx->images_last_used, gfx->num_images * sizeof(uint32_t), (gfx->num_images - 1) * sizeof(uint32_t));
            gfx->num_images--;
        } else {
            i++;
        }
    }
}
#endif


static void transform_size(t_pdlua_gfx *gfx, int *w, int *h) {
    for(int i = gfx->num_transforms - 1; i >= 0; i--)
    {
        if(gfx->transforms[i].type == SCALE)
        {
            *w *= gfx->transforms[i].x;
            *h *= gfx->transforms[i].y;
        }
    }
}

static void transform_point(t_pdlua_gfx *gfx, int *x, int *y) {
    for(int i = gfx->num_transforms - 1; i >= 0; i--)
    {
        if(gfx->transforms[i].type == SCALE)
        {
            *x *= gfx->transforms[i].x;
            *y *= gfx->transforms[i].y;
        }
        else // translate
        {
            *x += gfx->transforms[i].x;
            *y += gfx->transforms[i].y;
        }
    }
}

static void transform_size_float(t_pdlua_gfx *gfx, float *w, float *h) {
    for(int i = gfx->num_transforms - 1; i >= 0; i--)
    {
        if(gfx->transforms[i].type == SCALE)
        {
            *w *= gfx->transforms[i].x;
            *h *= gfx->transforms[i].y;
        }
    }
}

static void transform_point_float(t_pdlua_gfx *gfx, float *x, float *y) {
    for(int i = gfx->num_transforms - 1; i >= 0; i--)
    {
        if(gfx->transforms[i].type == SCALE)
        {
            *x *= gfx->transforms[i].x;
            *y *= gfx->transforms[i].y;
        }
        else // translate
        {
            *x += gfx->transforms[i].x;
            *y += gfx->transforms[i].y;
        }
    }
}

#ifdef PURR_DATA
// Purr Data's glist_drawiofor and glist_eraseiofor aren't compatible with
// vanilla's, so they won't work for what we're doing here. We replace them
// with something that's more like the vanilla routines but calling into the
// nw.js GUI.

#define EXTRAPIX 2
#define IOWIDTH 7

#define glist_drawiofor xxglist_drawiofor
#define glist_eraseiofor xxglist_eraseiofor

    /* draw inlets and outlets for a text object or for a graph. */
static void glist_drawiofor(t_glist *glist, t_object *ob, int firsttime,
    char *tag, int x1, int y1, int x2, int y2)
{
  t_canvas *canvas = glist_getcanvas(glist);
  int n = obj_noutlets(ob), nplus = (n == 1 ? 1 : n-1), i;
  int width = x2 - x1;
  int issignal;
  // in purr-data, we don't draw draw iolets on the gop area
  if (canvas != glist) return;
  for (i = 0; i < n; i++) {
    int onset = x1 + (width - IOWIDTH) * i / nplus;
    if (firsttime) {
      issignal = obj_issignaloutlet(ob,i);

      /* need to send issignal and is_iemgui here... */
      gui_vmess("gui_gobj_draw_io", "xssiiiiiisiii",
                canvas,
                tag,
                tag,
                onset,
                y2 - 2,
                onset + IOWIDTH,
                y2,
                x1,
                y1,
                "o",
                i,
                issignal,
                0);
    } else {
      gui_vmess("gui_gobj_redraw_io", "xssiisiii",
                canvas,
                tag,
                tag,
                onset,
                y2 - 2,
                "o",
                i,
                x1,
                y1);
    }
  }
  n = obj_ninlets(ob);
  nplus = (n == 1 ? 1 : n-1);
  for (i = 0; i < n; i++) {
    int onset = x1 + (width - IOWIDTH) * i / nplus;
    if (firsttime) {
      issignal = obj_issignalinlet(ob,i);
      gui_vmess("gui_gobj_draw_io", "xssiiiiiisiii",
                canvas,
                tag,
                tag,
                onset,
                y1,
                onset + IOWIDTH,
                y1 + EXTRAPIX,
                x1,
                y1,
                "i",
                i,
                issignal,
                0);
    } else {
      gui_vmess("gui_gobj_redraw_io", "xssiisiii",
                canvas,
                tag,
                tag,
                onset,
                y1,
                "i",
                i,
                x1,
                y1);
    }
  }
}

static void glist_eraseiofor(t_glist *glist, t_object *ob, char *tag)
{
  char tagbuf[MAXPDSTRING];
  t_canvas *canvas = glist_getcanvas(glist);
  int i, n;
  if (canvas != glist) return;
  n = obj_noutlets(ob);
  for (i = 0; i < n; i++) {
    sprintf(tagbuf, "%so%d", tag, i);
    gui_vmess("gui_gobj_erase_io", "xs", canvas, tagbuf);
  }
  n = obj_ninlets(ob);
  for (i = 0; i < n; i++) {
    sprintf(tagbuf, "%si%d", tag, i);
    gui_vmess("gui_gobj_erase_io", "xs", canvas, tagbuf);
  }
}
#endif

static void pdlua_gfx_clear(t_pdlua *obj, int layer, int removed) {
    t_pdlua_gfx *gfx = &obj->gfx;
    t_canvas *cnv = glist_getcanvas(obj->canvas);
#ifndef PURR_DATA

    if(layer < gfx->num_layers) {
        pdgui_vmess(0, "crs", cnv, "delete", layer == -1 ? gfx->object_tag : gfx->layer_tags[layer]);
    }

    if(removed && gfx->order_tag[0] != '\0')
    {
        pdgui_vmess(0, "crs", cnv, "delete", gfx->order_tag);
        gfx->order_tag[0] = '\0';
    }
#else // PURR_DATA
    if (removed) {
        // nuke the gobj container, this gets rid of everything
        gui_vmess("gui_luagfx_erase", "xs", cnv, gfx->object_tag);
    } else if (layer == -1) {
        // this clears all layers
        for (int l = 0; l < gfx->num_layers; l++)
            gui_vmess("gui_luagfx_clear", "xs", cnv, gfx->layer_tags[l]);
    } else {
        // this only clears the specified layer
        gui_vmess("gui_luagfx_clear", "xs", cnv, gfx->layer_tags[layer]);
    }
#endif

    glist_eraseiofor(obj->canvas, &obj->pd, gfx->object_tag);
}

static void get_bounds_args(lua_State *L, t_pdlua *obj, int *x1, int *y1, int *x2, int *y2) {
    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x = luaL_checknumber(L, 1);
    int y = luaL_checknumber(L, 2);
    int w = luaL_checknumber(L, 3);
    int h = luaL_checknumber(L, 4);

    transform_point(&obj->gfx, &x, &y);
    transform_size(&obj->gfx, &w, &h);

    x += text_xpix((t_object*)obj, obj->canvas) / glist_getzoom(cnv);
    y += text_ypix((t_object*)obj, obj->canvas) / glist_getzoom(cnv);

    *x1 = x * glist_getzoom(cnv);
    *y1 = y * glist_getzoom(cnv);
    *x2 = (x + w) * glist_getzoom(cnv);
    *y2 = (y + h) * glist_getzoom(cnv);
}

static void gfx_displace(t_pdlua *x, t_glist *glist, int dx, int dy)
{
#ifndef PURR_DATA
    char obj_name[32];
    snprintf(obj_name, 32 ,".x%lx", (long)x);
    pdgui_vmess(0, "crs ii", glist_getcanvas(x->canvas), "move", obj_name, dx, dy);
#else
    gui_vmess("gui_text_displace", "xsii", glist_getcanvas(x->canvas), x->gfx.object_tag, dx, dy);
#endif
    canvas_fixlinesfor(glist, (t_text*)x);

    int scale = glist_getzoom(glist_getcanvas(x->canvas));

    int xpos = text_xpix((t_object*)x, x->canvas);
    int ypos = text_ypix((t_object*)x, x->canvas);
    glist_drawiofor(x->canvas, (t_object*)x, 0, x->gfx.object_tag, xpos, ypos, xpos + (x->gfx.width * scale), ypos + (x->gfx.height * scale));
}

static const char *register_drawing(t_pdlua_gfx *gfx)
{
    generate_random_id(gfx->current_item_tag, 64);
    return gfx->current_item_tag;
}

static int gfx_initialize(t_pdlua *obj)
{
    t_pdlua_gfx *gfx = &obj->gfx;

#ifndef PURR_DATA
    snprintf(gfx->object_tag, 128, ".x%lx", (long)obj);
    gfx->object_tag[127] = '\0';
#else // PURR_DATA
    // deferred until the object is fully initialized
    strcpy(gfx->object_tag, "*");
#endif
    gfx->order_tag[0] = '\0';
    gfx->object = obj;
    gfx->transforms = NULL;
    gfx->num_transforms = 0;
    gfx->num_layers = 0;
    gfx->first_draw = 0;
    gfx->layer_tags = NULL;
    gfx->mouse_inside = 0;

    return 0;
}

static int set_size(lua_State *L)
{
    if (!lua_islightuserdata(L, 1))
        return 0;

    t_pdlua *obj = (t_pdlua*)lua_touserdata(L, 1);
    obj->gfx.width = luaL_checknumber(L, 2);
    obj->gfx.height = luaL_checknumber(L, 3);
    pdlua_gfx_repaint(obj, 0);
    if(glist_isvisible(obj->canvas) && gobj_shouldvis(&obj->pd.te_g, obj->canvas)) {
        canvas_fixlinesfor(obj->canvas, (t_text*)obj);
    }
    return 0;
}

static int start_paint(lua_State *L) {
    if (!lua_islightuserdata(L, 1)) {
        lua_pushnil(L);
        return 1;
    }

    t_pdlua *obj = (t_pdlua*)lua_touserdata(L, 1);

    t_pdlua_gfx *gfx = &obj->gfx;
    if(gfx->object_tag[0] == '\0')
    {
        lua_pushnil(L);
        return 1;
    }

#ifdef PURR_DATA
    if (gfx->object_tag[0] == '*') {
      // late initialization, deferred until glist_findrtext will work
      t_rtext *y = glist_findrtext(obj->canvas, (t_text*)obj);
      if (y) {
        const char *s = rtext_gettag(y);
        strcpy(gfx->object_tag, s);
      } else {
        // this shouldn't happen, but if it does, we fall back to the
        // vanilla-style tag
        snprintf(gfx->object_tag, 128, ".x%lx", (long)obj);
        gfx->object_tag[127] = '\0';
      }
    }
#endif

    // Check if:
    // 1. The canvas and object are visible
    // 2. This is the first repaint since "vis" was called
    // If neither are true, we are not allowed to draw because the targeted tcl/tk canvas is not visible
    int can_draw = (glist_isvisible(obj->canvas) && gobj_shouldvis(&obj->pd.te_g, obj->canvas)) || obj->gfx.first_draw;
    if(can_draw)
    {
        int layer = luaL_checknumber(L, 2) - 1;
#ifndef PURR_DATA
        if(layer > gfx->num_layers) // If we get here, we have skipped a layer. This isn't allowed, so we should instead repaint everything
        {
            pdlua_gfx_repaint(obj, 0);
            lua_pushnil(L);
            return 1;
        }
        else if(layer >= gfx->num_layers)
        {
            int new_num_layers = layer + 1;
            if(gfx->layer_tags)
                gfx->layer_tags = resizebytes(gfx->layer_tags, sizeof(char*) * gfx->num_layers, sizeof(char*) * new_num_layers);
            else
                gfx->layer_tags = getbytes(sizeof(char*));

            gfx->layer_tags[layer] = getbytes(64);
            snprintf(gfx->layer_tags[layer], 64, ".l%i%lx", layer, (long)obj);
            gfx->num_layers = new_num_layers;
        }
#else // PURR_DATA
        // We need to defer the actual layer creation until later, since at
        // this point we may not have created the gobj yet.
        int old_num_layers = gfx->num_layers, new_num_layers = layer + 1;
        if(layer >= gfx->num_layers)
        {
            if(gfx->layer_tags)
                gfx->layer_tags = resizebytes(gfx->layer_tags, sizeof(char*) * gfx->num_layers, sizeof(char*) * new_num_layers);
            else
                gfx->layer_tags = getbytes(sizeof(char*));

            for (int l = old_num_layers; l < new_num_layers; l++) {
                gfx->layer_tags[l] = getbytes(64);
                snprintf(gfx->layer_tags[l], 64, ".l%i%lx", l, (long)obj);
            }
            gfx->num_layers = new_num_layers;
        }
#endif
        gfx->current_layer_tag = gfx->layer_tags[layer];

        if(gfx->transforms) freebytes(gfx->transforms, gfx->num_transforms * sizeof(gfx_transform));
        gfx->num_transforms = 0;
        gfx->transforms = NULL;

        t_pdlua_gfx **ud = (t_pdlua_gfx**)lua_newuserdata(L, sizeof(t_pdlua_gfx*));
        *ud = &obj->gfx;
        luaL_setmetatable(L, "GraphicsContext");

#ifndef PURR_DATA
        // clear anything that was painted before
        if(strlen(gfx->object_tag)) pdlua_gfx_clear(obj, layer, 0);

        if(gfx->first_draw)
        {
            // Whenever the objects gets painted for the first time with a "vis" message,
            // we add a small invisible line that won't get touched or repainted later.
            // We can then use this line to set the correct z-index for the drawings, using the tcl/tk "lower" command
            t_canvas *cnv = glist_getcanvas(obj->canvas);
            generate_random_id(gfx->order_tag, 64);

            const char *tags[] = { gfx->order_tag };
            pdgui_vmess(0, "crr iiii ri rS", cnv, "create", "line", 0, 0, 0, 0,
                        "-width", 1, "-tags", 1, tags);
        }
#else // PURR_DATA
        t_canvas *cnv = glist_getcanvas(obj->canvas);
        if(gfx->first_draw) {
            int xpos = text_xpix((t_object*)obj, obj->canvas);
            int ypos = text_ypix((t_object*)obj, obj->canvas);
            // create a gobj graphics container in the GUI
            gui_vmess("gui_luagfx_new", "xsiiiii", cnv, gfx->object_tag,
                      xpos, ypos, glist_istoplevel(obj->canvas));
            for (int l = 0; l < old_num_layers; l++) {
                // re-create old graphics layers in the GUI
                gui_vmess("gui_luagfx_new_layer", "xss", cnv,
                          gfx->object_tag,
                          gfx->layer_tags[l]);
            }
        }
        if (strlen(gfx->object_tag))
            for (int l = old_num_layers; l < new_num_layers; l++) {
                // create a new graphics layer in the GUI
                gui_vmess("gui_luagfx_new_layer", "xss", cnv, gfx->object_tag,
                          gfx->layer_tags[l]);
            }
        if (!gfx->first_draw && strlen(gfx->object_tag))
            pdlua_gfx_clear(obj, layer, 0);
#endif

        return 1;
    }

    lua_pushnil(L);
    return 1;
}

static int end_paint(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = (t_pdlua*)gfx->object;
    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int scale = glist_getzoom(glist_getcanvas(obj->canvas));
    int layer = luaL_checknumber(L, 1) - 1;

    // Draw iolets on top
    int xpos = text_xpix((t_object*)obj, obj->canvas);
    int ypos = text_ypix((t_object*)obj, obj->canvas);

    // TODO: I don't think we need to call drawiofor on each layer?
    glist_drawiofor(obj->canvas, (t_object*)obj, 1, gfx->object_tag, xpos, ypos, xpos + (gfx->width * scale), ypos + (gfx->height * scale));

#ifndef PURR_DATA
    if(!gfx->first_draw && gfx->order_tag[0] != '\0') {

        // Move everything to below the order marker, to make sure redrawn stuff isn't always on top
        pdgui_vmess(0, "crss", cnv, "lower", gfx->object_tag, gfx->order_tag);

        if(layer == 0 && gfx->num_layers > 1)
        {
            if(layer < gfx->num_layers) pdgui_vmess(0, "crss", cnv, "lower", gfx->current_layer_tag, gfx->layer_tags[layer + 1]);
        }
        else if(layer != 0) {
            pdgui_vmess(0, "crss", cnv, "raise", gfx->current_layer_tag, gfx->layer_tags[layer - 1]);
        }
    }
    if(layer == 0) gfx->paint_generation++; // Currently, image cleanup only happens on a full repaint. This could be improved
    pdlua_sweep_image_cache(gfx);
#endif

    return 0;
}

static int set_color(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);

    int r = luaL_checknumber(L, 1);
    int g = luaL_checknumber(L, 2);
    int b = luaL_checknumber(L, 3);
#ifndef PURR_DATA
    // AFAIK, alpha is not supported in tcl/tk
    snprintf(gfx->current_color, 8, "#%02X%02X%02X", r, g, b);
    gfx->current_color[7] = '\0';
#else
    // ... but it is in Purr Data (nw.js gui)
    int a = luaL_optnumber(L, 4, 1.0f) * 255;
    snprintf(gfx->current_color, 10, "#%02X%02X%02X%02X", r, g, b, a);
    gfx->current_color[9] = '\0';
#endif

    return 0;
}

static int fill_ellipse(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1, y1, x2, y2;
    get_bounds_args(L, obj, &x1, &y1, &x2, &y2);

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii rs ri rS", cnv, "create", "oval", x1, y1, x2, y2, "-fill", gfx->current_color, "-width", 0, "-tags", 3, tags);
#else // PURR_DATA
    // in Purr Data, the coordinates of the graphical objects are all relative
    // to the gobj container
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_fill_ellipse", "xsssiiiii", cnv, tags[2], tags[1],
              gfx->current_color, 0,
              x1-x0, y1-y0, x2-x0, y2-y0);
#endif

    return 0;
}

static int stroke_ellipse(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1, y1, x2, y2;
    get_bounds_args(L, obj, &x1, &y1, &x2, &y2);

    int line_width = luaL_optnumber(L, 5, 1.0f) * glist_getzoom(cnv); // stroke width (optional, default to 1.0)

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "oval", x1, y1, x2, y2, "-width", line_width, "-outline", gfx->current_color, "-tags", 3, tags);
#else // PURR_DATA
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_stroke_ellipse", "xsssiiiii", cnv, tags[2], tags[1],
              gfx->current_color, line_width,
              x1-x0, y1-y0, x2-x0, y2-y0);
#endif

    return 0;
}

static int fill_all(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1 = text_xpix((t_object*)obj, obj->canvas);
    int y1 = text_ypix((t_object*)obj, obj->canvas);
    int x2 = x1 + gfx->width * glist_getzoom(cnv);
    int y2 = y1 + gfx->height * glist_getzoom(cnv);

    const char *tags[] =  { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii rs rk ri rS", cnv, "create", "rectangle", x1, y1, x2, y2, "-fill", gfx->current_color, "-outline", gfx->is_selected ? THISGUI->i_selectcolor : THISGUI->i_foregroundcolor, "-width",     glist_getzoom(cnv), "-tags", 3, tags);
#else // PURR_DATA
    gui_vmess("gui_luagfx_fill_all", "xsssiiii", cnv, tags[2], tags[1],
              gfx->current_color,
              0, 0, x2-x1, y2-y1);
#endif

    return 0;
}

static int fill_rect(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1, y1, x2, y2;
    get_bounds_args(L, obj, &x1, &y1, &x2, &y2);

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii rs ri rS", cnv, "create", "rectangle", x1, y1, x2, y2, "-fill", gfx->current_color, "-width", 0, "-tags", 3, tags);
#else // PURR_DATA
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_fill_rect", "xsssiiiii", cnv, tags[2], tags[1],
              gfx->current_color, 0,
              x1-x0, y1-y0, x2-x0, y2-y0);
#endif

    return 0;
}

static int stroke_rect(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1, y1, x2, y2;
    get_bounds_args(L, obj, &x1, &y1, &x2, &y2);

    int line_width = luaL_optnumber(L, 5, 1.0f) * glist_getzoom(cnv); // stroke width (optional, default to 1.0)

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "rectangle", x1, y1, x2, y2, "-width", line_width, "-outline", gfx->current_color, "-tags", 3, tags);
#else // PURR_DATA
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_stroke_rect", "xsssiiiii", cnv, tags[2], tags[1],
              gfx->current_color, line_width,
              x1-x0, y1-y0, x2-x0, y2-y0);
#endif

    return 0;
}

static int fill_rounded_rect(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1, y1, x2, y2;
    get_bounds_args(L, obj, &x1, &y1, &x2, &y2);

    int radius = luaL_checknumber(L, 5);  // Radius for rounded corners
    int radius_x = radius * glist_getzoom(cnv);
    int radius_y = radius * glist_getzoom(cnv);

    transform_size(gfx, &radius_x, &radius_y);

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    // Tcl/tk can't fill rounded rectangles, so we draw 2 smaller rectangles with 4 ovals over the corners
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "oval", x1, y1, x1 + radius_x * 2, y1 + radius_y * 2, "-width", 0, "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "oval", x2 - radius_x * 2 , y1, x2, y1 + radius_y * 2, "-width", 0, "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "oval", x1, y2 - radius_y * 2, x1 + radius_x * 2, y2, "-width", 0, "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "oval", x2 - radius_x * 2, y2 - radius_y * 2, x2, y2, "-width", 0, "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "rectangle", x1 + radius_x, y1, x2 - radius_x, y2, "-width", 0, "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "rectangle", x1, y1 + radius_y, x2, y2 - radius_y, "-width", 0, "-fill", gfx->current_color, "-tags", 3, tags);
#else // PURR_DATA
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_fill_rounded_rect", "xsssiiiiiii", cnv, tags[2], tags[1],
              gfx->current_color, 0,
              x1-x0, y1-y0, x2-x0, y2-y0,
              radius_x, radius_y);
#endif

    return 0;
}

static int stroke_rounded_rect(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1, y1, x2, y2;
    get_bounds_args(L, obj, &x1, &y1, &x2, &y2);

    int radius = luaL_checknumber(L, 5);       // Radius for rounded corners
    int radius_x = radius * glist_getzoom(cnv);
    int radius_y = radius * glist_getzoom(cnv);
    transform_size(gfx, &radius_x, &radius_y);
    int line_width = luaL_optnumber(L, 6, 1.0f) * glist_getzoom(cnv); // stroke width (optional, default to 1.0)

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    // Tcl/tk can't stroke rounded rectangles either, so we draw 2 lines connecting with 4 arcs at the corners
    pdgui_vmess(0, "crr iiii ri ri ri ri rs rs rS", cnv, "create", "arc", x1, y1 + radius_y*2, x1 + radius_x*2, y1,
                "-start", 0, "-extent", 90, "-width", line_width, "-start", 90, "-outline", gfx->current_color, "-style", "arc", "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri ri ri ri rs rs rS", cnv, "create", "arc", x2 - radius_x*2, y1, x2, y1 + radius_y*2,
                "-start", 270, "-extent", 90, "-width", line_width, "-start", 0, "-outline", gfx->current_color, "-style", "arc", "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri ri ri ri rs rs rS", cnv, "create", "arc", x1, y2 - radius_y*2, x1 + radius_x*2, y2,
                "-start", 180, "-extent", 90, "-width", line_width, "-start", 180, "-outline", gfx->current_color, "-style", "arc", "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri ri ri ri rs rs rS", cnv, "create", "arc", x2 - radius_x*2, y2, x2, y2 - radius_y*2,
                "-start", 90, "-extent", 90, "-width", line_width, "-start", 270, "-outline", gfx->current_color, "-style", "arc", "-tags", 3, tags);

    // Connect with lines
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "line", x1 + radius_x, y1, x2 - radius_x, y1,
                "-width", line_width, "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "line", x1 + radius_y, y2, x2 - radius_y, y2,
                "-width", line_width,  "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "line", x1 , y1 + radius_y, x1, y2 - radius_y,
                "-width", line_width, "-fill", gfx->current_color, "-tags", 3, tags);
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "line", x2 , y1 + radius_y, x2, y2 - radius_y,
                "-width", line_width,  "-fill", gfx->current_color, "-tags", 3, tags);
#else // PURR_DATA
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_stroke_rounded_rect", "xsssiiiiiii", cnv, tags[2], tags[1],
              gfx->current_color, line_width,
              x1-x0, y1-y0, x2-x0, y2-y0,
              radius_x, radius_y);
#endif

    return 0;
}

static int draw_line(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    int x1 = luaL_checknumber(L, 1);
    int y1 = luaL_checknumber(L, 2);
    int x2 = luaL_checknumber(L, 3);
    int y2 = luaL_checknumber(L, 4);
    int line_width = luaL_optnumber(L, 5, 1.0f); // line width (optional, default to 1.0)

    transform_point(gfx, &x1, &y1);
    transform_point(gfx, &x2, &y2);

    int canvas_zoom = glist_getzoom(cnv);

    x1 += text_xpix((t_object*)obj, obj->canvas) / canvas_zoom;
    y1 += text_ypix((t_object*)obj, obj->canvas) / canvas_zoom;
    x2 += text_xpix((t_object*)obj, obj->canvas) / canvas_zoom;
    y2 += text_ypix((t_object*)obj, obj->canvas) / canvas_zoom;

    x1 *= canvas_zoom;
    y1 *= canvas_zoom;
    x2 *= canvas_zoom;
    y2 *= canvas_zoom;
    line_width *= canvas_zoom;

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "line", x1, y1, x2, y2,
                "-width", line_width, "-fill", gfx->current_color, "-tags", 3, tags);
#else // PURR_DATA
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_draw_line", "xsssiiiii", cnv, tags[2], tags[1],
              gfx->current_color, line_width,
              x1-x0, y1-y0, x2-x0, y2-y0);
#endif

    return 0;
}

static int draw_text(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    const char *text = luaL_checkstring(L, 1); // Assuming text is a string
    int x = luaL_checknumber(L, 2);
    int y = luaL_checknumber(L, 3);
    int w = luaL_checknumber(L, 4);
    // default to standard font size
    int font_height = sys_hostfontsize(luaL_optnumber(L, 5, glist_getfont(cnv)), glist_getzoom(cnv));
    int alignment = luaL_optinteger(L, 6, 0); // Defaults to TOP_LEFT

    transform_point(gfx, &x, &y);
    transform_size(gfx, &w, &font_height);

    int canvas_zoom = glist_getzoom(cnv);
    x += text_xpix((t_object*)obj, obj->canvas) / canvas_zoom;
    y += text_ypix((t_object*)obj, obj->canvas) / canvas_zoom;

    x *= canvas_zoom;
    y *= canvas_zoom;
    w *= canvas_zoom;

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    // Convert alignment value to tcl/tk anchor point
    const char *anchor;
    switch (alignment) {
        case 1:  anchor = "n"; break;      // TOP_CENTER
        case 2:  anchor = "ne"; break;     // TOP_RIGHT
        case 3:  anchor = "w"; break;      // CENTER_LEFT
        case 4:  anchor = "center"; break; // CENTER
        case 5:  anchor = "e"; break;      // CENTER_RIGHT
        case 6:  anchor = "sw"; break;     // BOTTOM_LEFT
        case 7:  anchor = "s"; break;      // BOTTOM_CENTER
        case 8:  anchor = "se"; break;     // BOTTOM_RIGHT
        default: anchor = "nw"; break;     // TOP_LEFT
    }

    pdgui_vmess(0, "crr ii rs ri rs rS", cnv, "create", "text",
                0, 0, "-anchor", anchor, "-width", w, "-text", text, "-tags", 3, tags);

    t_atom fontatoms[3];
    SETSYMBOL(fontatoms+0, gensym(sys_font));
    SETFLOAT (fontatoms+1, -font_height); // Size is wrong on hi-dpi Windows if this is not negative
    SETSYMBOL(fontatoms+2, gensym(sys_fontweight));

    pdgui_vmess(0, "crs rA rs rs", cnv, "itemconfigure", tags[1],
            "-font", 3, fontatoms,
            "-fill", gfx->current_color,
            "-justify", "left");

    pdgui_vmess(0, "crs ii", cnv, "coords", tags[1], x, y);
#else // PURR_DATA
    int x0 = text_xpix((t_object*)obj, obj->canvas);
    int y0 = text_ypix((t_object*)obj, obj->canvas);
    gui_vmess("gui_luagfx_draw_text", "xsssiiiis", cnv, tags[2], tags[1],
              gfx->current_color, w, font_height, x-x0, y-y0, text);
#endif

    return 0;
}

// Create single hash of svg text and render scale
static uint64_t pdlua_image_hash(unsigned char *str, float scale)
{
    uint64_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    union { float f; uint32_t i; } u;
    u.f = scale;
    return hash ^ (u.i * 0x9E3779B9);
}

static char *pdlua_base64_encode(const unsigned char *data,
                    size_t input_length) {

    static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                    'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                    '4', '5', '6', '7', '8', '9', '+', '/'};

    static int mod_table[] = {0, 2, 1};

    size_t output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(output_length+1);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data[output_length] = '\0';
    return encoded_data;
}

static int draw_svg(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);
    int canvas_zoom = glist_getzoom(cnv);

    // We can only apply scaling with an equal aspect ratio, so we only use the first scale coordinate
    float scale_x = canvas_zoom, scale_y = canvas_zoom;
    transform_size_float(gfx, &scale_x, &scale_y);
    float scale = (scale_x + scale_y) * 0.5f;

    char *svg_text = strdup(luaL_checkstring(L, 1));
    uint64_t svg_hash = pdlua_image_hash((unsigned char*)svg_text, scale);

    int x = luaL_checknumber(L, 2);
    int y = luaL_checknumber(L, 3);

    transform_point(gfx, &x, &y);

    x += text_xpix((t_object*)obj, obj->canvas) / canvas_zoom;
    y += text_ypix((t_object*)obj, obj->canvas) / canvas_zoom;

    x *= canvas_zoom;
    y *= canvas_zoom;

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    // See if we already rendered the same svg text at the same size, if so, reuse that image
    for(int i = 0; i < gfx->num_images; i++)
    {
        if(gfx->images[i] == svg_hash)
        {
            char image_name[64];
            snprintf(image_name, 64, ".x%llupix%llu", (unsigned long long)gfx, (unsigned long long)svg_hash);
            pdgui_vmess(0, "crr ii rs rr rS", cnv, "create", "image", x, y, "-image", image_name, "-anchor", "nw", "-tags", 3, tags);
            gfx->images_last_used[i] = gfx->paint_generation;
            return 0;
        }
    }

    // First parse svg text with nanosvg
    struct NSVGimage *image = nsvgParse(svg_text, "px", 96);
    if (!image) {
        pd_error(0, "[pdlua] draw_svg: Failed to parse SVG data.");
        return 0;
    }

    // Then rasterize to a bitmap image
    struct NSVGrasterizer *rast = nsvgCreateRasterizer();
    if (!rast) {
        pd_error(0, "[pdlua] draw_svg: Failed to create rasterizer.");
        return 0;
    }

    const int channels = 4;
    // Apply scale, limit size to object size
    // This is not perfect clipping, but it at least prevents accidental large images from freezing pd
    int w = (int)fmax(image->width * scale, gfx->width * canvas_zoom);
    int h = (int)fmax(image->height* scale, gfx->height * canvas_zoom);
    int image_size = w * h * channels;

    unsigned char *bitmap_data = getbytes(image_size);
    if (!bitmap_data) {
        pd_error(0, "[pdlua] draw_svg: Failed to allocate memory for bitmap.");
        return 0;
    }

    nsvgRasterize(rast, image, 0, 0, scale, bitmap_data, w, h, w * channels);

    // Convert bitmap data to png
    int png_size;
    unsigned char *png_buf = stbi_write_png_to_mem(bitmap_data, w * channels, w, h, channels, &png_size);
    if (!png_buf || png_size <= 0) {
        pd_error(0, "[pdlua] draw_svg: Failed to encode PNG image.");
        return 0;
    }

    // Encode PNG data to Base64
    char *encoded_png = pdlua_base64_encode((unsigned char*)png_buf, png_size);
    free(png_buf);

    if (!encoded_png) {
        pd_error(0, "[pdlua] draw_svg: Failed to encode PNG to Base64.");
        return 0;
    }

    // Write entry to image hash table
    if(gfx->num_images == 0)
    {
        gfx->images = getbytes(sizeof(uint64_t));
        gfx->images_last_used = getbytes(sizeof(uint32_t));
    }
    else {
        gfx->images = resizebytes(gfx->images, gfx->num_images*sizeof(uint64_t), (gfx->num_images+1) * sizeof(uint64_t));
        gfx->images_last_used = resizebytes(gfx->images_last_used, gfx->num_images*sizeof(uint32_t), (gfx->num_images+1) * sizeof(uint32_t));
    }

    gfx->images[gfx->num_images] = svg_hash;
    gfx->images_last_used[gfx->num_images] = gfx->paint_generation;
    gfx->num_images++;

    char image_name[64];
    snprintf(image_name, 64, ".x%llupix%llu", (unsigned long long)gfx, (unsigned long long)svg_hash);
    pdgui_vmess(0, "rrr s rs", "image", "create", "photo", image_name, "-data", encoded_png);
    pdgui_vmess(0, "crr ii rs rr rS", cnv, "create", "image", x, y, "-image", image_name, "-anchor", "nw", "-tags", 3, tags);

    // Cleanup
    free(encoded_png);
    free(svg_text);
    freebytes(bitmap_data, image_size);
#else // PURR_DATA
    // TODO: implement for purr-data, probably just send the svg text over?
#endif
    return 0;
}

static int draw_image(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);
    int canvas_zoom = glist_getzoom(cnv);

    float scale_x = canvas_zoom, scale_y = canvas_zoom;
    transform_size_float(gfx, &scale_x, &scale_y);
    float scale = (scale_x + scale_y) * 0.5f;

    const char *image_path = luaL_checkstring(L, 1);
    int x = luaL_checknumber(L, 2);
    int y = luaL_checknumber(L, 3);

    uint64_t image_hash = pdlua_image_hash((unsigned char*)image_path, scale);

    transform_point(gfx, &x, &y);
    x += text_xpix((t_object*)obj, obj->canvas) / canvas_zoom;
    y += text_ypix((t_object*)obj, obj->canvas) / canvas_zoom;
    x *= canvas_zoom;
    y *= canvas_zoom;

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

    char image_name[64];
    snprintf(image_name, 64, ".x%llupix%llu", (unsigned long long)gfx, (unsigned long long)image_hash);

#ifndef PURR_DATA
    // Fast path: scaled image already uploaded to Tk
    for (int i = 0; i < gfx->num_images; i++) {
        if (gfx->images[i] == image_hash) {
            pdgui_vmess(0, "crr ii rs rr rS", cnv, "create", "image", x, y,
                        "-image", image_name, "-anchor", "nw", "-tags", 3, tags);
            gfx->images_last_used[i] = gfx->paint_generation;
            return 0;
        }
    }

    // Locate the file through Pd's search path
    char dirresult[MAXPDSTRING];
    char *nameresult = NULL;
    int fd = canvas_open(obj->canvas, image_path, "",
                         dirresult, &nameresult, MAXPDSTRING, 0);
    if (fd < 0) {
        pd_error(obj, "[pdlua] draw_image: cannot open '%s'", image_path);
        return 0;
    }

    FILE *fp = fdopen(fd, "rb");
    if (!fp) {
        pd_error(obj, "[pdlua] draw_image: fdopen failed");
        sys_close(fd);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        pd_error(obj, "[pdlua] draw_image: empty file '%s'", image_path);
        fclose(fp);
        return 0;
    }

    unsigned char *file_data = getbytes((size_t)file_size);
    if (!file_data) {
        pd_error(obj, "[pdlua] draw_image: out of memory");
        fclose(fp);
        return 0;
    }

    if ((long)fread(file_data, 1, (size_t)file_size, fp) != file_size) {
        pd_error(obj, "[pdlua] draw_image: read error for '%s'", image_path);
        freebytes(file_data, (size_t)file_size);
        fclose(fp);
        return 0;
    }
    fclose(fp);

    // Decode image with stb_image (handles PNG, JPG, GIF, BMP, etc)
    int src_w, src_h, channels;
    unsigned char *src_pixels = stbi_load_from_memory(file_data, (int)file_size,
                                                       &src_w, &src_h, &channels, 4);
    freebytes(file_data, (size_t)file_size);

    if (!src_pixels) {
        pd_error(obj, "[pdlua] draw_image: stb_image decode failed for '%s': %s",
                 image_path, stbi_failure_reason());
        return 0;
    }

    // Compute scaled dimensions
    int dst_w = (int)(src_w * scale + 0.5f);
    int dst_h = (int)(src_h * scale + 0.5f);
    if (dst_w < 1) dst_w = 1;
    if (dst_h < 1) dst_h = 1;

    unsigned char *dst_pixels = NULL;

    if (dst_w == src_w && dst_h == src_h) {
        // No resize needed, use source directly
        dst_pixels = src_pixels;
    } else {
        dst_pixels = getbytes(dst_w * dst_h * 4);
        if (!dst_pixels) {
            pd_error(obj, "[pdlua] draw_image: out of memory for resized buffer");
            stbi_image_free(src_pixels);
            return 0;
        }

        // stbir_resize_uint8_linear gives a proper box/bilinear filtered result
        stbir_resize_uint8_linear(src_pixels, src_w, src_h, 0,
                                   dst_pixels, dst_w, dst_h, 0, STBIR_RGBA);
        stbi_image_free(src_pixels);
    }

    // Encode to PNG → Base64 → Tk photo image (same path as draw_svg)
    int png_size;
    unsigned char *png_buf = stbi_write_png_to_mem(dst_pixels, dst_w * 4,
                                                    dst_w, dst_h, 4, &png_size);
    if (dst_pixels != src_pixels)
        freebytes(dst_pixels, dst_w * dst_h * 4);
    else
        stbi_image_free(src_pixels);

    if (!png_buf || png_size <= 0) {
        pd_error(obj, "[pdlua] draw_image: PNG encode failed");
        return 0;
    }

    char *encoded = pdlua_base64_encode(png_buf, (size_t)png_size);
    free(png_buf);

    if (!encoded) {
        pd_error(obj, "[pdlua] draw_image: base64 encode failed");
        return 0;
    }

    pdgui_vmess(0, "rrr s rs",  "image", "create", "photo", image_name, "-data", encoded);
    pdgui_vmess(0, "crr ii rs rr rS", cnv, "create", "image", x, y,
                "-image", image_name, "-anchor", "nw", "-tags", 3, tags);
    free(encoded);

    // Cache the scaled hash
    if (gfx->num_images == 0) {
        gfx->images = getbytes(sizeof(uint64_t));
        gfx->images_last_used = getbytes(sizeof(uint32_t));
    }
    else {
        gfx->images = resizebytes(gfx->images, gfx->num_images * sizeof(uint64_t), (gfx->num_images + 1) * sizeof(uint64_t));
        gfx->images_last_used = resizebytes(gfx->images_last_used, gfx->num_images * sizeof(uint32_t), (gfx->num_images + 1) * sizeof(uint32_t));
    }

    gfx->images_last_used[gfx->num_images] = gfx->paint_generation;
    gfx->images[gfx->num_images] = image_hash;
    gfx->num_images++;

#else
    // TODO: purr-data
#endif
    return 0;
}

static int stroke_path(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    if(path->num_path_segments < 3)
        return 0;

    int stroke_width = luaL_optnumber(L, 2, 1.0f) * glist_getzoom(cnv); // stroke width (optional, default to 1.0)
    int obj_x = text_xpix((t_object*)obj, obj->canvas);
    int obj_y = text_ypix((t_object*)obj, obj->canvas);
    int canvas_zoom = glist_getzoom(cnv);

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "line", 0, 0, 0, 0, "-width", stroke_width, "-fill", gfx->current_color, "-tags", 3, tags);

    t_float *transformed_coordinates = getbytes(path->num_path_segments * 2 * sizeof(t_float));
    for (int i = 0; i < path->num_path_segments; i++) {
        float x =  path->path_segments[i * 2], y = path->path_segments[i * 2 + 1];
        transform_point_float(gfx, &x, &y);
        transformed_coordinates[i * 2] = (x * canvas_zoom) + obj_x;
        transformed_coordinates[i * 2 + 1] = (y * canvas_zoom) + obj_y;
    }
    pdgui_vmess(0, "crs F", cnv, "coords", tags[1], path->num_path_segments*2, transformed_coordinates);
    freebytes(transformed_coordinates, path->num_path_segments * 2 * sizeof(t_float));

#else // PURR_DATA
    gui_start_vmess("gui_luagfx_stroke_path", "xsssi", cnv, tags[2], tags[1],
                    gfx->current_color, stroke_width);
    gui_start_array();
    for (int i = 0; i < path->num_path_segments; i++) {
        float x = path->path_segments[i * 2], y = path->path_segments[i * 2 + 1];
        transform_point_float(gfx, &x, &y);
        gui_s(i==0?"M":"L");
        gui_f(x); gui_f(y);
    }
    gui_end_array();
    gui_end_vmess();
#endif

    return 0;
}

static int fill_path(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    t_pdlua *obj = gfx->object;

    t_canvas *cnv = glist_getcanvas(obj->canvas);

    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    if(path->num_path_segments < 3)
        return 0;

    // Apply transformations to all coordinates
    int obj_x = text_xpix((t_object*)obj, obj->canvas);
    int obj_y = text_ypix((t_object*)obj, obj->canvas);
    int canvas_zoom = glist_getzoom(cnv);

    const char *tags[] = { gfx->object_tag, register_drawing(gfx), gfx->current_layer_tag };

#ifndef PURR_DATA
    pdgui_vmess(0, "crr iiii ri rs rS", cnv, "create", "polygon", 0, 0, 0, 0, "-width", 0, "-fill", gfx->current_color, "-tags", 3, tags);

    t_float *transformed_coordinates = getbytes(path->num_path_segments * 2 * sizeof(t_float));
    for (int i = 0; i < path->num_path_segments; i++) {
        float x =  path->path_segments[i * 2], y = path->path_segments[i * 2 + 1];
        transform_point_float(gfx, &x, &y);
        transformed_coordinates[i * 2] = (x * canvas_zoom) + obj_x;
        transformed_coordinates[i * 2 + 1] = (y * canvas_zoom) + obj_y;
    }
    pdgui_vmess(0, "crs F", cnv, "coords", tags[1], path->num_path_segments*2, transformed_coordinates);
    freebytes(transformed_coordinates, path->num_path_segments * 2 * sizeof(t_float));
#else // PURR_DATA
    gui_start_vmess("gui_luagfx_fill_path", "xsssi", cnv, tags[2], tags[1],
                    gfx->current_color, 0);
    gui_start_array();
    for (int i = 0; i < path->num_path_segments; i++) {
        float x = path->path_segments[i * 2], y = path->path_segments[i * 2 + 1];
        transform_point_float(gfx, &x, &y);
        gui_s(i==0?"M":"L");
        gui_f(x); gui_f(y);
    }
    gui_end_array();
    gui_end_vmess();
#endif

    return 0;
}


static int translate(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);

    if(gfx->num_transforms == 0)
        gfx->transforms = getbytes(sizeof(gfx_transform));
    else
        gfx->transforms = resizebytes(gfx->transforms, gfx->num_transforms * sizeof(gfx_transform), (gfx->num_transforms + 1) * sizeof(gfx_transform));

    gfx->transforms[gfx->num_transforms].type = TRANSLATE;
    gfx->transforms[gfx->num_transforms].x = luaL_checknumber(L, 1);
    gfx->transforms[gfx->num_transforms].y = luaL_checknumber(L, 2);

    gfx->num_transforms++;
    return 0;
}

static int scale(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);

    gfx->transforms = resizebytes(gfx->transforms, gfx->num_transforms * sizeof(gfx_transform), (gfx->num_transforms + 1) * sizeof(gfx_transform));

    gfx->transforms[gfx->num_transforms].type = SCALE;
    gfx->transforms[gfx->num_transforms].x = luaL_checknumber(L, 1);
    gfx->transforms[gfx->num_transforms].y = luaL_checknumber(L, 2);

    gfx->num_transforms++;
    return 0;
}

static int reset_transform(lua_State *L) {
    t_pdlua_gfx *gfx = pop_graphics_context(L);
    freebytes(gfx->transforms, gfx->num_transforms * sizeof(gfx_transform));
    gfx->transforms = NULL;
    gfx->num_transforms = 0;
    return 0;
}
#endif

static void add_path_segment(t_path_state *path, float x, float y)
{
    int path_segment_space = (path->num_path_segments + 1) * 2;
    int old_size = path->num_path_segments_allocated;
    int new_size = MAX(path_segment_space, path->num_path_segments_allocated);

    if(!path->num_path_segments_allocated)
        path->path_segments = (float*)getbytes(new_size * sizeof(float));
    else
        path->path_segments = (float*)resizebytes(path->path_segments, old_size * sizeof(float), new_size * sizeof(float));

    path->num_path_segments_allocated = new_size;

    path->path_segments[path->num_path_segments * 2] = x;
    path->path_segments[path->num_path_segments * 2 + 1] = y;
    path->num_path_segments++;
}

static int start_path(lua_State *L) {
    t_path_state *path = (t_path_state *)lua_newuserdata(L, sizeof(t_path_state));
    luaL_setmetatable(L, "Path");

    path->num_path_segments = 0;
    path->num_path_segments_allocated = 0;
    path->path_start_x = luaL_checknumber(L, 1);
    path->path_start_y = luaL_checknumber(L, 2);

    add_path_segment(path, path->path_start_x, path->path_start_y);
    return 1;
}

// Function to add a line to the current path
static int line_to(lua_State *L) {
    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    add_path_segment(path, x, y);
    return 0;
}

static int quad_to(lua_State *L) {
    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    float x2 = luaL_checknumber(L, 2);
    float y2 = luaL_checknumber(L, 3);
    float x3 = luaL_checknumber(L, 4);
    float y3 = luaL_checknumber(L, 5);

    float x1 = path->num_path_segments > 0 ? path->path_segments[(path->num_path_segments - 1) * 2] : x2;
    float y1 = path->num_path_segments > 0 ? path->path_segments[(path->num_path_segments - 1) * 2 + 1] : y2;

    // heuristic for deciding the number of lines in our bezier curve
    float dx = x3 - x1;
    float dy = y3 - y1;
    float distance = sqrtf(dx * dx + dy * dy);
    float resolution = MAX(10.0f, distance);

    // Get the last point
    float t = 0.0;
    while (t <= 1.0) {
        t += 1.0 / resolution;

        // Calculate quadratic bezier curve as points (source: https://en.wikipedia.org/wiki/B%C3%A9zier_curve)
        float x = (1.0f - t) * (1.0f - t) * x1 + 2.0f * (1.0f - t) * t * x2 + t * t * x3;
        float y = (1.0f - t) * (1.0f - t) * y1 + 2.0f * (1.0f - t) * t * y2 + t * t * y3;
        add_path_segment(path, x, y);
    }

    return 0;
}

static int cubic_to(lua_State *L) {
    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    float x2 = luaL_checknumber(L, 2);
    float y2 = luaL_checknumber(L, 3);
    float x3 = luaL_checknumber(L, 4);
    float y3 = luaL_checknumber(L, 5);
    float x4 = luaL_checknumber(L, 6);
    float y4 = luaL_checknumber(L, 7);

    float x1 = path->num_path_segments > 0 ? path->path_segments[(path->num_path_segments - 1) * 2] : x2;
    float y1 = path->num_path_segments > 0 ? path->path_segments[(path->num_path_segments - 1) * 2 + 1] : y2;

    // heuristic for deciding the number of lines in our bezier curve
    float dx = x3 - x1;
    float dy = y3 - y1;
    float distance = sqrtf(dx * dx + dy * dy);
    float resolution = MAX(10.0f, distance);

    // Get the last point
    float t = 0.0;
    while (t <= 1.0) {
        t += 1.0 / resolution;

        // Calculate cubic bezier curve as points (source: https://en.wikipedia.org/wiki/B%C3%A9zier_curve)
        float x = (1 - t)*(1 - t)*(1 - t) * x1 + 3 * (1 - t)*(1 - t) * t * x2 + 3 * (1 - t) * t*t * x3 + t*t*t * x4;
        float y = (1 - t)*(1 - t)*(1 - t) * y1 + 3 * (1 - t)*(1 - t) * t * y2 + 3 * (1 - t) * t*t * y3 + t*t*t * y4;

        add_path_segment(path, x, y);
    }

    return 0;
}

// Function to close the current path
static int close_path(lua_State *L) {
    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    add_path_segment(path, path->path_start_x, path->path_start_y);
    return 0;
}

static int free_path(lua_State *L)
{
    t_path_state *path = (t_path_state*)luaL_checkudata(L, 1, "Path");
    freebytes(path->path_segments, path->num_path_segments_allocated * sizeof(int));
    return 0;
}
