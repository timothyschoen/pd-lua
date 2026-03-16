local properties = pd.Class:new():register("properties")

function properties:initialize(sel, atoms)
    self.inlets = 1
    self.outlets = 1
    self.phase = 0
    self:addproperties()
    self:set_size(127, 127)

    self.checkbox1 = 1
    self.checkbox2 = 1
    self.color = {155,155,155}

    self.textinput1 = "Hello"
    self.textinput2 = 40
    self.bigstring = "Uidsja hasd  asdhj asdy asdhjasd"

    return true
end

function properties:properties()
    self:newframe("First CheckBox", 2)
    self:addcheckbox("Check Box 1", "updatecheckbox1", self.checkbox1)
    self:addcheckbox("Check Box 2", "updatecheckbox2", self.checkbox2)

    self:newframe("First textinput", 2)
    self:addtextinput("Check textinput 1", "updatetext1", self.textinput1, 5)
    self:addtextinput("Check textinput 2", "updatetext2", self.textinput2, 5)

    self:newframe("My Color Picker", 1)
    self:addcolorpicker("Background", "updatecolorbg");
end

function properties:updatecolorbg(args)
    self.color[1] = args[1][1]
    self.color[2] = args[1][2]
    self.color[3] = args[1][3]
    self:repaint(1)
end

function properties:updatetext1(args)
    self.textinput1 = args[1]
    pd.post("textinput1 is now " .. self.textinput1);
end

function properties:updatetext2(args)
    self.textinput2 = args[1]
    pd.post("textinput2 is now " .. self.textinput2);
end

function properties:updatecheckbox1(args)
    self.checkbox1 = args[1]
    pd.post("checkbox1 is now " .. self.checkbox1);
end

function properties:updatecheckbox2(args)
    self.checkbox2 = args[1]
    pd.post("checkbox2 is now " .. self.checkbox2);
end

function properties:paint(g)
   g:set_color(table.unpack(self.color))
   g:fill_all()
end
