
-- First approximation of a component script

Button = 
{
	-- Component
	uiComp;
	uiTransform;
	textComp;
	
	-- Data
	ButtonIndex;
	Enabled = true;

	-- Trigger/Touch
	Touched = false;
	Clicked = false;
	
	-- MOUSE
	MousePositionX = 0;
	MousePositionY = 0;
	LEFTCLICK = false;
	RIGHTCLICK = false;
	ENTER = false;

	-- KEYBOARD
	Up = false;
	Down = false;
	--	Text
	textUnTouchedScale = Vector3.new(0.0,0.0,0.0);
	textTouchedScale = Vector3.new(0.0,0.0,0.0);

}

--Init called when comp is created
Button.Init = function(self)
	OnKeyEvent():Bind({self, self.OnKey});
	OnMouseMotion():Bind({self, self.OnMouseMotion});
	OnMouseClick():Bind({self, self.OnMouseClick});
	OnJoystickButton():Bind({self, self.OnJoystickButton});
end

--Begin called when obj has all comps
Button.Begin = function(self, owner, goMgr)

	if (owner == nil) then
		OutputPrint("ERROR, OWNER IS NIL\n");
		return;
	end
	self.uiComp = owner:GetUiComp();
	if (self.uiComp == nil) then 
		OutputPrint("ERROR, UIComponent IS NIL\n");
	end
	self.uiTransform = owner:GetTransformComp();
	if (self.uiTransform == nil) then 
		OutputPrint("ERROR, UITransform IS NIL\n");
	end
	
	self.textComp = owner:GetCustomComp("Text");
	if (self.textComp == nil) then 
		OutputPrint("ERROR, Text Component IS NIL\n");
	end
	
	self.ButtonIndex = self.uiComp:GetButtonIndex();
	local scale = Vector3.new(self.uiTransform:GetScale());
	self.uiComp:SetUnTouchedScale(scale);
	self.uiComp:SetTouchedScale(scale*1.1);
	
	self.textUnTouchedScale = Vector3.new(self.uiComp:GetTextScale());
	self.textTouchedScale = Vector3.new(self.textUnTouchedScale*1.1);
end

--Update called every tick
Button.Update = function(self, dt, owner) 

	if(self.Enabled == false) then
		return;
	end
	local position = Vector3.new(self.uiTransform:GetPosition());
	local scale = Vector3.new(self.uiTransform:GetScale());
	local maxPosX = scale.x + position.x;
	local maxPosY = scale.y + position.y;
	if(self.Touched == true) then
		local touchedScale = Vector3.new(self.uiComp:GetTouchedScale());
		self.uiTransform:Scale(touchedScale);
		self.textComp:Touched();
	else
		local unTouchedScale = Vector3.new(self.uiComp:GetUnTouchedScale());
		self.uiTransform:Scale(unTouchedScale);
		self.textComp:UnTouched();
	end
	if(self.ButtonIndex == _G.CurrentButtonTouched) then
		self.Touched = true;
		if(self.ENTER == true ) then
			self.ENTER = false;
			self.Clicked = true;
		end
	else
		self.Touched = false;
	end
	if(self.MousePositionX < maxPosX) then
		if(self.MousePositionX > position.x) then
			if(self.MousePositionY < maxPosY) then
				if(self.MousePositionY > position.y) then
					self.Touched = true;
					_G.CurrentButtonTouched = self.ButtonIndex;
					if(self.LEFTCLICK == true) then
						self.LEFTCLICK = false;
						self.Clicked = true;
					end
				end	
			end
		end
	end
end
--Method
Button.OnKey = function(self, key, state)
	if(SCANCODE.ENTER == key) then
		self.ENTER = state;
	end
end
Button.OnJoystickButton = function(self, joystickId, button, state)
	if(button == CONTROLLER.A) then
		self.ENTER = state;
	end
end
Button.OnMouseMotion = function(self, position, deltaposition)
	self.MousePositionX = position.x;
	self.MousePositionY = position.y;
end

Button.OnMouseClick = function(self, button, state)
	if(button == 1) then
		self.LEFTCLICK = state;
	elseif(button == 2) then
		self.RIGHTCLICK = state;
	end
end

Button.OnDestruction = function(self)
	OnKeyEvent():Unbind({self, self.OnMouseMotion});
	OnMouseMotion():Unbind({self, self.OnMouseMotion});
	OnMouseClick():Unbind({self, self.OnMouseClick});
	OnJoystickButton():Unbind({self, self.OnJoystickButton});
end

Button.ReturnClicked = function(self)
   return self.Clicked;
end
Button.SetClicked = function(self, val)
    self.Clicked = val;
end
Button.Disable = function(self)
   self.Enabled = false;
end
Button.Enabled = function(self)
   self.Enabled = true;
end
return Button;