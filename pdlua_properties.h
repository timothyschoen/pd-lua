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

static int pdlua_properties_setup(lua_State* L)
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

#ifdef PLUGDATA

static void pdlua_properties(t_gobj *z, t_glist *owner)
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
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isnumber(L, 3))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        const char *title = lua_tostring(L, 2);
        int col = lua_tonumber(L, 3);

        t_atom atoms[1];
        SETSYMBOL(&atoms[0], gensym(title));
        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_frame_property"), 1, atoms);
    } else {
        pd_error(NULL, "[pdlua] new_frame: invalid args");
    }
    return 0;
}

static int pdlua_properties_addcheck(lua_State *L)
{
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3) && lua_isnumber(L, 4))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

        t_atom atoms[3];
        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        SETFLOAT (&atoms[2], (t_float)lua_tonumber(L, 4));
        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_check_property"), 3, atoms);
    } else {
        pd_error(NULL, "[pdlua] add_check: invalid args");
    }
    return 0;
}

static int pdlua_properties_addtext(lua_State *L)
{
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3) && lua_isstring(L, 4) && lua_isnumber(L, 5))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

        t_atom atoms[4];
        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        SETSYMBOL(&atoms[2], gensym(lua_tostring(L, 4)));
        SETFLOAT (&atoms[3], (t_float)lua_tonumber(L, 5));
        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_text_property"), 4, atoms);
    } else {
        pd_error(NULL, "[pdlua] add_text: invalid args");
    }
    return 0;
}

static int pdlua_properties_addcolor(lua_State *L)
{
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

        t_atom atoms[2];
        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_color_property"), 2, atoms);
    } else {
        pd_error(NULL, "[pdlua] add_color: invalid args");
    }
    return 0;
}

static int pdlua_properties_addint(lua_State *L)
{
    if (lua_isuserdata(L, 1) &&
        lua_isstring(L, 2) &&
        lua_isstring(L, 3) &&
        lua_isnumber(L, 4))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        t_atom atoms[5];

        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        SETFLOAT(&atoms[2], lua_tonumber(L, 4));
        SETFLOAT(&atoms[3], luaL_optnumber(L, 5, -1e36));
        SETFLOAT(&atoms[4], luaL_optnumber(L, 6, 1e36));

        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_number_property"), 5, atoms);
    }
    else
    {
        pd_error(NULL, "[pdlua] add_int: invalid args");
    }

    return 0;
}

static int pdlua_properties_addfloat(lua_State *L)
{
    if (lua_isuserdata(L, 1) &&
        lua_isstring(L, 2) &&
        lua_isstring(L, 3) &&
        lua_isnumber(L, 4))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        t_atom atoms[5];

        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        SETFLOAT(&atoms[2], lua_tonumber(L, 4));
        SETFLOAT(&atoms[3], luaL_optnumber(L, 5, -1e36));
        SETFLOAT(&atoms[4], luaL_optnumber(L, 6, 1e36));

        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_number_property"), 5, atoms);
    }
    else
    {
        pd_error(NULL, "[pdlua] add_number: invalid args");
    }

    return 0;
}

static int pdlua_properties_addcombo(lua_State *L)
{
    if (lua_isuserdata(L, 1) &&
        lua_isstring(L, 2) &&
        lua_isstring(L, 3) &&
        lua_isnumber(L, 4) &&
        lua_istable(L, 5))
    {
        int num_options = lua_rawlen(L, 5);
        int argc = 3 + num_options;
        t_atom *atoms = (t_atom *)getbytes(sizeof(t_atom) * argc);

        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        SETFLOAT(&atoms[2], lua_tonumber(L, 4));

        for (int i = 0; i < num_options; i++)
        {
            lua_rawgeti(L, 5, i + 1);
            if (lua_isstring(L, -1))
                SETSYMBOL(&atoms[3 + i], gensym(lua_tostring(L, -1)));
            else
                SETSYMBOL(&atoms[3 + i], gensym(""));
            lua_pop(L, 1);
        }

        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_combo_property"), argc, atoms);
        freebytes(atoms, sizeof(t_atom) * argc);
    }
    else
    {
        pd_error(NULL, "[pdlua] add_combo: invalid args");
    }

    return 0;
}


#else

static void pdlua_properties_createdialog(t_pdlua_properties *p, const char* name)
{
    pdgui_vmess(0, "ssss", "toplevel", p->properties_receiver->s_name, "-class", "DialogWindow");

    char title[MAXPDSTRING];
    snprintf(title, MAXPDSTRING, "%s", name);
    char *suffix = strstr(title, ":gfx");
    if (suffix) *suffix = '\0';
    strncat(title, " Properties", MAXPDSTRING - strlen(title) - 1);
    pdgui_vmess(0, "ssss", "wm", "title", p->properties_receiver->s_name, title);

    pdgui_vmess(0, "sss", "wm", "group", p->properties_receiver->s_name, ".");
    pdgui_vmess(0, "sssii", "wm", "resizable", p->properties_receiver->s_name, 0, 0);

    pdgui_vmess(0, "sss", "wm", "transient", p->properties_receiver->s_name, "$::focused_window");
    pdgui_vmess(0, "ssss", p->properties_receiver->s_name, "configure", "-menu", "$::dialog_menubar");
    pdgui_vmess(0, "sssfsf", p->properties_receiver->s_name, "configure", "-padx", 0.0f, "-pady", 0.0f);
}

static void pdlua_properties_updaterow(t_pdlua_properties *p)
{
    p->current_col++;
    if (p->current_col == p->max_col) {
        p->current_row++;
        p->current_col = 0; // not used for now
    }
}

static void pdlua_properties_setupbuttons(t_pdlua_properties *p) {
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

    // Criando o frame dos botões
    pdgui_vmess(0, "sssf", "frame", buttonsId, "-pady", 5.0f);
    pdgui_vmess(0, "ssss", "pack", buttonsId, "-fill", "x");

    // Cancel (Close window)
    pdgui_vmess(0, "ssssss", "button", buttonCancelId, "-text", "Cancel", "-command",
                destroyCommand);
    pdgui_vmess(0, "sssssisisi", "pack", buttonCancelId, "-side", "left", "-expand", 1, "-padx", 10,
                "-ipadx", 10);

    // Apply (send all data to pd and lua obj) for this must be necessary to save all the variables used in the object in a char [128][MAXPDSTRING],
    // I don't think that this is good, or there is better solution?
    // TODO: Need to dev the apply command
    pdgui_vmess(0, "ssss", "button", buttonApplyId, "-text", "Apply");
    // pdgui_vmess(0, "ssssss", "button", buttonApplyId, "-text", "Apply", "-command", command);
    pdgui_vmess(0, "sssssisisi", "pack", buttonApplyId, "-side", "left", "-expand", 1, "-padx", 10,
                "-ipadx", 10);

    // Ok
    pdgui_vmess(0, "ssssss", "button", buttonOkId, "-text", "OK", "-command", destroyCommand);
    pdgui_vmess(0, "sssssisisi", "pack", buttonOkId, "-side", "left", "-expand", 1, "-padx", 10,
                "-ipadx", 10);
}

static void pdlua_properties(t_gobj *z, t_glist *owner) {
    t_pdlua *pdlua = (t_pdlua *)z;
    t_pdlua_properties *p = &pdlua->properties;

    lua_State *L = __L();

    char receiver[MAXPDSTRING];
    snprintf(receiver, MAXPDSTRING, ".%p", p);
    pdlua->properties.properties_receiver = gensym(receiver);

    pdgui_vmess(0, "ss", "destroy", pdlua->properties.properties_receiver->s_name);

    if(pdlua->properties.frame_count != 0)
        pd_unbind(&pdlua->pd.ob_pd, p->properties_receiver);

    pdlua->properties.current_frame = NULL;
    pdlua->properties.frame_count = 0;
    pdlua->properties.property_count = 0;
    pdlua->properties.current_row = 0;
    pdlua->properties.current_col = 0;

    pd_bind(&pdlua->pd.ob_pd, p->properties_receiver);

    pdlua_properties_createdialog(p, pdlua->pd.te_g.g_pd->c_name->s_name); // <-- create hidden window

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

    if (lua_pcall(L, 2, 1, 0))
    {
        mylua_error(L, pdlua, "properties");
        return;
    }

    // Get the return value (Lua pushes it onto the stack)
    int result = lua_toboolean(__L(), -1); // Converts Lua boolean to C int (1 = true, 0 = false)
    lua_pop(__L(), 1); // Remove the result from the stack
    if (!result)
    {
        pdgui_vmess(0, "ss", "destroy", p->properties_receiver->s_name);
        return;
    }
    pdlua_properties_setupbuttons(p); // <- this is independed of all previous containers
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
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isnumber(L, 3))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        const char *title = lua_tostring(L, 2);
        int col = lua_tonumber(L, 3);

        pdlua->properties.frame_count++;
        char current_frameid[MAXPDSTRING];
        snprintf(current_frameid, MAXPDSTRING, ".%p.main.frame%d", (void *)&pdlua->properties, pdlua->properties.frame_count);
        pdlua->properties.current_frame = gensym(current_frameid);

        // raised, sunken, flat, ridge, solid, and groove.
        // Create main frame for set of configurations
        pdgui_vmess(0, "sssssi", "frame", current_frameid, "-relief", "groove", "-borderwidth", 1);
        pdgui_vmess(0, "sssssssisi", "pack", current_frameid, "-side", "top", "-fill", "x", "-padx", 10,
                    "-pady", 10);

        // Title of the Frame
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
    } else{
        pd_error(NULL, "[pdlua] new_frame: invalid args");

    }
    return 0;
}

static int pdlua_properties_addcheck(lua_State *L)
{
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3) && lua_isnumber(L, 4))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        const char *text = lua_tostring(L, 2);
        const char *method = lua_tostring(L, 3);
        int init_value = lua_tonumber(L, 4);
        if (pdlua == NULL){
            return 0 ;
        }

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
        snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s _properties checkbox %s $%s]",
                 pdlua->properties.properties_receiver->s_name, method, checkvariable);

        // Create the checkbox
        snprintf(checkid, MAXPDSTRING, "%s.check%d", pdlua->properties.current_frame->s_name, pdlua->properties.property_count);
        pdgui_vmess(0, "ssssssss", "checkbutton", checkid, "-text", text, "-variable", checkvariable,
                    "-command", pdsend);

        pdgui_vmess(0, "sssisi", "grid", checkid, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col,
                    "-sticky", "we");
        pdlua_properties_updaterow(&pdlua->properties);
    } else {
        pd_error(NULL, "[pdlua] add_check: invalid args");
    }
    return 0;
}


static int pdlua_properties_addtext(lua_State *L)
{
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3) && lua_isstring(L, 4) && lua_isnumber(L, 5))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        const char *text = lua_tostring(L, 2);
        const char *method = lua_tostring(L, 3);
        const char *init_value = lua_tostring(L, 4);
        int width = lua_tonumber(L, 5);
        if (pdlua == NULL){
            mylua_error(__L(), pdlua, "pdlua is NULL");
            return 0 ;
        }

        if(!pdlua->properties.current_frame)
        {
            pd_error(NULL, "[pdlua] add_text: no active frame");
            return 0;
        }

        char pdsend[MAXPDSTRING];
        char textid[MAXPDSTRING];
        char buttonid[MAXPDSTRING];
        char entryid[MAXPDSTRING];

        char textvariable[MAXPDSTRING];

        pdlua_properties_buildvar(pdlua, textvariable);

        pdgui_vmess(0, "sss", "set", textvariable, init_value);

        // Command to send it to pd
        snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s _properties textbox %s $%s]",
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
        pdgui_vmess(0, "sssssi", "entry", entryid, "-textvariable", textvariable, "-width", width);
        pdgui_vmess(0, "ssss", "bind", entryid, "<Return>", pdsend);
        pdgui_vmess(0, "ssss", "bind", entryid, "<FocusOut>", pdsend);

        // Pack the entry
        pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
        pdgui_vmess(0, "ssss", "pack", entryid, "-side", "left");
        pdgui_vmess(0, "sssisisssi", "grid", text_button_frame, "-row", pdlua->properties.current_row, "-column",
                    pdlua->properties.current_col, "-sticky", "we", "-padx", 20, "-pady", 20);
        pdlua_properties_updaterow(&pdlua->properties);
    } else {
        pd_error(NULL, "[pdlua] add_text: invalid args");
    }
    return 0;
}

// static int pdlua_dialog_createcolorpicker(t_pdlua *x, const char *text, const char *method) {
static int pdlua_properties_addcolor(lua_State *L) {
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        const char *text = lua_tostring(L, 2);
        const char *method = lua_tostring(L, 3);

        if(!pdlua->properties.current_frame)
        {
            pd_error(NULL, "[pdlua] add_color: no active frame");
            return 0;
        }

        char pdsend[MAXPDSTRING];
        char buttonid[MAXPDSTRING];
        char colorvariable[MAXPDSTRING];

        pdlua_properties_buildvar(pdlua, colorvariable);

        // Initialize the Tcl variable to a default color
        pdgui_vmess(0, "sss", "set", colorvariable, "#ffffff");

        // Build the pdsend command to trigger color picker and send result
        snprintf(pdsend, MAXPDSTRING,
            "eval pdsend [concat %s _properties colorpicker %s [tk_chooseColor -initialcolor {#ffffff} -title {Choose color}]]",
                pdlua->properties.properties_receiver->s_name, method);

        // Create the color picker button with the constructed command
        snprintf(buttonid, MAXPDSTRING, "%s.colorpicker%d", pdlua->properties.current_frame->s_name,
                 pdlua->properties.property_count);
        pdgui_vmess(0, "ssssss", "button", buttonid, "-text", text, "-command", pdsend);

        pdgui_vmess(0, "sssisi", "grid", buttonid, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col,
                    "-sticky", "we");
        pdlua_properties_updaterow(&pdlua->properties);
    } else {
        pd_error(NULL, "[pdlua] add_color: invalid args");
    }

    return 0;
}

static int pdlua_properties_addint(lua_State *L)
{
    if (lua_isuserdata(L, 1) &&
        lua_isstring(L, 2) &&
        lua_isstring(L, 3) &&
        lua_isnumber(L, 4))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

        const char *text = lua_tostring(L, 2);
        const char *method = lua_tostring(L, 3);
        double init = lua_tonumber(L, 4);
        double min = luaL_optnumber(L, 5, -1e36);
        double max = luaL_optnumber(L, 6, 1e36);

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

        pdgui_vmess(0, "ssf", "set", numvariable, init);

        snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s _properties numberbox %s $%s]",
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

        pdgui_vmess(0, "ssss", "bind", spinboxid, "<Return>", pdsend);
        pdgui_vmess(0, "ssss", "bind", spinboxid, "<FocusOut>", pdsend);

        pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
        pdgui_vmess(0, "ssss", "pack", spinboxid, "-side", "top");
        pdgui_vmess(0, "sssisiss", "grid", container, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we");

        pdlua_properties_updaterow(&pdlua->properties);
    }
    else {
        pd_error(NULL, "[pdlua] add_int: invalid args");
    }

    return 0;
}

static int pdlua_properties_addfloat(lua_State *L)
{
    if (lua_isuserdata(L, 1) &&
        lua_isstring(L, 2) &&
        lua_isstring(L, 3) &&
        lua_isnumber(L, 4))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

        const char *text = lua_tostring(L, 2);
        const char *method = lua_tostring(L, 3);
        double init = lua_tonumber(L, 4);
        double min = luaL_optnumber(L, 5, -1e36);
        double max = luaL_optnumber(L, 6, 1e36);

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

        pdgui_vmess(0, "ssf", "set", numvariable, init);

        snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s _properties floatbox %s $%s]",
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


        char returncmd[MAXPDSTRING * 4];
        snprintf(returncmd, sizeof(returncmd),
            "bind %s <Return> {"
            "  set v $%s;"
            "  if {$v < %g} {set %s %g} elseif {$v > %g} {set %s %g};"
            "  pdsend \"%s _properties floatbox %s $%s\""
            "}\n",
            entryid,
            numvariable,
            min, numvariable, min,
            max, numvariable, max,
            pdlua->properties.properties_receiver->s_name, method, numvariable);
        sys_gui(returncmd);

        char focusoutcmd[MAXPDSTRING * 4];
        snprintf(focusoutcmd, sizeof(focusoutcmd),
            "bind %s <FocusOut> {"
            "  set v $%s;"
            "  if {$v < %g} {set %s %g} elseif {$v > %g} {set %s %g};"
            "  pdsend \"%s _properties floatbox %s $%s\""
            "}\n",
            entryid,
            numvariable,
            min, numvariable, min,
            max, numvariable, max,
            pdlua->properties.properties_receiver->s_name, method, numvariable);
        sys_gui(focusoutcmd);


        pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
        pdgui_vmess(0, "ssss", "pack", entryid, "-side", "top");
        pdgui_vmess(0, "sssisiss", "grid", container, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we");

        pdlua_properties_updaterow(&pdlua->properties);
    }
    else {
        pd_error(NULL, "[pdlua] add_float: invalid args");
    }

    return 0;
}

static int pdlua_properties_addcombo(lua_State *L)
{
    if (lua_isuserdata(L,1) &&
        lua_isstring(L,2) &&
        lua_isstring(L,3) &&
        lua_isnumber(L,4) &&
        lua_istable(L,5))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L,1);

        const char *label = lua_tostring(L,2);
        const char *method = lua_tostring(L,3);
        int init = lua_tonumber(L,4) - 1;

        int options_count = lua_rawlen(L,5);

        char pdsend[MAXPDSTRING];
        char comboid[MAXPDSTRING];
        char textid[MAXPDSTRING];
        char combovar[MAXPDSTRING];

        pdlua_properties_buildvar(pdlua, combovar);

        const char **opts = calloc(options_count, sizeof(char*));
        for (int i = 0; i < options_count; i++)
        {
            lua_rawgeti(L, 5, i+1);
            opts[i] = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
            lua_pop(L, 1);
        }

        if(!pdlua->properties.current_frame)
        {
            pd_error(NULL, "[pdlua] add_combo: no active frame");
            return 0;
        }

        if(init < options_count) {
            pdgui_vmess(0,"sss","set", combovar, opts[init]);
        }

        snprintf(pdsend, MAXPDSTRING,
                 "eval pdsend [concat %s _properties combobox %s [expr {[%%W current] + 1}]]",
                 pdlua->properties.properties_receiver->s_name, method);

        char container[MAXPDSTRING];
        snprintf(container, MAXPDSTRING, "%s.combo%d", pdlua->properties.current_frame->s_name, pdlua->properties.property_count);

        pdgui_vmess(0,"ss", "frame", container);

        snprintf(textid, MAXPDSTRING, "%s.label", container);
        pdgui_vmess(0,"ssss", "label", textid, "-text", label);

        snprintf(comboid, MAXPDSTRING,"%s.widget",container);

        pdgui_vmess(0,"sssssSss", "ttk::combobox", comboid, "-textvariable",combovar, "-values", options_count, opts, "-state", "readonly");

        pdgui_vmess(0,"ssss", "bind", comboid, "<<ComboboxSelected>>", pdsend);

        pdgui_vmess(0,"ssss", "pack", textid, "-side", "top");
        pdgui_vmess(0,"ssss", "pack", comboid, "-side", "top");

        pdgui_vmess(0,"sssisiss", "grid", container, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col, "-sticky", "we");

        pdlua_properties_updaterow(&pdlua->properties);
        free(opts);
    }
    else
    {
        pd_error(NULL, "[pdlua] add_combo: invalid args");
    }

    return 0;
}
#endif

static void pdlua_properties_receiver(t_pdlua *o, t_symbol *s, int argc, t_atom *argv)
{
    if (argc < 2)
        return;

    lua_getglobal(__L(), "pd");
    lua_getfield(__L(), -1, "_set_properties");
    lua_remove(__L(), -2);

    lua_pushlightuserdata(__L(), o);
    lua_pushstring(__L(), atom_getsymbol(argv + 1)->s_name);
    lua_newtable(__L()); // Criando a tabela

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
            lua_rawseti(__L(), -2, 1);
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
                lua_rawseti(__L(), -2, i - 1); // Store at index (1-based in Lua)
            }
            else if (argv[i].a_type == A_SYMBOL)
            {
                lua_pushstring(__L(), atom_getsymbol(argv + i)->s_name);
                lua_rawseti(__L(), -2, i - 1);
            }
        }
    }

    if (lua_pcall(__L(), 3, 0, 0))
    {
        mylua_error(__L(), o, "_set_properties"); // Handle error
        lua_pop(__L(), 1); // Pop error message
        return;
    }
}
