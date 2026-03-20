local properties = pd.Class:new():register("props")

function properties:initialize(sel, atoms)
    self.inlets = 1
    self.outlets = 1
    self.phase = 0
    self:set_size(127, 127)

    self.checkbox1 = 1
    self.checkbox2 = 0
    self.color = {155,155,155}

    self.textinput1 = "Hello"
    self.textinput2 = "World"

    self.float = 0.5
    self.int = 4
    self.combo = 1

    return true
end

function properties:properties(p)
    -- Checkboxes
    p:new_frame("First CheckBox", 2)
    p:add_check("Check Box 1", "updatecheckbox1", self.checkbox1)
    p:add_check("Check Box 2", "updatecheckbox2", self.checkbox2)

    -- Text inputs
    p:new_frame("First textinput", 2)
    p:add_text("Check text 1", "updatetext1", self.textinput1)
    p:add_text("Check text 2", "updatetext2", self.textinput2)

    -- Color picker
    p:new_frame("My Color Picker", 1)
    p:add_color("Background", "updatecolorbg", self.color)

    -- Number boxes
    p:new_frame("Numbers", 2)
    p:add_float("Float", "updatefloat", self.float, 0, 1)
    p:add_int("Int", "updateint", self.int, 1, 16)

    -- Combobox
    p:new_frame("Combo", 1)
    p:add_combo("Waveform", "updatecombo", self.combo, { "Sine", "Square", "Saw", "Triangle" })
end

function properties:updatecolorbg(args)
    self.color[1] = args[1]
    self.color[2] = args[2]
    self.color[3] = args[3]
    self:repaint(1)
end

function properties:updatetext1(textval)
    self.textinput1 = textval
    pd.post("textinput1 is now " .. self.textinput1);
end

function properties:updatetext2(textval)
    self.textinput2 = textval
    pd.post("textinput2 is now " .. self.textinput2);
end

function properties:updatecheckbox1(checkval)
    self.checkbox1 = checkval
    pd.post("checkbox1 is now " .. self.checkbox1);
end

function properties:updatecheckbox2(checkval)
    self.checkbox2 = checkval
    pd.post("checkbox2 is now " .. self.checkbox2);
end

function properties:updatefloat(flonum)
    self.float = flonum
    pd.post("float is now " .. self.float);
end

function properties:updateint(intnum)
    self.int = intnum
    pd.post("int is now " .. self.int);
end

function properties:updatecombo(combosel)
    self.combo = combosel
    pd.post("combo is now " .. self.combo);
end

function properties:paint(g)
   g:set_color(table.unpack(self.color))
   g:fill_all()
end
