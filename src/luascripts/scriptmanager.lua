--Cracker64's Autorun Script Manager
--The autorun to end all autoruns
--Version 3.4

--TODO:
--manual file addition (that can be anywhere and any extension)
--Moving window (because why not)
--some more API functions
--prettier, organize code

--CHANGES:
--Version 3.4: some new buttons, better tooltips, fix 'Change dir' button, fix broken buttons on OS X
--Version 3.3: fix apostophes in filenames, allow authors to rename their scripts on the server
--Version 3.2: put MANAGER stuff in table, fix displaying changelogs
--Version 3.1: Organize scripts less randomly, fix scripts being run twice, fix other bugs
--central script / update server at starcatcher.us / delete local scripts / lots of other things by jacob1 v3.0
--Scan all subdirectories in scripts folder! v2.25
--Remove step hooks, v87 fixes them
--Socket is now default in v87+ , horray, everyone can now use update features without extra downloads.
--Handles up to 50 extra step functions, up from the default 5 (not including the manager's step) v2.1
--Other various nice API functions
--Scripts can store/retrieve settings through the manager, see comments below v2.0
--Small fillrect change for v85, boxes can have backgrounds v1.92
--Support spaces in exe names v1.91
--Auto-update for OTHER scripts now works, is a bit messy, will fix later, but nothing should change for users to use this
--  Place a line '--VER num UPDATE link' in one of the first four lines of the file, see my above example
--  The link at top downloads a file that contains ONLY version,full link,and prints the rest(changelog). See my link for example

if not socket then error("TPT version not supported") end
if MANAGER then error("manager is already running") end
if tpt.version.jacob1s_mod == 30 and tpt.version.jacob1s_mod_minor == 0 then
	return
end

local scriptversion = 6
MANAGER = {["version"] = "3.4", ["scriptversion"] = scriptversion, ["hidden"] = true}

local TPT_LUA_PATH = 'scripts'
local PATH_SEP = '\\'
local OS = "Windows"
local jacobsmod = tpt.version.jacob1s_mod
local CHECKUPDATE = false
if os.getenv('HOME') then
	PATH_SEP = '/'
	if fs.exists("/Applications") then
		OS = "OSX"
	else
		OS = "Linux"
	end
end
local EXE_NAME
if OS == "Windows" then
	EXE_NAME = jacobsmod and "Jacob1\'s Mod.exe" or "Powder.exe"
elseif OS == "Linux" then
	EXE_NAME = jacobsmod and "Jacob1\'s Mod" or "powder"
elseif OS == "OSX" then
	EXE_NAME = "powder-x" --can't restart on OS X
end
local filenames = {}
local num_files = 0 --downloaded scripts aren't stored in filenames
local localscripts = {}
local onlinescripts = {}
local running = {}
local requiresrestart=false
local online = false
local first_online = true
local updatetable --temporarily holds info on script manager updates
local gen_buttons
local sidebutton
local download_file
local settings = {}
math.randomseed(os.time()) math.random() math.random() math.random() --some filler randoms

--get line that can be saved into scriptinfo file
local function scriptInfoString(info)
	--Write table into data format
	if type(info)~="table" then return end
	local t = {}
	for k,v in pairs(info) do
		table.insert(t,k..":\""..v.."\"")
	end
	local rstr = table.concat(t,","):gsub("\r",""):gsub("\n","\\n")
	return rstr
end

--read a scriptinfo line
local function readScriptInfo(list)
	if not list then return {} end
	local scriptlist = {}
	for i in list:gmatch("[^\n]+") do
		local t = {}
		local ID = 0
		for k,v in i:gmatch("(%w+):\"([^\"]*)\"") do
			t[k]= tonumber(v) or v:gsub("\r",""):gsub("\\n","\n")
		end
		scriptlist[t.ID] = t
	end
	return scriptlist
end

--save settings
local function save_last()
	local savestring=""
	for script,v in pairs(running) do
		savestring = savestring.." \""..script.."\""
	end
	savestring = "SAV "..savestring.."\nEXE "..EXE_NAME.."\nDIR "..TPT_LUA_PATH
	for k,t in pairs(settings) do
	for n,v in pairs(t) do
		savestring = savestring.."\nSET "..k.." "..n..":\""..v.."\""
	end
	end
	local f
	if TPT_LUA_PATH == "scripts" then
		f = io.open(TPT_LUA_PATH..PATH_SEP.."autorunsettings.txt", "w")
	else
		f = io.open("autorunsettings.txt", "w")
	end
	if f then
		f:write(savestring)
		f:close()
	end

	f = io.open(TPT_LUA_PATH..PATH_SEP.."downloaded"..PATH_SEP.."scriptinfo", "w")
	if f then
		for k,v in pairs(localscripts) do
			f:write(scriptInfoString(v).."\n")
		end
		f:close()
	end
end

local function load_downloaded()
	local f = io.open(TPT_LUA_PATH..PATH_SEP.."downloaded"..PATH_SEP.."scriptinfo","r")
	if f then
		local lines = f:read("*a")
		f:close()
		localscripts = readScriptInfo(lines)
		for k,v in pairs(localscripts) do
			if k ~= 1 then
				if not v["ID"] or not v["name"] or not v["description"] or not v["path"] or not v["version"] then
					localscripts[k] = nil
				elseif not fs.exists(TPT_LUA_PATH.."/"..v["path"]:gsub("\\","/")) then
					 localscripts[k] = nil
				end
			end
		end
	end
end

--load settings before anything else
local function load_last()
	local f = io.open(TPT_LUA_PATH..PATH_SEP.."autorunsettings.txt","r")
	if not f then
		f = io.open("autorunsettings.txt","r")
	end
	if f then
		local lines = {}
		local line = f:read("*l")
		while line do
			table.insert(lines,(line:gsub("\r","")))
			line = f:read("*l")
		end
		f:close()
		for i=1, #lines do
			local tok=lines[i]:sub(1,3)
			local str=lines[i]:sub(5)
			if tok=="SAV" then
				for word in string.gmatch(str, "\"(.-)\"") do running[word] = true end
			elseif tok=="EXE" then
				EXE_NAME=str
			elseif tok=="DIR" then
				TPT_LUA_PATH=str
			elseif tok=="SET" then
				local ident,name,val = string.match(str,"(.-) (.-):\"(.-)\"")
				if settings[ident] then settings[ident][name]=val
				else settings[ident]={[name]=val} end
			end
		end
	end

	load_downloaded()
end
load_last()
--get list of files in scripts folder
local function load_filenames()
	filenames = {}
	local function searchRecursive(directory)
		local dirlist = fs.list(directory)
		if not dirlist then return end
		for i,v in ipairs(dirlist) do
			local file = directory.."/"..v
			if fs.isDirectory(file) and v ~= "downloaded" then
				searchRecursive(file)
			elseif fs.isFile(file) then
				if file:find("%.lua$") then
					local toinsert = file:sub(#TPT_LUA_PATH+2)
					if OS == "Windows" then
						toinsert = toinsert:gsub("/", "\\") --not actually required
					end
					table.insert(filenames, toinsert)
				end
			end
		end
	end
	searchRecursive(TPT_LUA_PATH)
	table.sort(filenames, function(first,second) return first:lower() < second:lower() end)
end
--ui object stuff
local ui_base local ui_box local ui_line local ui_text local ui_button local ui_scrollbar local ui_tooltip local ui_checkbox local ui_console local ui_window
local tooltip
ui_base = {
new = function()
	local b={}
	b.drawlist = {}
	function b:drawadd(f)
		table.insert(self.drawlist,f)
	end
	function b:draw(...)
		for _,f in ipairs(self.drawlist) do
			if type(f)=="function" then
				f(self,...)
			end
		end
	end
	b.movelist = {}
	function b:moveadd(f)
		table.insert(self.movelist,f)
	end
	function b:onmove(x,y)
		for _,f in ipairs(self.movelist) do
			if type(f)=="function" then
				f(self,x,y)
			end
		end
	end
	return b
end
}
ui_box = {
new = function(x,y,w,h,r,g,b)
	local box=ui_base.new()
	box.x=x box.y=y box.w=w box.h=h box.x2=x+w box.y2=y+h
	box.r=r or 255 box.g=g or 255 box.b=b or 255
	function box:setcolour(r,g,b) self.r=r self.g=g self.b=b end
	function box:setbackground(r,g,b,a) self.br=r self.bg=g self.bb=b self.ba=a end
	box.drawbox=true
	box.drawbackground=false
	box:drawadd(function(self) if self.drawbackground then tpt.fillrect(self.x,self.y,self.w+1,self.h+1,self.br,self.bg,self.bb,self.ba) end
								if self.drawbox then tpt.drawrect(self.x,self.y,self.w,self.h,self.r,self.g,self.b) end end)
	box:moveadd(function(self,x,y)
		if x then self.x=self.x+x self.x2=self.x2+x end
		if y then self.y=self.y+y self.y2=self.y2+y end
	end)
	return box
end
}
ui_line = {
new=function(x,y,x2,y2,r,g,b)
	local line=ui_box.new(x,y,x2-x,y2-y,r,g,b)
	--Line is essentially a box, but with a different draw
	line.drawlist={}
	line:drawadd(function(self) tpt.drawline(self.x,self.y,self.x2,self.y2,self.r,self.g,self.b) end)
	return line
	end
}
ui_text = {
new = function(text,x,y,r,g,b)
	local txt = ui_base.new()
	txt.text = text
	txt.x=x or 0 txt.y=y or 0 txt.r=r or 255 txt.g=g or 255 txt.b=b or 255
	function txt:setcolour(r,g,b) self.r=r self.g=g self.b=b end
	txt:drawadd(function(self,x,y) tpt.drawtext(x or self.x,y or self.y,self.text,self.r,self.g,self.b) end)
	txt:moveadd(function(self,x,y)
		if x then self.x=self.x+x end
		if y then self.y=self.y+y end
	end)
	function txt:process() return false end
	return txt
end,
--Scrolls while holding mouse over
newscroll = function(text,x,y,vis,r,g,b)
	local txt = ui_text.new(text,x,y,r,g,b)
	if tpt.textwidth(text)<vis then return txt end
	txt.visible=vis
	txt.length=string.len(text)
	txt.start=1
	txt.drawlist={} --reset draw
	txt.timer=socket.gettime()+3
	function txt:cuttext(self)
		local last = self.start+1
		while tpt.textwidth(self.text:sub(self.start,last))<txt.visible and last<=self.length do
			last = last+1
		end
		self.last=last-1
	end
	txt:cuttext(txt)
	txt.minlast=txt.last-1
	txt.ppl=((txt.visible-6)/(txt.length-txt.minlast+1))
	txt:drawadd(function(self,x,y)
		if socket.gettime() > self.timer then
			if self.last >= self.length then
				self.start = 1
				self:cuttext(self)
				self.timer = socket.gettime()+3
			else
				self.start = self.start + 1
				self:cuttext(self)
				if self.last >= self.length then
					self.timer = socket.gettime()+3
				else
					self.timer = socket.gettime()+.15
				end
			end
		end
		tpt.drawtext(x or self.x,y or self.y, self.text:sub(self.start,self.last) ,self.r,self.g,self.b)
	end)
	function txt:process(mx,my,button,event,wheel)
		if event==3 then
			local newlast = math.floor((mx-self.x)/self.ppl)+self.minlast
			if newlast<self.minlast then newlast=self.minlast end
			if newlast>0 and newlast~=self.last then
				local newstart=1
				while tpt.textwidth(self.text:sub(newstart,newlast))>= self.visible do
					newstart=newstart+1
				end
				self.start=newstart self.last=newlast
				self.timer = socket.gettime()+3
			end
		end
	end
	return txt
end
}
ui_scrollbar = {
new = function(x,y,h,t,m)
	local bar = ui_base.new() --use line object as base?
	bar.x=x bar.y=y bar.h=h
	bar.total=t
	bar.numshown=m
	bar.pos=0
	bar.length=math.floor((1/math.ceil(bar.total-bar.numshown+1))*bar.h)
	bar.soffset=math.floor(bar.pos*((bar.h-bar.length)/(bar.total-bar.numshown)))
	function bar:update(total,shown,pos)
		self.pos=pos or 0
		if self.pos<0 then self.pos=0 end
		self.total=total
		self.numshown=shown
		self.length= math.floor((1/math.ceil(self.total-self.numshown+1))*self.h)
		self.soffset= math.floor(self.pos*((self.h-self.length)/(self.total-self.numshown)))
	end
	function bar:move(wheel)
		self.pos = self.pos-wheel
		if self.pos < 0 then self.pos=0 end
		if self.pos > (self.total-self.numshown) then self.pos=(self.total-self.numshown) end
		self.soffset= math.floor(self.pos*((self.h-self.length)/(self.total-self.numshown)))
	end
	bar:drawadd(function(self)
		if self.total > self.numshown then
			tpt.drawline(self.x,self.y+self.soffset,self.x,self.y+self.soffset+self.length)
		end
	end)
	bar:moveadd(function(self,x,y)
		if x then self.x=self.x+x end
		if y then self.y=self.y+y end
	end)
	function bar:process(mx,my,button,event,wheel)
		if wheel~=0 and not MANAGER.hidden then
			if self.total > self.numshown then
				local previous = self.pos
				self:move(wheel)
				if self.pos~=previous then
					return previous-self.pos
				end
			end
		end
		--possibly click the bar and drag?
		return false
	end
	return bar
end
}
ui_button = {
new = function(x,y,w,h,f,text)
	local b = ui_box.new(x,y,w,h)
	b.f=f
	b.t=ui_text.new(text,x+2,y+2)
	b.drawbox=false
	b.clicked=false
	b.almostselected=false
	b.invert=true
	b:setbackground(127,127,127,125)
	b:drawadd(function(self)
		if self.invert and self.almostselected then
			self.almostselected=false
			tpt.fillrect(self.x,self.y,self.w,self.h)
			local tr=self.t.r local tg=self.t.g local tb=self.t.b
			b.t:setcolour(0,0,0)
			b.t:draw()
			b.t:setcolour(tr,tg,tb)
		else
			if tpt.mousex>=self.x and tpt.mousex<=self.x2 and tpt.mousey>=self.y and tpt.mousey<=self.y2 then
				self.drawbackground=true
			else
				self.drawbackground=false
			end
			b.t:draw()
		end
	end)
	b:moveadd(function(self,x,y)
		self.t:onmove(x,y)
	end)
	function b:process(mx,my,button,event,wheel)
		local clicked = self.clicked
		if event==2 then self.clicked = false end
		if mx<self.x or mx>self.x2 or my<self.y or my>self.y2 then return false end
		if event==1 then
			self.clicked=true
		elseif clicked then
			if event==3 then self.almostselected=true end
			if event==2 then self:f() end
			return true
		end
	end
	return b
end
}
ui_tooltip = {
new = function(x,y,w,text)
	local b = ui_box.new(x,y-1,w,0)
	function b:updatetooltip(tooltip)
		self.tooltip = tooltip
		self.length = #tooltip
		self.lines = 1

		local linebreak,lastspace = 0,nil
		for i=0,#self.tooltip do
			local width = tpt.textwidth(tooltip:sub(linebreak,i+1))
			if width > self.w/2 and tooltip:sub(i,i):match("[%s,_%.%-?!]") then
				lastspace = i
			end
			local isnewline = (self.tooltip:sub(i,i) == '\n')
			if width > self.w or isnewline then
				local pos = (i==#tooltip or not lastspace) and i or lastspace
				self.lines = self.lines + 1
				if self.tooltip:sub(pos,pos) == ' ' then
					self.tooltip = self.tooltip:sub(1,pos-1).."\n"..self.tooltip:sub(pos+1)
				elseif not isnewline then
					self.length = self.length + 1
					self.tooltip = self.tooltip:sub(1,pos-1).."\n"..self.tooltip:sub(pos)
					i = i + 1
					pos = pos + 1
				end
				linebreak = pos+1
				lastspace = nil
			end
		end
		self.h = self.lines*12+2
		--self.w = tpt.textwidth(self.tooltip)+3
		self.drawbox = tooltip ~= ""
		self.drawbackground = tooltip ~= ""
	end
	function b:settooltip(tooltip_)
		tooltip:onmove(tpt.mousex+5-tooltip.x, tpt.mousey+5-tooltip.y)
		tooltip:updatetooltip(tooltip_)
	end
	b:updatetooltip(text)
	b:setbackground(0,0,0,255)
	b.drawbackground = true
	b:drawadd(function(self)
		if self.tooltip ~= "" then
			tpt.drawtext(self.x+1,self.y+2,self.tooltip)
		end
		self:updatetooltip("")
	end)
	function b:process(mx,my,button,event,wheel) end
	return b
end
}
ui_checkbox = {
up_button = function(x,y,w,h,f,text)
	local b=ui_button.new(x,y,w,h,f,text)
	b.canupdate=false
	return b
end,
new_button = function(x,y,w,h,splitx,f,f2,text,localscript)
	local b = ui_box.new(x,y,splitx,h)
	b.f=f b.f2=f2
	b.localscript=localscript
	b.splitx = splitx
	b.t=ui_text.newscroll(text,x+24,y+2,splitx-24)
	b.clicked=false
	b.selected=false
	b.checkbut=ui_checkbox.up_button(x+splitx+9,y,33,9,ui_button.scriptcheck,"Update")
	b.drawbox=false
	b:setbackground(127,127,127,100)
	b:drawadd(function(self)
		if self.t.text == "" then return end
		self.drawbackground = false
		if tpt.mousey >= self.y and tpt.mousey < self.y2 then
			if tpt.mousex >= self.x and tpt.mousex < self.x+8 then
				if self.localscript then
					tooltip:settooltip("delete this script")
				else
					tooltip:settooltip("view script in browser")
				end
			elseif tpt.mousex>=self.x and tpt.mousex<self.x2 then
				local script
				if online and onlinescripts[self.ID]["description"] then
					script = onlinescripts[self.ID]
				elseif not online and localscripts[self.ID] then
					script = localscripts[self.ID]
				end
				if script then
					tooltip:settooltip(script["name"].." by "..script["author"].."\n\n"..script["description"])
				end
				self.drawbackground = true
			elseif tpt.mousex >= self.x2 then
				if tpt.mousex < self.x2+9 and self.running then
					tooltip:settooltip(online and "downloaded" or "running")
				elseif tpt.mousex >= self.x2+9 and tpt.mousex < self.x2+43 and self.checkbut.canupdate and onlinescripts[self.ID] and onlinescripts[self.ID]["changelog"] then
					tooltip:settooltip(onlinescripts[self.ID]["changelog"])
				end
			end
		end
		self.t:draw()
		if self.localscript then
			if self.deletealmostselected then
				self.deletealmostselected = false
				tpt.drawtext(self.x+1, self.y+1, "\134", 255, 48, 32, 255)
			else
				tpt.drawtext(self.x+1, self.y+1, "\134", 160, 48, 32, 255)
			end
			tpt.drawtext(self.x+1, self.y+1, "\133", 255, 255, 255, 255)
		else
			tpt.drawtext(self.x+1, self.y+1, "\147", 255, 200, 80, 255)
		end
		tpt.drawrect(self.x+12,self.y+1,8,8)
		if self.almostselected then self.almostselected=false tpt.fillrect(self.x+12,self.y+1,8,8,150,150,150)
		elseif self.selected then tpt.fillrect(self.x+12,self.y+1,8,8) end
		local filepath = self.ID and localscripts[self.ID] and localscripts[self.ID]["path"] or self.t.text
		if self.running then tpt.drawtext(self.x+self.splitx+2,self.y+2,online and "D" or "R") end
		if self.checkbut.canupdate then self.checkbut:draw() end
	end)
	b:moveadd(function(self,x,y)
		self.t:onmove(x,y)
		self.checkbut:onmove(x,y)
	end)
	function b:process(mx,my,button,event,wheel)
		if self.f2 and mx <= self.x+8 then
			if event==1 then
				self.clicked = 1
			elseif self.clicked == 1 then
				if event==3 then self.deletealmostselected = true end
				if event==2 then self:f2() end
			end
		elseif self.f and mx<=self.x+self.splitx then
			if event==1 then
				self.clicked = 2
			elseif self.clicked == 2 then
				if event==3 then self.almostselected=true end
				if event==2 then self:f() end
				self.t:process(mx,my,button,event,wheel)
			end
		else
			if self.checkbut.canupdate then self.checkbut:process(mx,my,button,event,wheel) end
		end
		return true
	end
	return b
end,
new = function(x,y,w,h)
	local box = ui_box.new(x,y,w,h)
	box.list={}
	box.numlist = 0
	box.max_lines = math.floor(box.h/10)-1
	box.max_text_width = math.floor(box.w*0.8)
	box.splitx=x+box.max_text_width
	box.scrollbar = ui_scrollbar.new(box.x2-2,box.y+11,box.h-12,0,box.max_lines)
	box.lines={
		ui_line.new(box.x+1,box.y+10,box.x2-1,box.y+10,170,170,170),
		ui_line.new(box.x+22,box.y+10,box.x+22,box.y2-1,170,170,170),
		ui_line.new(box.splitx,box.y+10,box.splitx,box.y2-1,170,170,170),
		ui_line.new(box.splitx+9,box.y+10,box.splitx+9,box.y2-1,170,170,170),
	}
	function box:updatescroll()
		self.scrollbar:update(self.numlist,self.max_lines)
	end
	function box:clear()
		self.list={}
		self.numlist=0
	end
	function box:add(f,f2,text,localscript)
		local but = ui_checkbox.new_button(self.x,self.y+1+((self.numlist+1)*10),tpt.textwidth(text)+4,10,self.max_text_width,f,f2,text,localscript)
		table.insert(self.list,but)
		self.numlist = #self.list
		return but
	end
	box:drawadd(function (self)
		tpt.drawtext(self.x+24,self.y+2,"Files in "..TPT_LUA_PATH.." folder")
		tpt.drawtext(self.splitx+11,self.y+2,"Update")
		for i,line in ipairs(self.lines) do
			line:draw()
		end
		self.scrollbar:draw()
		local restart = false
		for i,check in ipairs(self.list) do
			local filepath = check.ID and localscripts[check.ID] and localscripts[check.ID]["path"] or check.t.text
			if not check.selected and running[filepath] then
				restart = true
			end
			if i>self.scrollbar.pos and i<=self.scrollbar.pos+self.max_lines then
				check:draw()
			end
		end
		requiresrestart = restart and not online
	end)
	box:moveadd(function(self,x,y)
		for i,line in ipairs(self.lines) do
			line:onmove(x,y)
		end
		for i,check in ipairs(self.list) do
			check:onmove(x,y)
		end
	end)
	function box:scroll(amount)
		local move = amount*10
		if move==0 then return end
		for i,check in ipairs(self.list) do
			check:onmove(0,move)
		end
	end
	function box:process(mx,my,button,event,wheel)
		if mx<self.x or mx>self.x2 or my<self.y or my>self.y2-7 then return false end
		local scrolled = self.scrollbar:process(mx,my,button,event,wheel)
		if scrolled then self:scroll(scrolled) end
		local which = math.floor((my-self.y-11)/10)+1
		if which>0 and which<=self.numlist then self.list[which+self.scrollbar.pos]:process(mx,my,button,event,wheel) end
		if event == 2 then
			for i,v in ipairs(self.list) do v.clicked = false end
		end
		return true
	end
	return box
end
}
ui_console = {
new = function(x,y,w,h)
	local con = ui_box.new(x,y,w,h)
	con.shown_lines = math.floor(con.h/10)
	con.max_lines = 300
	con.max_width = con.w-4
	con.lines = {}
	con.scrollbar = ui_scrollbar.new(con.x2-2,con.y+1,con.h-2,0,con.shown_lines)
	con:drawadd(function(self)
		self.scrollbar:draw()
		local count=0
		for i,line in ipairs(self.lines) do
			if i>self.scrollbar.pos and i<= self.scrollbar.pos+self.shown_lines then
				line:draw(self.x+3,self.y+3+(count*10))
				count = count+1
			end
		end
	end)
	con:moveadd(function(self,x,y)
		self.scrollbar:onmove(x,y)
	end)
	function con:clear()
		self.lines = {}
		self.scrollbar:update(0,con.shown_lines)
	end
	function con:addstr(str,r,g,b)
		str = tostring(str)
		local nextl = str:find('\n')
		while nextl do
			local line = str:sub(1,nextl-1)
			self:addline(line,r,g,b)
			str = str:sub(nextl+1)
			nextl = str:find('\n')
		end
		self:addline(str,r,g,b) --anything leftover
	end
	function con:addline(line,r,g,b)
		if not line or line=="" then return end --No blank lines
		table.insert(self.lines,ui_text.newscroll(line,self.x,0,self.max_width,r,g,b))
		if #self.lines>self.max_lines then table.remove(self.lines,1) end
		self.scrollbar:update(#self.lines,self.shown_lines,#self.lines-self.shown_lines)
	end
	function con:process(mx,my,button,event,wheel)
		if mx<self.x or mx>self.x2 or my<self.y or my>self.y2 then return false end
		self.scrollbar:process(mx,my,button,event,wheel)
		local which = math.floor((my-self.y-1)/10)+1
		if which>0 and which<=self.shown_lines and self.lines[which+self.scrollbar.pos] then self.lines[which+self.scrollbar.pos]:process(mx,my,button,event,wheel) end
		return true
	end
	return con
end
}
ui_window = {
new = function(x,y,w,h)
	local w=ui_box.new(x,y,w,h)
	w.sub={}
	function w:add(m,name)
		if name then w[name]=m end
		table.insert(self.sub,m)
	end
	w:drawadd(function(self)
		for i,sub in ipairs(self.sub) do
			sub:draw()
		end
	end)
	w:moveadd(function(self,x,y)
		for i,sub in ipairs(self.sub) do
			sub:onmove(x,y)
		end
	end)
	function w:process(mx,my,button,event,wheel)
		if mx<self.x or mx>self.x2 or my<self.y or my>self.y2 then if button == 0 then return end ui_button.sidepressed() return true end
		local ret
		for i,sub in ipairs(self.sub) do
			if sub:process(mx,my,button,event,wheel) then ret = true end
		end
		return ret
	end
	return w
end
}
--Main window with everything!
local mainwindow = ui_window.new(50,50,525,300)
mainwindow:setbackground(10,10,10,235) mainwindow.drawbackground=true
mainwindow:add(ui_console.new(275,148,300,189),"menuconsole")
mainwindow:add(ui_checkbox.new(50,80,225,257),"checkbox")
tooltip = ui_tooltip.new(0,1,250,"")

--Some API functions you can call from other scripts
--put 'using_manager=MANAGER ~= nil' or similar in your scripts, using_manager will be true if the manager is active
--Print a message to the manager console, can be coloured
function MANAGER.print(msg,...)
	mainwindow.menuconsole:addstr(msg,...)
end
--downloads and returns a file, so you can do whatever...
local download_file
function MANAGER.download(url)
	return download_file(url)
end
function MANAGER.scriptinfo(id)
	local url = "http://starcatcher.us/scripts/main.lua"
	if id then
		url = url.."?info="..id
	end
	local info = download_file(url)
	infotable = readScriptInfo(info)
	return id and infotable[id] or infotable
end
--Get various info about the system (operating system, script directory, path seperator, if socket is loaded)
function MANAGER.sysinfo()
	return {["OS"]=OS, ["scriptDir"]=TPT_LUA_PATH, ["pathSep"]=PATH_SEP, ["exeName"] = EXE_NAME}
end
--Save a setting in the autorun settings file, ident should be your script name no one else would use.
--Name is variable name, val is the value which will be saved/returned as a string
function MANAGER.savesetting(ident,name,val)
	ident = tostring(ident)
	name = tostring(name)
	val = tostring(val)
	if settings[ident] then settings[ident][name]=val
	else settings[ident]={[name]=val} end
	save_last()
end
--Get a previously saved value, if it has one
function MANAGER.getsetting(ident,name)
	if settings[ident] then return settings[ident][name] end
	return nil
end
--delete a setting, leave name nil to delete all of ident
function MANAGER.delsetting(ident,name)
	if settings[ident] then
	if name then settings[ident][name]=nil
	else settings[ident]=nil end
	save_last()
	end
end

--mniip's download thing (mostly)
local pattern = "http://w*%.?(.-)(/.*)"
function download_file(url)
	local _,_,host,rest = url:find(pattern)
	if not host or not rest then MANAGER.print("Bad link") return end
	local conn=socket.tcp()
	if not conn then return end
	local succ=pcall(conn.connect,conn,host,80)
	conn:settimeout(5)
	if not succ then return end
	local userAgent = "PowderToy/"..tpt.version.major.."."..tpt.version.minor.."."..tpt.version.build.." ("..(OS == "Windows" and "WIN; " or (os == "Linux" and "LIN; " or "OSX; "))..(jacobsmod and "M1" or "M0")..") SCRIPT/"..MANAGER.version
	succ,resp,something=pcall(conn.send,conn,"GET "..rest.." HTTP/1.1\r\nHost: "..host.."\r\nConnection: close\r\nUser-Agent: "..userAgent.."\r\n\n")
	if not succ then return end
	local data=""
	local c=""
	while c do
		c=conn:receive("*l")
		if c then
			data=data.."\n"..c
		end
	end
	if data=="" then MANAGER.print("no data") return end
	local first,last,code = data:find("HTTP/1%.1 (.-) .-\n")
	while last do
		data = data:sub(last+1)
		first,last,header = data:find("^([^\n]-:.-)\n")
		--read something from headers?
		if header then
			if tonumber(code)==302 then
				local _,_,new = header:find("^Location: (.*)")
				if new then return download_file(new) end
			end
		end
	end
	if host:find("pastebin.com") then --pastebin adds some weird numbers
		_,_,data=data:find("\n[^\n]*\n(.*)\n.+\n$")
	end
	return data
end
--Downloads to a location
local function download_script(ID,location)
	local file = download_file("http://starcatcher.us/scripts/main.lua?get="..ID)
	if file then
		f=io.open(location,"w")
		f:write(file)
		f:close()
		return true
	end
	return false
end
--Restart exe (if named correctly)
local function do_restart()
	save_last()
	if OS == "Windows" then
		os.execute("TASKKILL /IM \""..EXE_NAME.."\" /F &&START .\\\""..EXE_NAME.."\"")
	elseif OS == "Linux" then
		os.execute("killall -s KILL \""..EXE_NAME.."\" && ./\""..EXE_NAME.."\"")
	elseif OS == "OSX" then
		MANAGER.print("Can't restart TPT on OS X, please close and reopen The Powder Toy")
		return
	end
	MANAGER.print("Restart failed, do you have the exe name right?",255,0,0)
end
--TPT interface
local function step()
	if jacobsmod then
		tpt.fillrect(0,0,gfx.WIDTH,gfx.HEIGHT,0,0,0,150)
	else
		tpt.fillrect(-1,-1,gfx.WIDTH,gfx.HEIGHT,0,0,0,150)
	end
	mainwindow:draw()
	tpt.drawtext(280,140,"Console Output:")
	if requiresrestart then
		tpt.drawtext(280,88,"Disabling a script requires a restart for effect!",255,50,50)
	end
	tpt.drawtext(55,55,"Click a script to toggle, hit DONE when finished")
	tpt.drawtext(474,55,"Script Manager v"..MANAGER.version)--479 for simple versions
	tooltip:draw()
end
local function mouseclick(mousex,mousey,button,event,wheel)
	sidebutton:process(mousex,mousey,button,event,wheel)
	if MANAGER.hidden then return true end

	if mousex>612 or mousey>384 then return false end
	mainwindow:process(mousex,mousey,button,event,wheel)
	return false
end
local jacobsmod_old_menu_check = false
local function keypress(key,nkey,modifier,event)
	if jacobsmod and key == 'o' and event == 1 then jacobsmod_old_menu_check = true end
	if nkey==27 and not MANAGER.hidden then MANAGER.hidden=true return false end
	if not MANAGER.hidden then return false end
end
--small button on right to bring up main menu
local WHITE = {255,255,255,255}
local BLACK = {0,0,0,255}
local ICON = math.random(2) --pick a random icon
local lua_letters= {{{2,2,2,7},{2,7,4,7},{6,7,6,11},{6,11,8,11},{8,7,8,11},{10,11,12,11},{10,11,10,15},{11,13,11,13},{12,11,12,15},},
	{{2,3,2,13},{2,14,7,14},{4,3,4,12},{4,12,7,12},{7,3,7,12},{9,3,12,3},{9,3,9,14},{10,8,11,8},{12,3,12,14},}}
local function smallstep()
	gfx.drawRect(sidebutton.x, sidebutton.y+1, sidebutton.w+1, sidebutton.h+1,200,200,200)
	local colour=WHITE
	if not MANAGER.hidden then
		step()
		gfx.fillRect(sidebutton.x, sidebutton.y+1, sidebutton.w+1, sidebutton.h+1)
		colour=BLACK
	end
	for i,dline in ipairs(lua_letters[ICON]) do
		tpt.drawline(dline[1]+sidebutton.x,dline[2]+sidebutton.y,dline[3]+sidebutton.x,dline[4]+sidebutton.y,colour[1],colour[2],colour[3])
	end
	if jacobsmod_old_menu_check then
		if tpt.oldmenu()==0 and sidebutton.y > 150 then sidebutton:onmove(0, -256) elseif tpt.oldmenu()==1 and sidebutton.y < 150 then sidebutton:onmove(0, 256) end
		jacobsmod_old_menu_check = false
	end
end
--button functions on click
function ui_button.reloadpressed(self)
	load_filenames()
	load_downloaded()
	gen_buttons()
	mainwindow.checkbox:updatescroll()
	if num_files == 0 then
		MANAGER.print("No scripts found in '"..TPT_LUA_PATH.."' folder",255,255,0)
		fs.makeDirectory(TPT_LUA_PATH)
	else
		MANAGER.print("Reloaded file list, found "..num_files.." scripts")
	end
end
function ui_button.selectnone(self)
	for i,but in ipairs(mainwindow.checkbox.list) do
		but.selected = false
	end
end
function ui_button.consoleclear(self)
	mainwindow.menuconsole:clear()
end
function ui_button.changeexename(self)
	local last = EXE_NAME
	local new = tpt.input("Change exe name","Enter the exact name of powder toy executable",EXE_NAME,EXE_NAME)
	if new~=last and new~="" then
		MANAGER.print("Executable name changed to "..new,255,255,0)
		EXE_NAME = new
	end
	save_last()
end
function ui_button.changedir(self)
	local last = TPT_LUA_PATH
	local new = tpt.input("Change search directory","Enter the folder where your scripts are",TPT_LUA_PATH,TPT_LUA_PATH)
	if new~=last and new~="" then
		fs.removeFile(last..PATH_SEP.."autorunsettings.txt")
		MANAGER.print("Directory changed to "..new,255,255,0)
		TPT_LUA_PATH = new
	end
	ui_button.reloadpressed()
	save_last()
end
function ui_button.uploadscript(self)
	local command = OS == "Windows" and "start" or (OS == "Linux" and "xdg-open" or "open")
	if online then
		os.execute(command.." http://starcatcher.us/scripts/#submit-page")
	else
		os.execute(command.." "..TPT_LUA_PATH)
	end
end
local lastpaused
function ui_button.sidepressed(self)
	if TPTMP and TPTMP.chatHidden == false then print("minimize TPTMP before opening the manager") return end
	MANAGER.hidden = not MANAGER.hidden
	ui_button.localview()
	if not MANAGER.hidden then
		lastpaused = tpt.set_pause()
		tpt.set_pause(1)
		ui_button.reloadpressed()
	else
		tpt.set_pause(lastpaused)
	end
end
local donebutton
function ui_button.donepressed(self)
	MANAGER.hidden = true
	for i,but in ipairs(mainwindow.checkbox.list) do
		local filepath = but.ID and localscripts[but.ID]["path"] or but.t.text
		if but.selected then
			if requiresrestart then
				running[filepath] = true
			else
				if not running[filepath] then
					local status,err = pcall(dofile,TPT_LUA_PATH..PATH_SEP..filepath)
					if not status then
						MANAGER.print(err,255,0,0)
						print(err)
						but.selected = false
					else
						MANAGER.print("Started "..filepath)
						running[filepath] = true
					end
				end
			end
		elseif running[filepath] then
			running[filepath] = nil
		end
	end
	if requiresrestart then do_restart() return end
	save_last()
end
function ui_button.downloadpressed(self)
	for i,but in ipairs(mainwindow.checkbox.list) do
		if but.selected then
			--maybe do better display names later
			local displayName
			local function get_script(butt)
				local script = download_file("http://starcatcher.us/scripts/main.lua?get="..butt.ID)
				displayName = "downloaded"..PATH_SEP..butt.ID.." "..onlinescripts[butt.ID].author.."-"..onlinescripts[butt.ID].name..".lua"
				local name = TPT_LUA_PATH..PATH_SEP..displayName
				if not fs.exists(TPT_LUA_PATH..PATH_SEP.."downloaded") then
					fs.makeDirectory(TPT_LUA_PATH..PATH_SEP.."downloaded")
				end
				local file = io.open(name, "w")
				if not file then error("could not open "..name) end
				file:write(script)
				file:close()
				if localscripts[butt.ID] and localscripts[butt.ID]["path"] ~= displayName then
					local oldpath = localscripts[butt.ID]["path"]
					fs.removeFile(TPT_LUA_PATH.."/"..oldpath:gsub("\\","/"))
					running[oldpath] = nil
				end
				localscripts[butt.ID] = onlinescripts[butt.ID]
				localscripts[butt.ID]["path"] = displayName
				dofile(name)
			end
			local status,err = pcall(get_script, but)
			if not status then
				MANAGER.print(err,255,0,0)
				print(err)
				but.selected = false
			else
				MANAGER.print("Downloaded and started "..but.t.text)
				running[displayName] = true
			end
		end
	end
	MANAGER.hidden = true
	ui_button.localview()
	save_last()
end

function ui_button.pressed(self)
	self.selected = not self.selected
end
function ui_button.delete(self)
	--there is no tpt.confirm() yet
	if tpt.input("Delete File", "Delete "..self.t.text.."?", "yes", "no") == "yes" then
		local filepath = self.ID and localscripts[self.ID]["path"] or self.t.text
		fs.removeFile(TPT_LUA_PATH.."/"..filepath:gsub("\\","/"))
		if running[filepath] then running[filepath] = nil end
		if localscripts[self.ID] then localscripts[self.ID] = nil end
		save_last()
		ui_button.localview()
		load_filenames()
		gen_buttons()
	end
end
function ui_button.viewonline(self)
	local command = OS == "Windows" and "start" or (OS == "Linux" and "xdg-open" or "open")
	os.execute(command.." http://starcatcher.us/scripts/#"..self.ID)
end
function ui_button.scriptcheck(self)
	local oldpath = localscripts[self.ID]["path"]
	local newpath = "downloaded"..PATH_SEP..self.ID.." "..onlinescripts[self.ID].author.."-"..onlinescripts[self.ID].name..".lua"
	if download_script(self.ID,TPT_LUA_PATH..PATH_SEP..newpath) then
		self.canupdate = false
		localscripts[self.ID] = onlinescripts[self.ID]
		localscripts[self.ID]["path"] = newpath
		if oldpath ~= newpath then
			fs.removeFile(TPT_LUA_PATH.."/"..oldpath:gsub("\\","/"))
			if running[oldpath] then
				running[newpath],running[oldpath] = running[oldpath],nil
			end
		end
		if running[newpath] then
			do_restart()
		else
			MANAGER.print("Updated "..onlinescripts[self.ID]["name"])
		end
	end
end
function ui_button.doupdate(self)
	if jacobsmod and jacobsmod >= 30 then
		fileSystem.move("scriptmanager.lua", "scriptmanagerold.lua")
		download_script(1, 'scriptmanager.lua')
	else
		fileSystem.move("autorun.lua", "autorunold.lua")
		download_script(1, 'autorun.lua')
	end
	localscripts[1] = updatetable[1]
	do_restart()
end
local uploadscriptbutton
function ui_button.localview(self)
	if online then
		online = false
		gen_buttons()
		donebutton.t.text = "DONE"
		donebutton.w = 29 donebutton.x2 = donebutton.x + donebutton.w
		donebutton.f = ui_button.donepressed
		uploadscriptbutton.t.text = "\147 Script Folder"
	end
end
function ui_button.onlineview(self)
	if not online then
		online = true
		gen_buttons()
		donebutton.t.text = "DOWNLOAD"
		donebutton.w = 55 donebutton.x2 = donebutton.x + donebutton.w
		donebutton.f = ui_button.downloadpressed
		uploadscriptbutton.t.text = "Upload Script"
	end
end
--add buttons to window
donebutton = ui_button.new(55,339,29,10,ui_button.donepressed,"DONE")
mainwindow:add(donebutton)
mainwindow:add(ui_button.new(134,339,40,10,ui_button.sidepressed,"CANCEL"))
--mainwindow:add(ui_button.new(152,339,29,10,ui_button.selectnone,"NONE"))
local nonebutton = ui_button.new(62,81,8,8,ui_button.selectnone,"")
nonebutton.drawbox = true
mainwindow:add(nonebutton)
mainwindow:add(ui_button.new(538,339,33,10,ui_button.consoleclear,"CLEAR"))
mainwindow:add(ui_button.new(278,67,39,10,ui_button.reloadpressed,"RELOAD"))
mainwindow:add(ui_button.new(333,67,79,10,ui_button.changeexename,"Change exe name"))
mainwindow:add(ui_button.new(428,67,51,10,ui_button.changedir,"Change dir"))
uploadscriptbutton = ui_button.new(493,67,79,10,ui_button.uploadscript,"\147 Script Folder")
mainwindow:add(uploadscriptbutton)
local tempbutton = ui_button.new(60, 65, 30, 10, ui_button.localview, "Local")
tempbutton.drawbox = true
mainwindow:add(tempbutton)
tempbutton = ui_button.new(100, 65, 35, 10, ui_button.onlineview, "Online")
tempbutton.drawbox = true
mainwindow:add(tempbutton)
sidebutton = ui_button.new(613,134,14,15,ui_button.sidepressed,'')
if jacobsmod and tpt.oldmenu()==1 then
	sidebutton:onmove(0, 256)
end

local function gen_buttons_local()
	local count = 0
	local sorted = {}
	for k,v in pairs(localscripts) do if v.ID ~= 1 then table.insert(sorted, v) end end
	table.sort(sorted, function(first,second) return first.name:lower() < second.name:lower() end)
	for i,v in ipairs(sorted) do
		local check = mainwindow.checkbox:add(ui_button.pressed,ui_button.delete,v.name,true)
		check.ID = v.ID
		if running[v.path] then
			check.running = true
			check.selected = true
		end
		count = count + 1
	end
	if #sorted >= 5 and #filenames >= 5 then
		mainwindow.checkbox:add(nil, nil, "", false) --empty space to separate things
	end
	for i=1,#filenames do
		local check = mainwindow.checkbox:add(ui_button.pressed,ui_button.delete,filenames[i],true)
		if running[filenames[i]] then
			check.running = true
			check.selected = true
		end
	end
	num_files = count + #filenames
end
local function gen_buttons_online()
	local list = download_file("http://starcatcher.us/scripts/main.lua")
	onlinescripts = readScriptInfo(list)
	local sorted = {}
	for k,v in pairs(onlinescripts) do table.insert(sorted, v) end
	table.sort(sorted, function(first,second) return first.ID < second.ID end)
	for k,v in pairs(sorted) do
		local check = mainwindow.checkbox:add(ui_button.pressed, ui_button.viewonline, v.name, false)
		check.ID = v.ID
		check.checkbut.ID = v.ID
		if localscripts[v.ID] then
			check.running = true
			if tonumber(v.version) > tonumber(localscripts[check.ID].version) then
				check.checkbut.canupdate = true
			end
		end
	end
	if first_online then
		first_online = false
		local updateinfo = download_file("http://starcatcher.us/scripts/main.lua?info=1")
		updatetable = readScriptInfo(updateinfo)
		if not updatetable[1] then return end
		if tonumber(updatetable[1].version) > scriptversion then
			local updatebutton = ui_button.new(278,127,40,10,ui_button.doupdate,"UPDATE")
			updatebutton.t:setcolour(25,255,25)
			mainwindow:add(updatebutton)
			MANAGER.print("A script manager update is available! Click UPDATE",25,255,55)
			MANAGER.print(updatetable[1].changelog,25,255,55)
		end
	end
end
gen_buttons = function()
	mainwindow.checkbox:clear()
	if online then
		gen_buttons_online()
	else
		gen_buttons_local()
	end
	mainwindow.checkbox:updatescroll()
end
gen_buttons()

--register manager first
tpt.register_step(smallstep)
--load previously running scripts
local started = ""
for prev,v in pairs(running) do
	local status,err = pcall(dofile,TPT_LUA_PATH..PATH_SEP..prev)
	if not status then
		MANAGER.print(err,255,0,0)
		running[prev] = nil
	else
		started=started.." "..prev
		local newbut = mainwindow.checkbox:add(ui_button.pressed,prev,nil,false)
		newbut.selected=true
	end
end
save_last()
if started~="" then
	MANAGER.print("Auto started"..started)
end
tpt.register_mouseevent(mouseclick)
tpt.register_keypress(keypress)
