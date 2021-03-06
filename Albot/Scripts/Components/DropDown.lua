
-- First approximation of a component script

DropDown = 
{
	Enabled = true;
	-- Component
	uiComp;
	uiTransformComp;
	
	-- Data
	TotalChildren;
	CurrentSelectedChildButtonIndex;
	ParentName;
	-- DropDown ButtonArrow Component
	ButtonArrowObj;
	uiCompButtonArrow;
	uiButtonArrowTransformComp;
	ButtonArrowIndex;
	ScaleButtonArrow;
	TouchedScaleButtonArrow;
	TouchedButtonArrow = false;
	ClickedButtonArrow = false;
	ButtonArrowSelected = false;
	StartButtonCount = 0;
	EndButtonCount = 0;
	-- MOUSE
	MousePositionX = 0;
	MousePositionY = 0;
	LEFTCLICK = false;
	RIGHTCLICK = false;
	ENTER = false;
	-- KEYBOARD
	Up = false;
	Down = false;	
}

--Init called when comp is created
DropDown.Init = function(self)
	OnKeyEvent():Bind({self, self.OnKey});
	OnMouseMotion():Bind({self, self.OnMouseMotion});
	OnMouseClick():Bind({self, self.OnMouseClick});
	OnJoystickButton():Bind({self, self.OnJoystickButton});
end

--Begin called when obj has all comps
DropDown.Begin = function(self, owner, goMgr)
	
	if (owner == nil) then
		OutputPrint("ERROR, OWNER IS NIL\n");
		return;
	end
	
	tag = owner:GetTag();
	self.uiComp = owner:GetUiComp();
	if (self.uiComp == nil) then
		OutputPrint(">>>  UI Component not found\n");
		return;
	end
	
	self.uiTransformComp = owner:GetTransformComp();
	if (self.uiTransformComp == nil) then
		OutputPrint(">>>  Transform Component not found\n");
		return;
	end
	-- Button Arrow
	ButtonArrowAddress = tag .. "ButtonArrow";
	self.ButtonArrowObj = goMgr:FindGameObject(ButtonArrowAddress);
	if (self.ButtonArrowObj == nil) then
		OutputPrint(">>> GO  not found\n");
		return;
	end
	self.uiCompButtonArrow = self.ButtonArrowObj:GetUiComp();
	if (self.uiCompButtonArrow == nil) then
		OutputPrint(">>> Button Arrow UI Component not found\n");
		return;
	end
	self.uiButtonArrowTransformComp = self.ButtonArrowObj:GetTransformComp();
	if (self.uiButtonArrowTransformComp == nil) then
		OutputPrint(">>> Button Arrow Transform Component not found\n");
		return;
	end
	self.ButtonArrowIndex = self.uiCompButtonArrow:GetButtonIndex();
	local scale = Vector3.new(self.uiButtonArrowTransformComp:GetScale());
	self.uiCompButtonArrow:SetUnTouchedScale(scale);
	self.uiCompButtonArrow:SetTouchedScale(scale*1.1);
	
	self.ParentName = self.uiComp:GetParentName();

	--  todo: We will set the last button Index from the source file
	self.CurrentSelectedChildButtonIndex = self.ButtonArrowIndex +3;
	--
	
	
end

--Update called every tick
DropDown.Update = function(self, dt, owner) 
	if(self.Enabled == false) then
		return;
	end
	self.TotalChildren = self.uiComp:GetNumberOfChildren();
	
	-- Button Arrow
	local positionButtonArrow = Vector3.new(self.uiButtonArrowTransformComp:GetPosition());
	local scaleButtonArrow = Vector3.new(self.uiButtonArrowTransformComp:GetScale());
	local maxPosXButtonArrow = scaleButtonArrow.x + positionButtonArrow.x;
	local maxPosYButtonArrow = 0.0;
	local minPosYButtonArrow = 0.0;
	if(self.ButtonArrowSelected == true) then
		 maxPosYButtonArrow = positionButtonArrow.y;
		 minPosYButtonArrow = scaleButtonArrow.y + positionButtonArrow.y
	else
		 maxPosYButtonArrow = scaleButtonArrow.y + positionButtonArrow.y;
		 minPosYButtonArrow = positionButtonArrow.y;
	end
	
	if(self.TouchedButtonArrow == true) then
		local touchedScale = Vector3.new(self.uiCompButtonArrow:GetTouchedScale());
		self.uiButtonArrowTransformComp:Scale(touchedScale);
	else
		local unTouchedScale = Vector3.new(self.uiCompButtonArrow:GetUnTouchedScale());
		self.uiButtonArrowTransformComp:Scale(unTouchedScale);
	end
	if(self.ButtonArrowIndex == _G.CurrentButtonTouched) then
		self.TouchedButtonArrow = true;
		
		if(self.ENTER == true ) then
			self.ENTER = false;
			self.ClickedButtonArrow = true;
			
		end
	else
		self.TouchedButtonArrow = false;
	end
	if(self.MousePositionX < maxPosXButtonArrow) then
		if(self.MousePositionX > positionButtonArrow.x) then
			if(self.MousePositionY < maxPosYButtonArrow) then
				if(self.MousePositionY > minPosYButtonArrow) then
					self.TouchedButtonArrow = true;
					_G.CurrentButtonTouched = self.ButtonArrowIndex;
					if(self.LEFTCLICK == true) then
						self.LEFTCLICK = false;
						self.ClickedButtonArrow = true;
						
					end
				end	
			end
		end
	end

	
	if(self.ClickedButtonArrow == true) then
		self.ClickedButtonArrow = false;
		if(self.ButtonArrowSelected == true) then
			self.ButtonArrowSelected = false;
			positionButtonArrow = Vector3.new(self.uiCompButtonArrow:GetFinalPosition());
			local touchedScale = Vector3.new(self.uiCompButtonArrow:GetTouchedScale());
			local unTouchedScale = Vector3.new(self.uiCompButtonArrow:GetUnTouchedScale());
			touchedScale.y = -touchedScale.y;
			unTouchedScale.y = -unTouchedScale.y;
			scaleButtonArrow.y = -scaleButtonArrow.y ;
			self.uiButtonArrowTransformComp:Scale(scaleButtonArrow);
			self.uiButtonArrowTransformComp:SetLocalPosition(positionButtonArrow);
			self.uiCompButtonArrow:SetTouchedScale(touchedScale);
			self.uiCompButtonArrow:SetUnTouchedScale(unTouchedScale);
			
		else
			self.ButtonArrowSelected = true;
			positionButtonArrow.y = positionButtonArrow.y + scaleButtonArrow.y;
			local touchedScale = Vector3.new(self.uiCompButtonArrow:GetTouchedScale());
			local unTouchedScale = Vector3.new(self.uiCompButtonArrow:GetUnTouchedScale());
			touchedScale.y = -touchedScale.y;
			unTouchedScale.y = -unTouchedScale.y;
			scaleButtonArrow.y = -scaleButtonArrow.y ;
			self.uiButtonArrowTransformComp:Scale(scaleButtonArrow);
			self.uiButtonArrowTransformComp:SetLocalPosition(positionButtonArrow);
			self.uiCompButtonArrow:SetTouchedScale(touchedScale);
			self.uiCompButtonArrow:SetUnTouchedScale(unTouchedScale);
			
		end
		
		local uiCompoent = self.uiComp:GetChild(0);
		self.StartButtonCount = uiCompoent:GetButtonIndex();
		uiCompoent = self.uiComp:GetChild(self.TotalChildren-1);
		self.EndButtonCount = uiCompoent:GetButtonIndex();
		
		for i =0,self.TotalChildren-1, 1
		do
			local childUICompoent = self.uiComp:GetChild(i);
			if (childUICompoent == nil) then
				OutputPrint(">>> Child UI Component not found\n");
				return;
			end
			local renderEnable = childUICompoent:GetRenderEnable();
			if(renderEnable == true) then
				local buttonIndex = childUICompoent:GetButtonIndex();
				if(buttonIndex ~= self.CurrentSelectedChildButtonIndex) then
				
					childUICompoent:SetRenderEnable(false);
				end
			else
				
				childUICompoent:SetRenderEnable(true);
			end
			
		end
	end
	
end

--Method
DropDown.OnKey = function(self, key, state)
	if(SCANCODE.ENTER == key) then
		self.ENTER = state;
	end
end
DropDown.OnJoystickButton = function(self, joystickId, button, state)
	if(button == CONTROLLER.A) then
		self.ENTER = state;
	end
end
DropDown.OnMouseMotion = function(self, position, deltaposition)
	self.MousePositionX = position.x;
	self.MousePositionY = position.y;
end
DropDown.OnMouseClick = function(self, button, state)
	if(button == 1) then
		self.LEFTCLICK = state;
	elseif(button == 2) then
		self.RIGHTCLICK = state;
	end
end

DropDown.OnDestruction = function(self, key, state)
	OnKeyEvent():Unbind({self, self.OnKey});
	OnMouseMotion():Unbind({self, self.OnMouseMotion});
	OnMouseClick():Unbind({self, self.OnMouseClick});
	OnJoystickButton():Unbind({self, self.OnJoystickButton});

end
DropDown.SetArrowClick = function(self, val)
	self.ClickedButtonArrow = val;
	if(val == true) then
		_G.CurrentButtonTouched = self.ButtonArrowIndex;
	end
	
end
DropDown.GetButtonArrowIndex = function(self)
	return self.ButtonArrowIndex;
end
DropDown.StartButtonIndex = function(self)
	return self.StartButtonCount;
end
DropDown.EndButtonIndex = function(self)
	return self.EndButtonCount;
end
DropDown.GetArrowClick = function(self)
	return self.ButtonArrowSelected;
end
DropDown.GetArrowClickInstance = function(self)
	return self.ClickedButtonArrow;
end
DropDown.SetCurrentChildButtonSelect = function(self, val)
	 self.CurrentSelectedChildButtonIndex = val;
end
DropDown.GetCurrentChildButtonSelect = function(self)
	return self.CurrentSelectedChildButtonIndex;
end
DropDown.Disable = function(self)
   self.Enabled = false;
end
DropDown.GetParentName = function(self)
   return self.ParentName;
end

return DropDown;