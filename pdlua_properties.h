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
static int pdlua_properties_addcheckbox(lua_State *L);
static int pdlua_properties_addtextinput(lua_State *L);
static int pdlua_properties_addcolorpicker(lua_State *L);

static int pdlua_properties_setup(lua_State* L)
{
    static const luaL_Reg properties_methods[] = {
        {"new_frame", pdlua_properties_newframe},
        {"add_checkbox", pdlua_properties_addcheckbox},
        {"add_textinput", pdlua_properties_addtextinput},
        {"add_colorpicker", pdlua_properties_addcolorpicker},
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
        mylua_error(__L(), NULL, "new_frame: invalid args");
    }
    return 0;
}

static int pdlua_properties_addcheckbox(lua_State *L)
{
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3) && lua_isnumber(L, 4))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

        t_atom atoms[3];
        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        SETFLOAT (&atoms[2], (t_float)lua_tonumber(L, 4));
        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_checkbox_property"), 3, atoms);
    } else {
        mylua_error(__L(), NULL, "add_checkbox: invalid args");
    }
    return 0;
}

static int pdlua_properties_addtextinput(lua_State *L)
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
        mylua_error(__L(), NULL, "add_textinput: invalid args");
    }
    return 0;
}

static int pdlua_properties_addcolorpicker(lua_State *L)
{
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);

        t_atom atoms[2];
        SETSYMBOL(&atoms[0], gensym(lua_tostring(L, 2)));
        SETSYMBOL(&atoms[1], gensym(lua_tostring(L, 3)));
        pdlua->properties.plugdata_properties_callback(pdlua, gensym("add_color_property"), 2, atoms);
    } else {
        mylua_error(__L(), NULL, "add_colorpicker: invalid args");
    }
    return 0;
}

#else

static void pdlua_properties(t_gobj *z, t_glist *owner) {
    t_pdlua *pdlua = (t_pdlua *)z;
    t_pdlua_properties *p = &pdlua->properties;

    char receiver[MAXPDSTRING];
    snprintf(receiver, MAXPDSTRING, ".%p", p);
    pdlua->properties.properties_receiver = gensym(receiver);
    pdlua->properties.current_frame = NULL;
    pd_bind(&pdlua->pd.ob_pd, p->properties_receiver); // new to unbind

    pdlua_properties_createdialog(p); // <-- create hidden window

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

    t_pdlua **ctx = lua_newuserdata(L, sizeof(t_pdlua*));
    *ctx = pdlua;

    lua_pushlightuserdata(L, pdlua);
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

static void pdlua_properties_createdialog(t_pdlua_properties *p)
{
    pdgui_vmess(0, "ssss", "toplevel", p->properties_receiver->s_name, "-class", "DialogWindow");
    pdgui_vmess(0, "ssss", "wm", "title", p->properties_receiver->s_name, "{[mydialog] Properties}");
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
        mylua_error(__L(), NULL, "new_frame: invalid args");

    }
    return 0;
}

static int pdlua_properties_addcheckbox(lua_State *L)
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

        char pdsend[MAXPDSTRING];
        char checkid[MAXPDSTRING];
        char checkvariable[MAXPDSTRING];
        char sanitized_frame[MAXPDSTRING];
        pdlua->properties.checkbox_count++;

        // Sanitize frame name (replace '.' with '_')
        snprintf(sanitized_frame, MAXPDSTRING, "%s", pdlua->properties.current_frame->s_name);
        for (char *p = sanitized_frame; *p != '\0'; p++) {
            if (*p == '.') {
                *p = '_';
            }
        }

        // Generate unique variable name
        snprintf(checkvariable, MAXPDSTRING, "::checkbox%d_%s_state", pdlua->properties.checkbox_count,
                 sanitized_frame);

        // Initialize the Tcl variable to 0 (unchecked)
        pdgui_vmess(0, "ssi", "set", checkvariable, init_value);

        // Build the pdsend command
        snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s _properties checkbox %s $%s]",
                 pdlua->properties.properties_receiver->s_name, method, checkvariable);

        // Create the checkbox
        snprintf(checkid, MAXPDSTRING, "%s.check%d", pdlua->properties.current_frame->s_name, pdlua->properties.checkbox_count);
        pdgui_vmess(0, "ssssssss", "checkbutton", checkid, "-text", text, "-variable", checkvariable,
                    "-command", pdsend);

        pdgui_vmess(0, "sssisi", "grid", checkid, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col,
                    "-sticky", "we");
        pdlua_properties_updaterow(&pdlua->properties);
    } else {
        mylua_error(__L(), NULL, "add_checkbox: invalid args");
    }
    return 0;
}


static int pdlua_properties_addtextinput(lua_State *L)
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

        char pdsend[MAXPDSTRING];
        char textid[MAXPDSTRING];
        char buttonid[MAXPDSTRING];
        char entryid[MAXPDSTRING];

        char numvariable[MAXPDSTRING];

        char sanitized_frame[MAXPDSTRING];
        pdlua->properties.numberbox_count++;

        // Sanitize frame name (replace '.' with '_')
        snprintf(sanitized_frame, MAXPDSTRING, "%s", pdlua->properties.current_frame->s_name);
        for (char *p = sanitized_frame; *p != '\0'; p++) {
            if (*p == '.') {
                *p = '_';
            }
        }

        // Variable save the value of gui obj
        snprintf(numvariable, MAXPDSTRING, "::numberbox%d_%s_value", pdlua->properties.numberbox_count,
                 sanitized_frame);
        pdgui_vmess(0, "sss", "set", numvariable, init_value);

        // Command to send it to pd
        snprintf(pdsend, MAXPDSTRING, "eval pdsend [concat %s _properties numberbox %s $%s]",
                 pdlua->properties.properties_receiver->s_name, method, numvariable);

        // container for button to set and text input
        char text_button_frame[MAXPDSTRING];
        snprintf(text_button_frame, MAXPDSTRING, "%s.text_button_frame_%d", pdlua->properties.current_frame->s_name,
                 pdlua->properties.numberbox_count);
        pdgui_vmess(0, "sssssisisi", "frame", text_button_frame, "-relief", "solid", "-borderwidth", 1,
                    "-padx", 5, "-pady", 5);

        // create text for identification
        snprintf(textid, MAXPDSTRING, "%s.text%d", text_button_frame, pdlua->properties.numberbox_count);
        pdgui_vmess(0, "ssss", "label", textid, "-text", text);

        // Create the number entry box
        snprintf(entryid, MAXPDSTRING, "%s.numberbox%d", text_button_frame, pdlua->properties.numberbox_count);
        pdgui_vmess(0, "sssssi", "entry", entryid, "-textvariable", numvariable, "-width", width);

        // Create the set button
        snprintf(buttonid, MAXPDSTRING, "%s.setbutton%d", text_button_frame, pdlua->properties.numberbox_count);
        pdgui_vmess(0, "sssssssisi", "button", buttonid, "-text", "Set", "-command", pdsend, "-padx",
                    10, "-pady", 0);

        // Pack the entry and button side by side
        pdgui_vmess(0, "ssss", "pack", textid, "-side", "top");
        pdgui_vmess(0, "ssss", "pack", entryid, "-side", "left");
        pdgui_vmess(0, "ssss", "pack", buttonid, "-side", "right");
        pdgui_vmess(0, "sssisisssi", "grid", text_button_frame, "-row", pdlua->properties.current_row, "-column",
                    pdlua->properties.current_col, "-sticky", "we", "-padx", 20, "-pady", 20);
        pdlua_properties_updaterow(&pdlua->properties);
    } else {
        mylua_error(__L(), NULL, "add_textinput: invalid args");
    }
    return 0;
}

// static int pdlua_dialog_createcolorpicker(t_pdlua *x, const char *text, const char *method) {
static int pdlua_properties_addcolorpicker(lua_State *L) {
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3))
    {
        t_pdlua *pdlua = *(t_pdlua**)lua_touserdata(L, 1);
        const char *text = lua_tostring(L, 2);
        const char *method = lua_tostring(L, 3);

        char pdsend[MAXPDSTRING];
        char buttonid[MAXPDSTRING];
        char colorvariable[MAXPDSTRING];
        char sanitized_frame[MAXPDSTRING];
        pdlua->properties.colorpicker_count++;

        // Sanitize frame name (replace '.' with '_')
        snprintf(sanitized_frame, MAXPDSTRING, "%s", pdlua->properties.current_frame->s_name);
        for (char *p = sanitized_frame; *p != '\0'; p++) {
            if (*p == '.') {
                *p = '_';
            }
        }

        // Generate unique variable name
        snprintf(colorvariable, MAXPDSTRING, "::colorpicker%d_%s_value", pdlua->properties.colorpicker_count,
                 sanitized_frame);

        // Initialize the Tcl variable to a default color
        pdgui_vmess(0, "sss", "set", colorvariable, "#ffffff");

        // Build the pdsend command to trigger color picker and send result
        snprintf(pdsend, MAXPDSTRING,
            "eval pdsend [concat %s _properties colorpicker %s [tk_chooseColor -initialcolor {#ffffff} -title {Choose color}]]",
                pdlua->properties.properties_receiver->s_name, method);

        // Create the color picker button with the constructed command
        snprintf(buttonid, MAXPDSTRING, "%s.colorpicker%d", pdlua->properties.current_frame->s_name,
                 pdlua->properties.colorpicker_count);
        pdgui_vmess(0, "ssssss", "button", buttonid, "-text", text, "-command", pdsend);

        pdgui_vmess(0, "sssisi", "grid", buttonid, "-row", pdlua->properties.current_row, "-column", pdlua->properties.current_col,
                    "-sticky", "we");
        pdlua_properties_updaterow(&pdlua->properties);
    } else {
        mylua_error(__L(), NULL, "add_colorpicker: invalid args");
    }
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
    } else{
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
