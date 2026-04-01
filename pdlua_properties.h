/** @file pdlua_properties.h
 *  @brief pdlua_properties.h -- an extension to pdlua to allow spawning properties windows
 *  @author Charles K. Neimog and Timothy Schoen
 *  @date 2026
 *
 * Copyright (C) 2026 Charles K. Neimog and Timothy Schoen
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

static int pdlua_properties_newframe(lua_State *L);
static int pdlua_properties_addcheck(lua_State *L);
static int pdlua_properties_addtext(lua_State *L);
static int pdlua_properties_addcolor(lua_State *L);
static int pdlua_properties_addint(lua_State *L);
static int pdlua_properties_addfloat(lua_State *L);
static int pdlua_properties_addcombo(lua_State *L);

static void pdlua_properties_setup(lua_State* L)
{
    static const luaL_Reg properties_methods[] = {
        {"new_frame", pdlua_properties_newframe},
        {"add_check", pdlua_properties_addcheck},
        {"add_text", pdlua_properties_addtext},
        {"add_color", pdlua_properties_addcolor},
        {"add_combo", pdlua_properties_addcombo},
        {"add_int", pdlua_properties_addint},
        {"add_float", pdlua_properties_addfloat},
        {NULL, NULL} // Sentinel to end the list
    };

    luaL_newmetatable(L, "PropertiesContext");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, properties_methods, 0);
}

#define IDLENGTH 256

static void pdlua_properties_free(t_pdlua_properties *MAYBE_UNUSED(p))
{
#ifdef PURR_DATA
    if(p->property_count)
    {
        free(p->property_method_callbacks);
        free(p->property_types);
    }
#endif
}

#ifdef PLUGDATA
#ifdef _MSC_VER
#define alloca _alloca
#endif

static inline void plugdata_add_property(lua_State* L, const char* sym, const char *fmt, ...)
{
    if (!lua_isuserdata(L, 1))
        return;
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

    int fmtlen = strlen(fmt);
    int argc = 0;
    for (int i = 0; i < fmtlen; i++)
        argc += (fmt[i] == 't' && lua_istable(L, i + 2)) ? lua_rawlen(L, i + 2) : 1;

    t_atom *atoms = alloca(sizeof(t_atom) * argc);
    int atom_idx = 0;

    va_list defaults;
    va_start(defaults, fmt);

    for (int i = 0; i < fmtlen; i++)
    {
        int lua_idx = i + 2;
        switch (fmt[i])
        {
            case 'f': {
                SETFLOAT (&atoms[atom_idx], (t_float)luaL_checknumber(L, lua_idx));
                atom_idx++;
                break;
            }
            case 's': {
                SETSYMBOL(&atoms[atom_idx], gensym(luaL_checkstring(L, lua_idx)));
                atom_idx++;
                break;
            }
            case 'b': {
                t_float val;
                if (lua_isboolean(L, lua_idx)) {
                    val = (t_float)lua_toboolean(L, lua_idx);
                } else {
                    val = (t_float)luaL_checknumber(L, lua_idx);
                }
                SETFLOAT (&atoms[atom_idx], val);
                atom_idx++;
                break;
            }
            case 'F': {
                SETFLOAT (&atoms[atom_idx], (t_float)luaL_optnumber(L, lua_idx, va_arg(defaults, double)));
                atom_idx++;
                break;
            }
            case 'S': {
                SETSYMBOL(&atoms[atom_idx], gensym(luaL_optstring(L, lua_idx, va_arg(defaults, const char*))));
                atom_idx++;
                break;
            }
            case 'c':
            {
                char color[16];
                if (lua_isstring(L, lua_idx))
                {
                    snprintf(color, sizeof(color), "%s", luaL_checkstring(L, lua_idx));
                }
                else if (lua_istable(L, lua_idx))
                {
                    lua_rawgeti(L, lua_idx, 1);
                    lua_rawgeti(L, lua_idx, 2);
                    lua_rawgeti(L, lua_idx, 3);
                    double r = luaL_checknumber(L, -3);
                    double g = luaL_checknumber(L, -2);
                    double b = luaL_checknumber(L, -1);
                    lua_pop(L, 3);
                    if (r <= 1.0 && g <= 1.0 && b <= 1.0) { r *= 255.0; g *= 255.0; b *= 255.0; }
                    int ri = (int)fmax(0, fmin(255, r));
                    int gi = (int)fmax(0, fmin(255, g));
                    int bi = (int)fmax(0, fmin(255, b));
                    snprintf(color, sizeof(color), "#%02x%02x%02x", ri, gi, bi);
                }
                else
                {
                    pd_error(NULL, "[pdlua] %s: expected color string or {r,g,b} table", sym);
                    goto cleanup;
                }
                SETSYMBOL(&atoms[atom_idx], gensym(color));
                atom_idx++;
                break;
            }
            case 't':
            {
                if (!lua_istable(L, lua_idx))
                {
                    pd_error(NULL, "[pdlua] %s: expected table at arg %d", sym, i + 1);
                    goto cleanup;
                }
                int n = lua_rawlen(L, lua_idx);
                for (int j = 0; j < n; j++)
                {
                    lua_rawgeti(L, lua_idx, j + 1);
                    SETSYMBOL(&atoms[atom_idx], gensym(lua_isstring(L, -1) ? lua_tostring(L, -1) : ""));
                    atom_idx++;
                    lua_pop(L, 1);
                }
                break;
            }
            default: break;
        }
    }

    if (pdlua->properties.plugdata_properties_callback)
        pdlua->properties.plugdata_properties_callback(pdlua, gensym(sym), atom_idx, atoms);

cleanup:
    va_end(defaults);
}

static void pdlua_properties(t_gobj *z, t_glist *MAYBE_UNUSED(owner))
{
    t_pdlua *pdlua = (t_pdlua *)z;
    lua_State *L = __L();

    lua_getglobal(L, "pd");
    lua_getfield(L, -1, "_properties");

    lua_pushlightuserdata(L, pdlua);
    t_pdlua **ctx = lua_newuserdata(L, sizeof(t_pdlua*));
    *ctx = pdlua;

    luaL_getmetatable(L, "PropertiesContext");
    lua_setmetatable(L, -2);

    if (lua_pcall(L, 2, 1, 0))
    {
        mylua_error(L, pdlua, "properties");
        return;
    }
}

static int pdlua_properties_newframe(lua_State *L)
{
    plugdata_add_property(L, "add_frame_property", "sf");
    return 0;
}

static int pdlua_properties_addcheck(lua_State *L)
{
    plugdata_add_property(L, "add_check_property", "ssb");
    return 0;
}

static int pdlua_properties_addtext(lua_State *L)
{
    plugdata_add_property(L, "add_text_property", "sss");
    return 0;
}

static int pdlua_properties_addcolor(lua_State *L)
{
    plugdata_add_property(L, "add_color_property", "ssc");
    return 0;
}

static int pdlua_properties_addint(lua_State *L)
{
    plugdata_add_property(L, "add_int_property", "ssfFF", -1e36, 1e36);
    return 0;
}

static int pdlua_properties_addfloat(lua_State *L)
{
    plugdata_add_property(L, "add_float_property", "ssfFF", -1e36, 1e36);
    return 0;
}

static int pdlua_properties_addcombo(lua_State *L)
{
    plugdata_add_property(L, "add_combo_property", "ssft");
    return 0;
}

static void pdlua_properties_receiver(t_pdlua *o, t_symbol *MAYBE_UNUSED(s), int argc, t_atom *argv)
{
    if (argc < 2)
        return;

    lua_getglobal(__L(), "pd");
    lua_getfield(__L(), -1, "_set_properties");
    lua_remove(__L(), -2);

    lua_pushlightuserdata(__L(), o);
    lua_pushstring(__L(), atom_getsymbol(argv + 1)->s_name);

    const char *guitype = atom_getsymbol(argv)->s_name;
    if (strcmp(guitype, "colorpicker") == 0)
    {
        const char *hexcolor = atom_getsymbol(argv + 2)->s_name;
        int isvalid = 1;

        if (hexcolor == NULL || hexcolor[0] != '#' || strlen(hexcolor) != 7) {
            isvalid = 0;
        } else {
            for (int i = 1; i < 7; i++) {
                if (!isxdigit(hexcolor[i])) isvalid = 0;
            }
        }

        if (!isvalid) {
            pd_error(o, "Invalid color string");
            return;
        }

        int r, g, b;
        if (sscanf(hexcolor + 1, "%2x%2x%2x", &r, &g, &b) == 3) {
            lua_newtable(__L());
            lua_pushinteger(__L(), r);
            lua_rawseti(__L(), -2, 1);
            lua_pushinteger(__L(), g);
            lua_rawseti(__L(), -2, 2);
            lua_pushinteger(__L(), b);
            lua_rawseti(__L(), -2, 3);
        } else {
            pd_error(o, "Invalid color format in sscanf");
            return;
        }
    } else {
        for (int i = 2; i < argc; i++)
        {
            if (argv[i].a_type == A_FLOAT)
            {
                lua_pushnumber(__L(), atom_getfloat(argv + i));
            }
            else if (argv[i].a_type == A_SYMBOL)
            {
                lua_pushstring(__L(), atom_getsymbol(argv + i)->s_name);
            }
        }
    }

    if (lua_pcall(__L(), 3, 0, 0))
    {
        mylua_error(__L(), o, "_set_properties");
        return;
    }
}
#else

#ifndef PURR_DATA
static void pdlua_properties_updaterow(t_pdlua_properties *p)
{
    p->current_col++;
    if (p->current_col == p->max_col) {
        p->current_row++;
        p->current_col = 0; // not used for now
    }
}
#else
static inline void purrdata_add_callback(t_pdlua_properties *p, const char *type, const char *method)
{
    p->property_method_callbacks = realloc(p->property_method_callbacks, sizeof(t_symbol*) * (p->property_count + 1));
    p->property_types = realloc(p->property_types, sizeof(t_symbol*) * (p->property_count + 1));

    p->property_method_callbacks[p->property_count] = gensym(method);
    p->property_types[p->property_count] = gensym(type);
    p->property_count++;
}

#endif

static void pdlua_properties(t_gobj *z, t_glist *MAYBE_UNUSED(owner)) {

    t_pdlua *pdlua = (t_pdlua *)z;
    t_pdlua_properties *p = &pdlua->properties;
    lua_State *L = __L();

#ifdef PURR_DATA
    const char *gfx_tag = gfxstub_new2(&pdlua->pd.te_g.g_pd, (void*)pdlua);

    free(p->property_method_callbacks);
    free(p->property_types);
    p->property_count = 0;

    gui_start_vmess("gui_external_dialog", "ss", gfx_tag, pdlua->pd.te_g.g_pd->c_name->s_name);
    gui_start_array();

    lua_getglobal(L, "pd");
    lua_getfield(L, -1, "_properties");

    lua_pushlightuserdata(L, pdlua);
    t_pdlua **ctx = lua_newuserdata(L, sizeof(t_pdlua*));
    *ctx = pdlua;

    luaL_getmetatable(L, "PropertiesContext");
    lua_setmetatable(L, -2);

    if (lua_pcall(L, 2, 0, 0))
    {
        gui_end_array();
        gui_end_vmess();

        mylua_error(L, pdlua, "properties");
        return;
    }

    gui_end_array();
    gui_end_vmess();
#else
    char receiver[MAXPDSTRING];
    snprintf(receiver, MAXPDSTRING, ".%p", p);
    pdlua->properties.properties_receiver = gensym(receiver);

    pdgui_vmess(0, "rs", "destroy", pdlua->properties.properties_receiver->s_name);

    if(pdlua->properties.frame_count != 0)
        pd_unbind(&pdlua->pd.ob_pd, p->properties_receiver);

    p->current_frame_name = NULL;
    p->current_frame_id = NULL;
    p->frame_count = 0;
    p->property_count = 0;
    p->current_row = 0;
    p->current_col = 0;
    p->pending_count = 0;

    pd_bind(&pdlua->pd.ob_pd, p->properties_receiver);

    pdgui_vmess(0, "rs rs", "toplevel", p->properties_receiver->s_name, "-class", "DialogWindow");

    char title[MAXPDSTRING];
    snprintf(title, MAXPDSTRING, "%s Properties", pdlua->pdlua_class->c_name->s_name);
    pdgui_vmess(0, "rr rs", "wm", "title", p->properties_receiver->s_name, title);

    pdgui_vmess(0, "rrsr", "wm", "group", p->properties_receiver->s_name, ".");
    pdgui_vmess(0, "rrsii", "wm", "resizable", p->properties_receiver->s_name, 0, 0);

    pdgui_vmess(0, "rrsr", "wm", "transient", p->properties_receiver->s_name, "$::focused_window");
    pdgui_vmess(0, "sr rr", p->properties_receiver->s_name, "configure", "-menu", "$::dialog_menubar");
    pdgui_vmess(0, "sr ri ri", p->properties_receiver->s_name, "configure", "-padx", 0, "-pady", 0);

    // main window
    char frame_id[IDLENGTH];
    snprintf(frame_id, IDLENGTH, ".%p.main", (void *)p);
    pdgui_vmess(0, "rrs", "wm", "deiconify", p->properties_receiver->s_name); // <- on sucess show the window
    pdgui_vmess(0, "rs ri ri", "frame", frame_id, "-padx", 15, "-pady", 15);
    pdgui_vmess(0, "rs rr ri", "pack", frame_id, "-fill", "both", "-expand", 4);
    pdgui_vmess(0, "rs ri ri", "pack", frame_id, "-pady", 10, "-padx", 10);

    // call _properties
    lua_getglobal(L, "pd");
    lua_getfield(L, -1, "_properties");

    lua_pushlightuserdata(L, pdlua);
    t_pdlua **ctx = lua_newuserdata(L, sizeof(t_pdlua*));
    *ctx = pdlua;

    luaL_getmetatable(L, "PropertiesContext");
    lua_setmetatable(L, -2);

    if (lua_pcall(L, 2, 0, 0))
    {
        mylua_error(L, pdlua, "properties");
        return;
    }

    char buttons_id[IDLENGTH];
    snprintf(buttons_id, IDLENGTH, ".%p.buttons", (void *)p);

    char button_cancel_id[IDLENGTH];
    char button_apply_id[IDLENGTH];
    char button_ok_id[IDLENGTH];
    snprintf(button_cancel_id, IDLENGTH, ".%p.buttons.cancel", (void *)p);
    snprintf(button_apply_id, IDLENGTH, ".%p.buttons.apply", (void *)p);
    snprintf(button_ok_id, IDLENGTH, ".%p.buttons.ok", (void *)p);

    char destroy_command[MAXPDSTRING];
    snprintf(destroy_command, MAXPDSTRING, "destroy .%p", (void *)p);

    pdgui_vmess(0, "rs ri", "frame", buttons_id, "-pady", 5);
    pdgui_vmess(0, "rs rs", "pack", buttons_id, "-fill", "x");

#if __APPLE__
    const char* ok_state = "normal";
#else
    const char* ok_state = "active";
#endif

    char ok_command[MAXPDSTRING];
    snprintf(ok_command, MAXPDSTRING,
        "pdsend \"%s dialog apply\"; destroy .%p",
        p->properties_receiver->s_name, (void *)p);
    pdgui_vmess(0, "rs rs rs rs", "button", button_ok_id,
        "-text", "OK", "-command", ok_command, "-default", ok_state);

    pdgui_vmess(0, "rs rs rs", "button", button_cancel_id,
        "-text", "Cancel", "-command", destroy_command);
    pdgui_vmess(0, "rs rs ri ri ri", "pack", button_cancel_id,
        "-side", "left", "-expand", 1, "-padx", 10, "-ipadx", 10);

#if __APPLE__
    char returnbind[MAXPDSTRING * 4];
    snprintf(returnbind, MAXPDSTRING * 4,
        "bind %s <Return> {"
        "  catch {"
        "    if {[focus] eq \"%s\"} {%s} "
        "    else {"
        "      pdsend \"%s dialog apply\";"
        "      %s configure -default active;"
        "      focus %s"
        "    }"
        "  };"
        "  break"
        "}",
        p->properties_receiver->s_name,
        button_ok_id, ok_command,
        p->properties_receiver->s_name,
        button_ok_id,
        button_ok_id);
    pdgui_vmess(0, "r", returnbind);
#else
    char returnbind[MAXPDSTRING * 2];
    snprintf(returnbind, MAXPDSTRING * 2,
        "bind %s <Return> {catch {pdsend \"%s dialog apply\"; destroy .%p}; break}",
        p->properties_receiver->s_name,
        p->properties_receiver->s_name,
        (void *)p);
    pdgui_vmess(0, "r", returnbind);

    char apply_command[MAXPDSTRING];
    snprintf(apply_command, MAXPDSTRING, "pdsend \"%s dialog apply\"", p->properties_receiver->s_name);
    pdgui_vmess(0, "rs rs rs", "button", button_apply_id, "-text", "Apply", "-command", apply_command);
    pdgui_vmess(0, "rs rs ri ri ri", "pack", button_apply_id, "-side", "left", "-expand", 1, "-padx", 10, "-ipadx", 10);
#endif

    pdgui_vmess(0, "rs rs ri ri ri", "pack", button_ok_id, "-side", "left", "-expand", 1, "-padx", 10, "-ipadx", 10);

    char ok_return_bind[MAXPDSTRING * 2];
    snprintf(ok_return_bind, MAXPDSTRING * 2, "bind %s <Return> {%s}", button_ok_id, ok_command);
    pdgui_vmess(0, "r", ok_return_bind);

#if __APPLE__
    char focus_bind[MAXPDSTRING * 2];
    snprintf(focus_bind, MAXPDSTRING * 2,
        "bind %s <FocusIn> {if {[focus] ne \"%s\"} {catch {%s configure -default normal}}}",
        p->properties_receiver->s_name,
        button_ok_id, button_ok_id);
    pdgui_vmess(0, "r", focus_bind);
#endif
#endif
}

static int pdlua_properties_newframe(lua_State *L)
{
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
    const char *title = luaL_checkstring(L, 2);
    int col = luaL_checknumber(L, 3);

#ifndef PURR_DATA
    pdlua->properties.frame_count++;
    char current_frame_id[100]; // limit frame name to 100 chars to ensure no buffer overflows are possible
    char current_frame_name[100];
    snprintf(current_frame_id, 100, ".%p.main.frame%d", (void *)&pdlua->properties, pdlua->properties.frame_count);
    snprintf(current_frame_name, 100, "_%p_main_frame%d", (void *)&pdlua->properties, pdlua->properties.frame_count);

    // Create main frame for set of configurations
    pdgui_vmess(0, "rs rs ri", "frame", current_frame_id, "-relief", "groove", "-borderwidth", 1);
    pdgui_vmess(0, "rs rs rs ri ri", "pack", current_frame_id, "-side", "top", "-fill", "x", "-padx", 10,
                "-pady", 10);

    // Title of the frame
    char labelid[MAXPDSTRING];
    snprintf(labelid, MAXPDSTRING, "%s.title", current_frame_id);
    pdgui_vmess(0, "rs rs", "label", labelid, "-text", title);
    pdgui_vmess(0, "rs rs ri", "pack", labelid, "-side", "top", "-pady", 5);

    // Create content frame with grid layout
    char content_frame_id[MAXPDSTRING];
    snprintf(content_frame_id, MAXPDSTRING, "%s.content", current_frame_id);
    pdgui_vmess(0, "rs", "frame", content_frame_id);
    pdgui_vmess(0, "rs rs rs", "pack", content_frame_id, "-side", "top", "-fill", "x");

    // Configure grid with 2 equal columns
    for (int i = 0; i < col; i++) {
        pdgui_vmess(0, "rs ri ri", "grid", "columnconfigure", content_frame_id, i, "-weight", 1);
    }

    pdlua->properties.current_frame_id = gensym(content_frame_id);
    pdlua->properties.current_frame_name = gensym(current_frame_name);
    pdlua->properties.max_col = col;
    pdlua->properties.current_col = 0;
    pdlua->properties.current_row = 0;
#else
    // TODO: purr-data doesn't have frames yet
#endif
    return 0;
}

static int pdlua_properties_addcheck(lua_State *L)
{
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
    const char *text = luaL_checkstring(L, 2);
    const char *method = luaL_checkstring(L, 3);
    int init_value;
    if (lua_isboolean(L, 4)) {
        init_value = lua_toboolean(L, 4);
    } else {
        init_value = (int)luaL_checknumber(L, 4);
    }

#ifndef PURR_DATA
    if(!pdlua->properties.current_frame_id)
    {
        pd_error(NULL, "[pdlua] add_check: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char check_id[IDLENGTH];
    char check_var[IDLENGTH];

    snprintf(check_var, IDLENGTH, "::%s_pdlua_property%d_value", pdlua->properties.current_frame_name->s_name, ++pdlua->properties.property_count);

    // Initialize the Tcl variable to 0 (unchecked)
    pdgui_vmess(0, "rsi", "set", check_var, init_value);

    // Build the pdsend command
    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog checkbox %s $%s]",
             pdlua->properties.properties_receiver->s_name, method, check_var);

    // Create the checkbox
    snprintf(check_id, IDLENGTH, "%s.check%d", pdlua->properties.current_frame_id->s_name, pdlua->properties.property_count);
    pdgui_vmess(0, "rs rs rs rs", "checkbutton", check_id, "-text", text, "-variable", check_var, "-command", pdsend);

    pdgui_vmess(0, "rs ri ri", "grid", check_id, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col);
    pdlua_properties_updaterow(&pdlua->properties);
#else
    char name[MAXPDSTRING];
    snprintf(name, MAXPDSTRING, "%s_%s", text, "_toggle");
    gui_s(name);
    gui_i(init_value);
    purrdata_add_callback(&pdlua->properties, "check", method);
#endif
    return 0;
}


static int pdlua_properties_addtext(lua_State *L)
{
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
    const char *text = luaL_checkstring(L, 2);
    const char *method = luaL_checkstring(L, 3);
    const char *init_value = luaL_checkstring(L, 4);

#ifndef PURR_DATA
    if(!pdlua->properties.current_frame_id)
    {
        pd_error(NULL, "[pdlua] add_text: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char text_id[MAXPDSTRING];
    char entry_id[MAXPDSTRING];
    char text_var[IDLENGTH];

    snprintf(text_var, IDLENGTH, "::%s_pdlua_property%d_value", pdlua->properties.current_frame_name->s_name, ++pdlua->properties.property_count);

    pdgui_vmess(0, "rss", "set", text_var, init_value);

    // Command to send it to pd
    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog textbox %s $%s]",
             pdlua->properties.properties_receiver->s_name, method, text_var);

    // container_id for button to set and text input
    char text_button_frame_id[IDLENGTH];
    snprintf(text_button_frame_id, IDLENGTH, "%s.text_button_frame_%d", pdlua->properties.current_frame_id->s_name, pdlua->properties.property_count);
    pdgui_vmess(0, "rs rs ri ri ri", "frame", text_button_frame_id, "-relief", "solid", "-borderwidth", 0, "-padx", 5, "-pady", 5);

    // create text for identification
    snprintf(text_id, MAXPDSTRING, "%s.text%d", text_button_frame_id, pdlua->properties.property_count);
    pdgui_vmess(0, "rs rs", "label", text_id, "-text", text);

    snprintf(entry_id, MAXPDSTRING, "%s.textbox%d", text_button_frame_id, pdlua->properties.property_count);
    pdgui_vmess(0, "rs rs ri", "entry", entry_id, "-textvariable", text_var, "-width", 8);
    pdgui_vmess(0, "rsss", "bind", entry_id, "<KeyRelease>", pdsend);
    pdgui_vmess(0, "rsss", "bind", entry_id, "<FocusOut>", pdsend);

    // Pack the entry
    pdgui_vmess(0, "rs rs", "pack", text_id, "-side", "top");
    pdgui_vmess(0, "rs rs", "pack", entry_id, "-side", "left");
    pdgui_vmess(0, "rs ri ri rs ri ri", "grid", text_button_frame_id, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we", "-padx", 8, "-pady", 8);
    pdlua_properties_updaterow(&pdlua->properties);
#else
    char name[MAXPDSTRING];
    snprintf(name, MAXPDSTRING, "%s_%s", text, "_symbol");
    gui_s(name);
    gui_s(init_value);
    purrdata_add_callback(&pdlua->properties, "text", method);
#endif
    return 0;
}

static int pdlua_properties_addcolor(lua_State *L) {
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
    const char *text = luaL_checkstring(L, 2);
    const char *method = luaL_checkstring(L, 3);

    char initcolor[16];
    if (lua_isstring(L, 4)) {
        const char *init = lua_tostring(L, 4);
        snprintf(initcolor, sizeof(initcolor), "%s", init);
    }
    else if (lua_istable(L, 4)) {
        lua_rawgeti(L, 4, 1);
        lua_rawgeti(L, 4, 2);
        lua_rawgeti(L, 4, 3);

        double r = luaL_checknumber(L, -3);
        double g = luaL_checknumber(L, -2);
        double b = luaL_checknumber(L, -1);

        lua_pop(L, 3);

        if (r <= 1.0 && g <= 1.0 && b <= 1.0) {
            r *= 255.0;
            g *= 255.0;
            b *= 255.0;
        }

        int ri = (int)(r < 0 ? 0 : (r > 255 ? 255 : r));
        int gi = (int)(g < 0 ? 0 : (g > 255 ? 255 : g));
        int bi = (int)(b < 0 ? 0 : (b > 255 ? 255 : b));

        snprintf(initcolor, sizeof(initcolor), "#%02x%02x%02x", ri, gi, bi);
    }
    else {
        pd_error(NULL, "[pdlua] add_color: invalid init value");
        return 0;
    }

#ifndef PURR_DATA
    if (!pdlua->properties.current_frame_id) {
        pd_error(NULL, "[pdlua] add_color: no active frame");
        return 0;
    }

    char container_id[IDLENGTH/2];
    char colorbox_id[IDLENGTH];
    char text_id[MAXPDSTRING];
    char pdsend[MAXPDSTRING];

    pdlua->properties.property_count++;

    snprintf(container_id, IDLENGTH/2, "%s.color%d",
             pdlua->properties.current_frame_id->s_name,
             pdlua->properties.property_count);
    snprintf(text_id, MAXPDSTRING, "%s.label", container_id);
    snprintf(colorbox_id, IDLENGTH, "%s.box", container_id);

    pdgui_vmess(0, "rs", "frame", container_id);
    pdgui_vmess(0, "rs rs ri", "label", text_id, "-text", text, "-width", 8);
    pdgui_vmess(0, "rs rs ri ri rs rs ri", "label", colorbox_id, "-text", "", "-width", 4, "-height", 2, "-background", initcolor, "-relief", "sunken", "-borderwidth", 1);
    pdgui_vmess(0, "rs rs", colorbox_id, "configure", "-cursor", "hand2");

    pdgui_vmess(0, "r",
    "proc pdlua_choose_color {widget receiver method} {\n"
    "    set current [ $widget cget -background ]\n"
    "\n"
    "    set c [tk_chooseColor -initialcolor $current -title \"Choose color\"]\n"
    "\n"
    "    if {$c ne \"\"} {\n"
    "        $widget configure -background $c\n"
    "\n"
    "        pdsend [concat $receiver dialog colorpicker $method $c]\n"
#if __APPLE__
    "        pdsend [concat $receiver dialog apply]\n"
#endif
    "    }\n"
    "}\n");

    snprintf(pdsend, MAXPDSTRING, "pdlua_choose_color %s %s %s", colorbox_id, pdlua->properties.properties_receiver->s_name, method);

    pdgui_vmess(0, "rsss", "bind", colorbox_id, "<Button-1>", pdsend);

    pdgui_vmess(0, "rs rs", "pack", text_id, "-side", "top");
    pdgui_vmess(0, "rs rs ri", "pack", colorbox_id, "-side", "top", "-pady", 2);

    pdgui_vmess(0,"rs ri ri rs ri ri", "grid", container_id, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "w", "-padx", 8, "-pady", 8);

    pdlua_properties_updaterow(&pdlua->properties);

#else
    char name[MAXPDSTRING];
    snprintf(name, MAXPDSTRING, "%s_%s", text, "_symbol");
    gui_s(name);
    gui_s(initcolor);
    purrdata_add_callback(&pdlua->properties, "colorpicker", method);
#endif

    return 0;
}

static int pdlua_properties_addint(lua_State *L)
{
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

    const char *text = luaL_checkstring(L, 2);
    const char *method = luaL_checkstring(L, 3);
    int init_value = (int)luaL_checknumber(L, 4);
    double min = luaL_optnumber(L, 5, -1e36);
    double max = luaL_optnumber(L, 6, 1e36);

#ifndef PURR_DATA
    if(!pdlua->properties.current_frame_id)
    {
        pd_error(NULL, "[pdlua] add_int: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char text_id[MAXPDSTRING];
    char spinbox_id[IDLENGTH];
    char int_var[IDLENGTH];
    char container_id[IDLENGTH/2];

    snprintf(int_var, IDLENGTH, "::%s_pdlua_property%d_value", pdlua->properties.current_frame_name->s_name, ++pdlua->properties.property_count);

    pdgui_vmess(0, "rsi", "set", int_var, init_value);

    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog numberbox %s $%s]", pdlua->properties.properties_receiver->s_name, method, int_var);

    snprintf(container_id, IDLENGTH/2, "%s.numberbox%d", pdlua->properties.current_frame_id->s_name, pdlua->properties.property_count);

    pdgui_vmess(0, "rs", "frame", container_id);

    snprintf(text_id, MAXPDSTRING, "%s.label", container_id);
    pdgui_vmess(0, "rs rs", "label", text_id, "-text", text);

    snprintf(spinbox_id, IDLENGTH, "%s.spin", container_id);

    pdgui_vmess(0, "rs rf rf rf rs ri rs rs rs", "ttk::spinbox", spinbox_id,
                "-from", min, "-to", max,
                "-increment", 1.0f,
                "-textvariable", int_var,
                "-width", 6,
                "-validate", "key",
                "-validatecommand", "string is integer %P",
                "-command", pdsend);

    pdgui_vmess(0, "rsss", "bind", spinbox_id, "<KeyRelease>", pdsend);
    pdgui_vmess(0, "rsss", "bind", spinbox_id, "<FocusOut>", pdsend);

    pdgui_vmess(0, "rs rs", "pack", text_id, "-side", "top");
    pdgui_vmess(0, "rs rs", "pack", spinbox_id, "-side", "top");
    pdgui_vmess(0, "rs ri ri rs ri ri", "grid", container_id, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we", "-padx", 8, "-pady", 8);

    pdlua_properties_updaterow(&pdlua->properties);
#else
    gui_s(text);
    gui_i(init_value);
    purrdata_add_callback(&pdlua->properties, "int", method);
#endif

    return 0;
}

static int pdlua_properties_addfloat(lua_State *L)
{
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

    const char *text = luaL_checkstring(L, 2);
    const char *method = luaL_checkstring(L, 3);
    double init_value = luaL_checknumber(L, 4);
    double min = luaL_optnumber(L, 5, -1e36);
    double max = luaL_optnumber(L, 6, 1e36);

#ifndef PURR_DATA
    if(!pdlua->properties.current_frame_id)
    {
        pd_error(NULL, "[pdlua] add_float: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char text_id[MAXPDSTRING];
    char entry_id[MAXPDSTRING];
    char float_variable[IDLENGTH];
    char container_id[IDLENGTH];

    snprintf(float_variable, IDLENGTH, "::%s_pdlua_property%d_value", pdlua->properties.current_frame_name->s_name, ++pdlua->properties.property_count);

    pdgui_vmess(0, "rsf", "set", float_variable, init_value);

    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog floatbox %s $%s]", pdlua->properties.properties_receiver->s_name, method, float_variable);

    snprintf(container_id, IDLENGTH, "%s.floatbox%d", pdlua->properties.current_frame_id->s_name, pdlua->properties.property_count);

    pdgui_vmess(0, "rs", "frame", container_id);

    snprintf(text_id, MAXPDSTRING, "%s.label", container_id);
    pdgui_vmess(0, "rs rs", "label", text_id, "-text", text);

    snprintf(entry_id, MAXPDSTRING, "%s.entry", container_id);

    char entrycmd[MAXPDSTRING];
    snprintf(entrycmd, sizeof(entrycmd), "entry %s -textvariable %s -width 7 -validate key -validatecommand {regexp {^-?[0-9]*\\.?[0-9]*$} %%P}\n", entry_id, float_variable);
    pdgui_vmess(0, "r", entrycmd);

    char keypresscmd[MAXPDSTRING * 4];
    snprintf(keypresscmd, sizeof(keypresscmd),
        "bind %s <KeyRelease> {"
        "  set v $%s;"
        "  if {$v < %g} {set %s %g} elseif {$v > %g} {set %s %g};"
        "  pdsend \"%s dialog floatbox %s $%s\""
        "}\n",
        entry_id,
        float_variable,
        min, float_variable, min,
        max, float_variable, max,
        pdlua->properties.properties_receiver->s_name, method, float_variable);
    pdgui_vmess(0, "r", keypresscmd);

    char focusoutcmd[MAXPDSTRING * 4];
    snprintf(focusoutcmd, sizeof(focusoutcmd),
        "bind %s <FocusOut> {"
        "  set v $%s;"
        "  if {$v < %g} {set %s %g} elseif {$v > %g} {set %s %g};"
        "  pdsend \"%s dialog floatbox %s $%s\""
        "}\n",
        entry_id,
        float_variable,
        min, float_variable, min,
        max, float_variable, max,
        pdlua->properties.properties_receiver->s_name, method, float_variable);
    pdgui_vmess(0, "r", focusoutcmd);


    pdgui_vmess(0, "rs rs", "pack", text_id, "-side", "top");
    pdgui_vmess(0, "rs rs", "pack", entry_id, "-side", "top");
    pdgui_vmess(0, "rs ri ri rs ri ri", "grid", container_id, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we", "-padx", 8, "-pady", 8);

    pdlua_properties_updaterow(&pdlua->properties);
#else
    gui_s(text);
    gui_f(init_value);
    purrdata_add_callback(&pdlua->properties, "float", method);
#endif

    return 0;
}


static int pdlua_properties_addcombo(lua_State *L)
{
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L,1);

    const char *text = luaL_checkstring(L,2);
    const char *method = luaL_checkstring(L,3);
    int init_value = luaL_checknumber(L,4) - 1;

    int options_count = lua_rawlen(L,5);
    const char **opts = calloc(options_count, sizeof(char*));
    for (int i = 0; i < options_count; i++)
    {
        lua_rawgeti(L, 5, i+1);
        opts[i] = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
        lua_pop(L, 1);
    }

#ifndef PURR_DATA
    char pdsend[MAXPDSTRING];
    char combo_id[IDLENGTH];
    char text_id[IDLENGTH];
    char combo_var[IDLENGTH];

    if(!pdlua->properties.current_frame_id)
    {
        pd_error(NULL, "[pdlua] add_combo: no active frame");
        return 0;
    }

    snprintf(combo_var, IDLENGTH, "::%s_pdlua_property%d_value", pdlua->properties.current_frame_name->s_name, ++pdlua->properties.property_count);

    if(init_value < options_count) {
        pdgui_vmess(0, "rss", "set", combo_var, opts[init_value]);
    }

    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog combobox %s [expr {[%%W current] + 1}]]", pdlua->properties.properties_receiver->s_name, method);

    char container_id[IDLENGTH/2];
    snprintf(container_id, IDLENGTH/2, "%s.combo%d", pdlua->properties.current_frame_id->s_name, pdlua->properties.property_count);

    pdgui_vmess(0," rs", "frame", container_id);

    snprintf(text_id, IDLENGTH, "%s.label", container_id);
    pdgui_vmess(0, "rs rs", "label", text_id, "-text", text);

    snprintf(combo_id, IDLENGTH,"%s.widget",container_id);

    pdgui_vmess(0, "rs rs rS rs ri", "ttk::combobox", combo_id, "-textvariable",combo_var, "-values", options_count, opts, "-state", "readonly", "-width", 8);

    pdgui_vmess(0, "rsss", "bind", combo_id, "<<ComboboxSelected>>", pdsend);

    pdgui_vmess(0, "rs rs", "pack", text_id, "-side", "top");
    pdgui_vmess(0, "rs rs", "pack", combo_id, "-side", "top");

    pdgui_vmess(0, "rs ri ri rs ri ri", "grid", container_id, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we", "-padx", 8, "-pady", 8);

    pdlua_properties_updaterow(&pdlua->properties);
#else
    gui_s(text);
    gui_i(init_value);
    purrdata_add_callback(&pdlua->properties, "combo", method);
#endif
    free(opts);

    return 0;
}

static void pdlua_properties_apply(t_pdlua *o)
{
    lua_State *L = __L();
    t_pdlua_properties *p = &o->properties;

    for (int i = 0; i < p->pending_count; i++)
    {
        t_pending_property *entry = &p->pending[i];

        lua_getglobal(L, "pd");
        lua_getfield(L, -1, "_set_properties");
        lua_remove(L, -2);

        lua_pushlightuserdata(L, o);
        lua_pushstring(L, entry->method);

        if (strcmp(entry->guitype, "colorpicker") == 0)
        {
            const char *hexcolor = atom_getsymbol(&entry->argv[0])->s_name;
            int r, g, b;
            if (sscanf(hexcolor + 1, "%2x%2x%2x", &r, &g, &b) == 3)
            {
                lua_newtable(L);
                lua_pushinteger(L, r); lua_rawseti(L, -2, 1);
                lua_pushinteger(L, g); lua_rawseti(L, -2, 2);
                lua_pushinteger(L, b); lua_rawseti(L, -2, 3);
            }
        }
        else
        {
            if (entry->argv[0].a_type == A_FLOAT)
                lua_pushnumber(L, atom_getfloat(&entry->argv[0]));
            else
                lua_pushstring(L, atom_getsymbol(&entry->argv[0])->s_name);
        }

        if (lua_pcall(L, 3, 0, 0))
        {
            mylua_error(L, o, "_set_properties");
        }
    }

    p->pending_count = 0;
}

static void pdlua_properties_receiver(t_pdlua *o, t_symbol *MAYBE_UNUSED(s), int argc, t_atom *argv)
{
    t_pdlua_properties *p = &o->properties;

#ifndef PURR_DATA
    if (argc < 1) return;

    const char *guitype = atom_getsymbol(argv)->s_name;

    // Apply button sends this
    if (strcmp(guitype, "apply") == 0)
    {
        pdlua_properties_apply(o);
        return;
    }

    if (argc < 2) return;

    const char *method = atom_getsymbol(argv + 1)->s_name;

    // Overwrite existing entry for the same method so only the latest value is kept
    t_pending_property *entry = NULL;
    for (int i = 0; i < p->pending_count; i++)
    {
        if (strcmp(p->pending[i].method, method) == 0)
        {
            entry = &p->pending[i];
            break;
        }
    }

    if (!entry)
    {
        if (p->pending_count >= MAX_PENDING_PROPERTIES)
        {
            pd_error(o, "[pdlua] too many pending properties");
            return;
        }
        entry = &p->pending[p->pending_count++];
    }

    snprintf(entry->guitype, MAXPDSTRING, "%s", guitype);
    snprintf(entry->method,  MAXPDSTRING, "%s", method);
    entry->argc = 0;

    for (int i = 2; i < argc && (i - 2) < MAX_PROPERTY_ARGS; i++)
    {
        entry->argv[entry->argc++] = argv[i];
    }
#else
    lua_State *L = __L();
    for (int i = 0; i < argc; i++)
    {
        lua_getglobal(L, "pd");
        lua_getfield(L, -1, "_set_properties");
        lua_remove(L, -2);

        lua_pushlightuserdata(L, o);
        lua_pushstring(L, p->property_method_callbacks[i]->s_name);

        t_atom* value = argv + i;
        if (strcmp(p->property_types[i]->s_name, "colorpicker") == 0)
        {
            const char *hexcolor = atom_getsymbol(value)->s_name;
            int r, g, b;
            if (sscanf(hexcolor + 1, "%2x%2x%2x", &r, &g, &b) == 3)
            {
                lua_newtable(L);
                lua_pushinteger(L, r); lua_rawseti(L, -2, 1);
                lua_pushinteger(L, g); lua_rawseti(L, -2, 2);
                lua_pushinteger(L, b); lua_rawseti(L, -2, 3);
            }
        }
        else
        {
            if (value->a_type == A_FLOAT)
                lua_pushnumber(L, atom_getfloat(value));
            else
                lua_pushstring(L, atom_getsymbol(value)->s_name);
        }

        if (lua_pcall(L, 3, 0, 0))
        {
            mylua_error(L, o, "_set_properties");
        }
    }
#endif
}

#endif
