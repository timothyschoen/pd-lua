/** @file pdlua_properties.h
 *  @brief pdlpdlua_propertiesua_gfx -- an extension to pdlua that spawning properties windowds
 *  @author Charles K. Neimog
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

static void pdlua_properties_free(t_pdlua_properties *p)
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

static void pdlua_properties(t_gobj *z, t_glist * IGNORE_UNUSED(owner))
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

static void pdlua_properties_receiver(t_pdlua *o, t_symbol * IGNORE_UNUSED(s), int argc, t_atom *argv)
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

static void pdlua_properties(t_gobj *z, t_glist *IGNORE_UNUSED(owner)) {

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

    pdgui_vmess(0, "ss", "destroy", pdlua->properties.properties_receiver->s_name);

    if(pdlua->properties.frame_count != 0)
        pd_unbind(&pdlua->pd.ob_pd, p->properties_receiver);

    p->current_frame = NULL;
    p->frame_count = 0;
    p->property_count = 0;
    p->current_row = 0;
    p->current_col = 0;
    p->pending_count = 0;

    pd_bind(&pdlua->pd.ob_pd, p->properties_receiver);

    pdgui_vmess(0, "ssss", "toplevel", p->properties_receiver->s_name, "-class", "DialogWindow");

    char title[MAXPDSTRING];
    snprintf(title, MAXPDSTRING, "%s Properties", pdlua->pdlua_class->c_name->s_name);
    pdgui_vmess(0, "ssss", "wm", "title", p->properties_receiver->s_name, title);

    pdgui_vmess(0, "sss", "wm", "group", p->properties_receiver->s_name, ".");
    pdgui_vmess(0, "sssii", "wm", "resizable", p->properties_receiver->s_name, 0, 0);

    pdgui_vmess(0, "sss", "wm", "transient", p->properties_receiver->s_name, "$::focused_window");
    pdgui_vmess(0, "ssss", p->properties_receiver->s_name, "configure", "-menu", "$::dialog_menubar");
    pdgui_vmess(0, "sssfsf", p->properties_receiver->s_name, "configure", "-padx", 0.0f, "-pady", 0.0f);

    // main window
    char frameId[MAXPDSTRING];
    snprintf(frameId, MAXPDSTRING, ".%p.main", (void *)p);
    pdgui_vmess(0, "sss", "wm", "deiconify", p->properties_receiver->s_name); // <- on sucess show the window
    pdgui_vmess(0, "sssf", "frame", frameId, "-padx", 15.0f, "-pady", 15.0f);
    pdgui_vmess(0, "sssssf", "pack", frameId, "-fill", "both", "-expand", 4.0f);
    pdgui_vmess(0, "sssfsf", "pack", frameId, "-pady", 10.f, "-padx", 10.f);

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

    char buttonsId[MAXPDSTRING];
    snprintf(buttonsId, MAXPDSTRING, ".%p.buttons", (void *)p);

    char buttonCancelId[MAXPDSTRING];
    char buttonApplyId[MAXPDSTRING];
    char buttonOkId[MAXPDSTRING];
    snprintf(buttonCancelId, MAXPDSTRING, ".%p.buttons.cancel", (void *)p);
    snprintf(buttonApplyId, MAXPDSTRING, ".%p.buttons.apply", (void *)p);
    snprintf(buttonOkId, MAXPDSTRING, ".%p.buttons.ok", (void *)p);

    char destroyCommand[MAXPDSTRING];
    snprintf(destroyCommand, MAXPDSTRING, "destroy .%p", (void *)p);

    pdgui_vmess(0, "sssf", "frame", buttonsId, "-pady", 5.0f);
    pdgui_vmess(0, "ssss", "pack", buttonsId, "-fill", "x");

#if __APPLE__
    const char* okButtonState = "normal";
#else
    const char* okButtonState = "active";
#endif

    char okCommand[MAXPDSTRING * 2];
    snprintf(okCommand, MAXPDSTRING * 2,
        "pdsend \"%s dialog apply\"; destroy .%p",
        p->properties_receiver->s_name, (void *)p);
    pdgui_vmess(0, "ssssssss", "button", buttonOkId,
        "-text", "OK", "-command", okCommand, "-default", okButtonState);

    pdgui_vmess(0, "ssssss", "button", buttonCancelId,
        "-text", "Cancel", "-command", destroyCommand);
    pdgui_vmess(0, "sssssisisi", "pack", buttonCancelId,
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
        buttonOkId, okCommand,
        p->properties_receiver->s_name,
        buttonOkId,
        buttonOkId);
    pdgui_vmess(0, "r", returnbind);
#else
    char returnbind[MAXPDSTRING * 2];
    snprintf(returnbind, MAXPDSTRING * 2,
        "bind %s <Return> {catch {pdsend \"%s dialog apply\"; destroy .%p}; break}",
        p->properties_receiver->s_name,
        p->properties_receiver->s_name,
        (void *)p);
    pdgui_vmess(0, "r", returnbind);

    char applyCommand[MAXPDSTRING];
    snprintf(applyCommand, MAXPDSTRING,
        "pdsend \"%s dialog apply\"",
        p->properties_receiver->s_name);
    pdgui_vmess(0, "ssssss", "button", buttonApplyId,
        "-text", "Apply", "-command", applyCommand);
    pdgui_vmess(0, "sssssisisi", "pack", buttonApplyId,
        "-side", "left", "-expand", 1, "-padx", 10, "-ipadx", 10);
#endif

    pdgui_vmess(0, "sssssisisi", "pack", buttonOkId,
        "-side", "left", "-expand", 1, "-padx", 10, "-ipadx", 10);

    char okReturnBind[MAXPDSTRING * 2];
    snprintf(okReturnBind, MAXPDSTRING * 2,
        "bind %s <Return> {%s}",
        buttonOkId, okCommand);
    pdgui_vmess(0, "r", okReturnBind);

#if __APPLE__
    char focusbind[MAXPDSTRING * 2];
    snprintf(focusbind, MAXPDSTRING * 2,
        "bind %s <FocusIn> {if {[focus] ne \"%s\"} {catch {%s configure -default normal}}}",
        p->properties_receiver->s_name,
        buttonOkId,
        buttonOkId);
    pdgui_vmess(0, "r", focusbind);
#endif
#endif
}

static void pdlua_properties_buildvar(t_pdlua *pdlua, char *out)
{
    char sanitized_frame[MAXPDSTRING];
    snprintf(sanitized_frame, MAXPDSTRING, "%s",  pdlua->properties.current_frame->s_name);

    /* replace '.' because Tcl variable names don't like them */
    for (char *p = sanitized_frame; *p; p++)
        if (*p == '.')
            *p = '_';

    snprintf(out, MAXPDSTRING, "::%s%d_%s_value", "pdlua_property", ++pdlua->properties.property_count, sanitized_frame);
}

static int pdlua_properties_newframe(lua_State *L)
{
    t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
    const char *title = luaL_checkstring(L, 2);
    int col = luaL_checknumber(L, 3);

#ifndef PURR_DATA
    pdlua->properties.frame_count++;
    char current_frameid[MAXPDSTRING];
    snprintf(current_frameid, MAXPDSTRING, ".%p.main.frame%d", (void *)&pdlua->properties, pdlua->properties.frame_count);
    pdlua->properties.current_frame = gensym(current_frameid);

    // Create main frame for set of configurations
    pdgui_vmess(0, "sssssi", "frame", current_frameid, "-relief", "groove", "-borderwidth", 1);
    pdgui_vmess(0, "sssssssisi", "pack", current_frameid, "-side", "top", "-fill", "x", "-padx", 10,
                "-pady", 10);

    // Title of the frame
    char labelid[MAXPDSTRING];
    snprintf(labelid, MAXPDSTRING, "%s.title", current_frameid);
    pdgui_vmess(0, "ssss", "label", labelid, "-text", title);
    pdgui_vmess(0, "sssssf", "pack", labelid, "-side", "top", "-pady", 5.f);

    // Create content frame with grid layout
    char content_frameid[MAXPDSTRING];
    snprintf(content_frameid, MAXPDSTRING, "%s.content", current_frameid);
    pdgui_vmess(0, "ss", "frame", content_frameid);
    pdgui_vmess(0, "ssss", "pack", content_frameid, "-side", "top", "-fill", "x");

    // Configure grid with 2 equal columns
    for (int i = 0; i < col; i++) {
        pdgui_vmess(0, "sssisi", "grid", "columnconfigure", content_frameid, i, "-weight", 1);
    }
    pdlua->properties.current_frame = gensym(content_frameid);
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
    if(!pdlua->properties.current_frame)
    {
        pd_error(NULL, "[pdlua] add_check: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char checkid[MAXPDSTRING];
    char checkvariable[MAXPDSTRING];

    pdlua_properties_buildvar(pdlua, checkvariable);

    // Initialize the Tcl variable to 0 (unchecked)
    pdgui_vmess(0, "ssi", "set", checkvariable, init_value);

    // Build the pdsend command
    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog checkbox %s $%s]",
             pdlua->properties.properties_receiver->s_name, method, checkvariable);

    // Create the checkbox
    snprintf(checkid, MAXPDSTRING, "%s.check%d", pdlua->properties.current_frame->s_name, pdlua->properties.property_count);
    pdgui_vmess(0, "ssssssss", "checkbutton", checkid, "-text", text, "-variable", checkvariable,
                "-command", pdsend);

    pdgui_vmess(0, "sssisi", "grid", checkid, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col,
                "-sticky", "we");
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
    if(!pdlua->properties.current_frame)
    {
        pd_error(NULL, "[pdlua] add_text: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char textid[MAXPDSTRING];
    char entryid[MAXPDSTRING];

    char textvariable[MAXPDSTRING];

    pdlua_properties_buildvar(pdlua, textvariable);

    pdgui_vmess(0, "sss", "set", textvariable, init_value);

    // Command to send it to pd
    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog textbox %s $%s]",
             pdlua->properties.properties_receiver->s_name, method, textvariable);

    // container for button to set and text input
    char text_button_frame[MAXPDSTRING];
    snprintf(text_button_frame, MAXPDSTRING, "%s.text_button_frame_%d", pdlua->properties.current_frame->s_name,
             pdlua->properties.property_count);
    pdgui_vmess(0, "sssssisisi", "frame", text_button_frame, "-relief", "solid", "-borderwidth", 0,
                "-padx", 5, "-pady", 5);

    // create text for identification
    snprintf(textid, MAXPDSTRING, "%s.text%d", text_button_frame, pdlua->properties.property_count);
    pdgui_vmess(0, "ssss", "label", textid, "-text", text);

    snprintf(entryid, MAXPDSTRING, "%s.textbox%d", text_button_frame, pdlua->properties.property_count);
    pdgui_vmess(0, "sssssi", "entry", entryid, "-textvariable", textvariable, "-width", 8);
    pdgui_vmess(0, "ssss", "bind", entryid, "<KeyRelease>", pdsend);
    pdgui_vmess(0, "ssss", "bind", entryid, "<FocusOut>", pdsend);

    // Pack the entry
    pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
    pdgui_vmess(0, "ssss", "pack", entryid, "-side", "left");
    pdgui_vmess(0, "sssisisssi", "grid", text_button_frame, "-row", pdlua->properties.current_row, "-column",
                pdlua->properties.current_col, "-sticky", "we", "-padx", 20, "-pady", 20);
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
    if (!pdlua->properties.current_frame) {
        pd_error(NULL, "[pdlua] add_color: no active frame");
        return 0;
    }

    char container[MAXPDSTRING];
    char textid[MAXPDSTRING];
    char colorboxid[MAXPDSTRING];
    char pdsend[MAXPDSTRING];

    pdlua->properties.property_count++;

    snprintf(container, MAXPDSTRING, "%s.color%d",
             pdlua->properties.current_frame->s_name,
             pdlua->properties.property_count);
    snprintf(textid, MAXPDSTRING, "%s.label", container);
    snprintf(colorboxid, MAXPDSTRING, "%s.box", container);

    pdgui_vmess(0, "ss", "frame", container);
    pdgui_vmess(0, "ssss", "label", textid, "-text", text);
    pdgui_vmess(0, "sssssisisssssi", "label", colorboxid, "-text", "", "-width", 4, "-height", 2, "-background", initcolor, "-relief", "sunken", "-borderwidth", 1);
    pdgui_vmess(0, "ssss", colorboxid, "configure", "-cursor", "hand2");

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

    snprintf(pdsend, MAXPDSTRING, "pdlua_choose_color %s %s %s", colorboxid, pdlua->properties.properties_receiver->s_name, method);

    pdgui_vmess(0, "ssss", "bind", colorboxid, "<Button-1>", pdsend);

    pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
    pdgui_vmess(0, "sssssi", "pack", colorboxid, "-side", "top", "-pady", 2);

    pdgui_vmess(0,"sssisiss", "grid", container, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "w");

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
    if(!pdlua->properties.current_frame)
    {
        pd_error(NULL, "[pdlua] add_int: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char spinboxid[MAXPDSTRING];
    char textid[MAXPDSTRING];
    char numvariable[MAXPDSTRING];
    char container[MAXPDSTRING];

    pdlua_properties_buildvar(pdlua, numvariable);

    pdgui_vmess(0, "ssi", "set", numvariable, init_value);

    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog numberbox %s $%s]",
             pdlua->properties.properties_receiver->s_name, method, numvariable);

    snprintf(container, MAXPDSTRING, "%s.numberbox%d",
             pdlua->properties.current_frame->s_name,
             pdlua->properties.property_count);

    pdgui_vmess(0, "ss", "frame", container);

    snprintf(textid, MAXPDSTRING, "%s.label", container);
    pdgui_vmess(0, "ssss", "label", textid, "-text", text);

    snprintf(spinboxid, MAXPDSTRING, "%s.spin", container);

    pdgui_vmess(0, "sssfsfsfsssissssss", "ttk::spinbox", spinboxid,
                "-from", min, "-to", max,
                "-increment", 1.0f,
                "-textvariable", numvariable,
                "-width", 8,
                "-validate", "key",
                "-validatecommand", "string is integer %P",
                "-command", pdsend);

    pdgui_vmess(0, "ssss", "bind", spinboxid, "<KeyRelease>", pdsend);
    pdgui_vmess(0, "ssss", "bind", spinboxid, "<FocusOut>", pdsend);

    pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
    pdgui_vmess(0, "ssss", "pack", spinboxid, "-side", "top");
    pdgui_vmess(0, "sssisiss", "grid", container, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we");

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
    if(!pdlua->properties.current_frame)
    {
        pd_error(NULL, "[pdlua] add_float: no active frame");
        return 0;
    }

    char pdsend[MAXPDSTRING];
    char entryid[MAXPDSTRING];
    char textid[MAXPDSTRING];
    char numvariable[MAXPDSTRING];
    char container[MAXPDSTRING];

    pdlua_properties_buildvar(pdlua, numvariable);

    pdgui_vmess(0, "ssf", "set", numvariable, init_value);

    snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s dialog floatbox %s $%s]",
             pdlua->properties.properties_receiver->s_name, method, numvariable);

    snprintf(container, MAXPDSTRING, "%s.floatbox%d",
             pdlua->properties.current_frame->s_name,
             pdlua->properties.property_count);

    pdgui_vmess(0, "ss", "frame", container);

    snprintf(textid, MAXPDSTRING, "%s.label", container);
    pdgui_vmess(0, "ssss", "label", textid, "-text", text);

    snprintf(entryid, MAXPDSTRING, "%s.entry", container);

    pdgui_vmess(0, "sssssissss", "entry", entryid,
                "-textvariable", numvariable,
                "-width", 8,
                "-validate", "key",
                "-validatecommand", "regexp {^-?[0-9]*\\.?[0-9]*$} %P");


    char keypresscmd[MAXPDSTRING * 4];
    snprintf(keypresscmd, sizeof(keypresscmd),
        "bind %s <KeyRelease> {"
        "  set v $%s;"
        "  if {$v < %g} {set %s %g} elseif {$v > %g} {set %s %g};"
        "  pdsend \"%s dialog floatbox %s $%s\""
        "}\n",
        entryid,
        numvariable,
        min, numvariable, min,
        max, numvariable, max,
        pdlua->properties.properties_receiver->s_name, method, numvariable);
    pdgui_vmess(0, "r", keypresscmd);

    char focusoutcmd[MAXPDSTRING * 4];
    snprintf(focusoutcmd, sizeof(focusoutcmd),
        "bind %s <FocusOut> {"
        "  set v $%s;"
        "  if {$v < %g} {set %s %g} elseif {$v > %g} {set %s %g};"
        "  pdsend \"%s dialog floatbox %s $%s\""
        "}\n",
        entryid,
        numvariable,
        min, numvariable, min,
        max, numvariable, max,
        pdlua->properties.properties_receiver->s_name, method, numvariable);
    pdgui_vmess(0, "r", focusoutcmd);


    pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
    pdgui_vmess(0, "ssss", "pack", entryid, "-side", "top");
    pdgui_vmess(0, "sssisiss", "grid", container, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we");

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
    char comboid[MAXPDSTRING];
    char textid[MAXPDSTRING];
    char combovar[MAXPDSTRING];

    pdlua_properties_buildvar(pdlua, combovar);

    if(!pdlua->properties.current_frame)
    {
        pd_error(NULL, "[pdlua] add_combo: no active frame");
        return 0;
    }

    if(init_value < options_count) {
        pdgui_vmess(0,"sss","set", combovar, opts[init_value]);
    }

    snprintf(pdsend, MAXPDSTRING,
             "eval pdsend [concat %s dialog combobox %s [expr {[%%W current] + 1}]]",
             pdlua->properties.properties_receiver->s_name, method);

    char container[MAXPDSTRING];
    snprintf(container, MAXPDSTRING, "%s.combo%d", pdlua->properties.current_frame->s_name, pdlua->properties.property_count);

    pdgui_vmess(0,"ss", "frame", container);

    snprintf(textid, MAXPDSTRING, "%s.label", container);
    pdgui_vmess(0,"ssss", "label", textid, "-text", text);

    snprintf(comboid, MAXPDSTRING,"%s.widget",container);

    pdgui_vmess(0,"sssssSsssi", "ttk::combobox", comboid, "-textvariable",combovar, "-values", options_count, opts, "-state", "readonly", "-width", 8);

    pdgui_vmess(0,"ssss", "bind", comboid, "<<ComboboxSelected>>", pdsend);

    pdgui_vmess(0,"ssss", "pack", textid, "-side", "top");
    pdgui_vmess(0,"ssss", "pack", comboid, "-side", "top");

    pdgui_vmess(0,"sssisiss", "grid", container, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we");

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

static void pdlua_properties_receiver(t_pdlua *o, t_symbol *IGNORE_UNUSED(s), int argc, t_atom *argv)
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
