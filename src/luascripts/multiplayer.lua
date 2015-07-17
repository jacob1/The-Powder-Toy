--Cracker64's Powder Toy Multiplayer
--I highly recommend to use my Autorun Script Manager

local versionstring = "0.83.1"

--TODO's
--FIGH,STKM,STK2,LIGH need a few more creation adjustments
--Some more server functions
-------------------------------------------------------

--CHANGES:
--Lots of Fixes
--More colours!
--ESC key will unfocus, then minimize chat
--Changes from jacob, including: Support jacobsMod, keyrepeat
--Support replace mode

if TPTMP then if TPTMP.version <= 2 then TPTMP.disableMultiplayer() else error("newer version already running") end end local get_name = tpt.get_name -- if script already running, replace it
TPTMP = {["version"] = 2} -- script version sent on connect to ensure server protocol is the same
local issocket,socket = pcall(require,"socket")
if not tpt.selectedreplace then error"Tpt version not supported" end
local using_manager = false
local _print = print
if MANAGER ~= nil or MANAGER_EXISTS then
	using_manager = true
	_print = MANAGER and MANAGER.print or MANAGER_PRINT
else
	_print = print
end
local hooks_enabled = false --hooks only enabled once you maximize the button

local PORT = 34403 --Change 34403 to your desired port
local KEYBOARD = 1 --only change if you have issues. Only other option right now is 2(finnish).
--Local player vars we need to keep
local L = {mousex=0, mousey=0, brushx=0, brushy=0, sell=1, sela=296, selr=0, selrep=0, replacemode = 0, mButt=0, mEvent=0, dcolour=0, stick2=false, chatHidden=true, flashChat=false,
shift=false, alt=false, ctrl=false, tabs = false, z=false, downInside=nil, skipClick=false, pauseNextFrame=false, copying=false, stamp=false, placeStamp=false, lastStamp=nil, lastCopy=nil, smoved=false, rotate=false, sendScreen=false}

local tptversion = tpt.version.build
local jacobsmod = tpt.version.jacob1s_mod~=nil
math.randomseed(os.time())
local username = get_name()
if username == "" then
	username = "Guest"..math.random(10000,99999)
end
local chatwindow
local con = {connected = false,
		 socket = nil,
		 members = nil,
		 pingTime = os.time()+60}
local function disconnected(reason)
	if con.socket then
		con.socket:close()
	end
	if reason then
		chatwindow:addline(reason,255,50,50)
	else
		chatwindow:addline("Connection was closed",255,50,50)
	end
	con.connected = false
	con.members = {}
end
local function conSend(cmd,msg,endNull)
	if not con.connected then return false,"Not connected" end
	msg = msg or ""
	if endNull then msg = msg.."\0" end
	if cmd then msg = string.char(cmd)..msg end
	--print("sent "..msg)
	con.socket:settimeout(10)
	con.socket:send(msg)
	con.socket:settimeout(0)
end
local function joinChannel(chan)
	conSend(16,chan,true)
	--send some things to new channel
	conSend(34,string.char(L.brushx,L.brushy))
	conSend(37,string.char(math.floor(L.sell/256),L.sell%256))
	conSend(37,string.char(math.floor(64 + L.sela/256),L.sela%256))
	conSend(37,string.char(math.floor(128 + L.selr/256),L.selr%256))
	conSend(37,string.char(math.floor(192 + L.selrep/256),L.selrep%256))
	conSend(38,L.replacemode)
	conSend(65,string.char(math.floor(L.dcolour/16777216),math.floor(L.dcolour/65536)%256,math.floor(L.dcolour/256)%256,L.dcolour%256))
end
local function connectToServer(ip,port,nick)
	if con.connected then return false,"Already connected" end
	ip = ip or "starcatcher.us"
	port = port or PORT
	local sock = socket.tcp()
	sock:settimeout(10)
	local s,r = sock:connect(ip,port)
	if not s then return false,r end
	sock:settimeout(0)
	sock:setoption("keepalive",true)
	sock:send(string.char(tpt.version.major)..string.char(tpt.version.minor)..string.char(TPTMP.version)..nick.."\0")
	local c,r
	while not c do
	c,r = sock:receive(1)
	if not c and r~="timeout" then break end
	end
	if not c and r~="timeout" then return false,r end

	if c~= "\1" then
	if c=="\0" then
		local err=""
		c,r = sock:receive(1)
		while c~="\0" do
		err = err..c
		c,r = sock:receive(1)
		end
		if err=="This nick is already on the server" then
			nick = nick:gsub("(.)$",function(s) local n=tonumber(s) if n and n+1 <= 9 then return n+1 else return nick:sub(-1)..'0' end end)
			return connectToServer(ip,port,nick)
		end
		return false,err
	end
	return false,"Bad Connect"
	end

	con.socket = sock
	con.connected = true
	username = nick
	conSend(34,string.char(L.brushx,L.brushy))
	conSend(37,string.char(math.floor(L.sell/256),L.sell%256))
	conSend(37,string.char(math.floor(64 + L.sela/256),L.sela%256))
	conSend(37,string.char(math.floor(128 + L.selr/256),L.selr%256))
	conSend(37,string.char(math.floor(192 + L.selrep/256),L.selrep%256))
	conSend(38,L.replacemode)
	conSend(65,string.char(math.floor(L.dcolour/16777216),math.floor(L.dcolour/65536)%256,math.floor(L.dcolour/256)%256,L.dcolour%256))
	return true
end
--get up to a null (\0)
local function conGetNull()
	con.socket:settimeout(nil)
	local c,r = con.socket:receive(1)
	if not c and r ~= "timeout" then disconnected() return nil end
	local rstring=""
	while c~="\0" do
	rstring = rstring..c
	c,r = con.socket:receive(1)
	if not c and r ~= "timeout" then disconnected() return nil end
	end
	con.socket:settimeout(0)
	return rstring
end
--get next char/byte
local function cChar()
	con.socket:settimeout(nil)
	local c,r = con.socket:receive(1)
	con.socket:settimeout(0)
	if not c then disconnected() end
	return c
end
local function cByte()
	local byte = cChar()
	return byte and byte:byte() or nil
end
--return table of arguments
local function getArgs(msg)
	if not msg then return {} end
	local args = {}
	for word in msg:gmatch("([^%s%c]+)") do
	table.insert(args,word)
	end
	return args
end

--get different lists for other language keyboards
local keyboardshift = { {before=" qwertyuiopasdfghjklzxcvbnm1234567890-=.,/`|;'[]\\",after=" QWERTYUIOPASDFGHJKLZXCVBNM!@#$%^&*()_+><?~\\:\"{}|",},{before=" qwertyuiopasdfghjklzxcvbnm1234567890+,.-'߿߿߿߿߿߿߿<",after=" QWERTYUIOPASDFGHJKLZXCVBNM!\"#߿߿߿ߥ&/()=?;:_*`^>",}  }
local keyboardaltrg = { {nil},{before=" qwertyuiopasdfghjklzxcvbnm1234567890+,.-'߿߿߿߼",after=" qwertyuiopasdfghjklzxcvbnm1@߿߿߿ߤ߶{[]}\\,.-'~|",},}

local function shift(s)
	if keyboardshift[KEYBOARD]~=nil then
		return (s:gsub("(.)",function(c)return keyboardshift[KEYBOARD]["after"]:sub(keyboardshift[KEYBOARD]["before"]:find(c,1,true))end))
	else return s end
end
local function altgr(s)
	if keyboardaltgr[KEYBOARD]~=nil then
		return (s:gsub("(.)",function(c)return keyboardaltgr[KEYBOARD]["after"]:sub(keyboardaltgr[KEYBOARD]["before"]:find(c,1,true))end))
	else return s end
end

local ui_base local ui_box local ui_text local ui_button local ui_scrollbar local ui_inputbox local ui_chatbox
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
	box:drawadd(function(self) if self.drawbackground then tpt.fillrect(self.x,self.y,self.w,self.h,self.br,self.bg,self.bb,self.ba) end
								if self.drawbox then tpt.drawrect(self.x,self.y,self.w,self.h,self.r,self.g,self.b) end end)
	box:moveadd(function(self,x,y)
		if x then self.x=self.x+x self.x2=self.x2+x end
		if y then self.y=self.y+y self.y2=self.y2+y end
	end)
	return box
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
newscroll = function(text,x,y,vis,force,r,g,b)
	local txt = ui_text.new(text,x,y,r,g,b)
	if not force and tpt.textwidth(text)<vis then return txt end
	txt.visible=vis
	txt.length=string.len(text)
	txt.start=1
	local last=2
	while tpt.textwidth(text:sub(1,last))<vis and last<=txt.length do
		last=last+1
	end
	txt.last=last-1
	txt.minlast=last-1
	txt.ppl=((txt.visible-6)/(txt.length-txt.minlast+1))
	function txt:update(text,pos)
		if text then
			self.text=text
			self.length=string.len(text)
			local last=2
			while tpt.textwidth(text:sub(1,last))<self.visible and last<=self.length do
				last=last+1
			end
			self.minlast=last-1
			self.ppl=((self.visible-6)/(self.length-self.minlast+1))
			if not pos then self.last=self.minlast end
		end
		if pos then
			if pos>=self.last and pos<=self.length then --more than current visible
				local newlast = pos
				local newstart=1
				while tpt.textwidth(self.text:sub(newstart,newlast))>= self.visible do
					newstart=newstart+1
				end
				self.start=newstart self.last=newlast
			elseif pos<self.start and pos>0 then --position less than current visible
				local newstart=pos
				local newlast=pos+1
				while tpt.textwidth(self.text:sub(newstart,newlast))<self.visible and newlast<self.length do
						newlast=newlast+1
				end
				self.start=newstart self.last=newlast-1
			end
			--keep strings as long as possible (pulls from left)
			local newlast=self.last
			if newlast<self.minlast then newlast=self.minlast end
			local newstart=1
			while tpt.textwidth(self.text:sub(newstart,newlast))>= self.visible do
					newstart=newstart+1
			end
			self.start=newstart self.last=newlast
		end
	end
	txt.drawlist={} --reset draw
	txt:drawadd(function(self,x,y)
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
			end
		end
	end
	return txt
end
}
ui_inputbox = {
new=function(x,y,w,h)
	local intext=ui_box.new(x,y,w,h)
	intext.cursor=0
	intext.line=1
	intext.currentline = ""
	intext.focus=false
	intext.t=ui_text.newscroll("",x+2,y+2,w-2,true)
	intext.history={}
	intext.max_history=200
	intext:drawadd(function(self)
		local cursoradjust=tpt.textwidth(self.t.text:sub(self.t.start,self.cursor))+2
		tpt.drawline(self.x+cursoradjust,self.y,self.x+cursoradjust,self.y+10,255,255,255)
		self.t:draw()
	end)
	intext:moveadd(function(self,x,y) self.t:onmove(x,y) end)
	function intext:setfocus(focus)
		self.focus=focus
		if focus then tpt.set_shortcuts(0) self:setcolour(255,255,0)
		else tpt.set_shortcuts(1) self:setcolour(255,255,255) end
	end
	function intext:movecursor(amt)
		self.cursor = self.cursor+amt
		if self.cursor>self.t.length then self.cursor = self.t.length end
		if self.cursor<0 then self.cursor = 0 return end
	end
	function intext:addhistory(str)
		self.history[#self.history+1] = str
		if #self.history >= self.max_history then
			table.remove(self.history, 1)
		end
	end
	function intext:moveline(amt)
		self.line = self.line+amt
		local max = #self.currentline and #self.history+2 or #self.history+1
		if self.line>max then self.line=max
		elseif self.line<1 then self.line=1 end
		local history = self.history[self.line] or ""
		if self.line == #self.history+1 then history = self.currentline end
		self.cursor = string.len(history)
		self.t:update(history, self.cursor)
	end
	function intext:textprocess(key,nkey,modifier,event)
		if event~=1 then return end
		if not self.focus then
			if nkey==13 then self:setfocus(true) return true end
			return
		end
		if nkey==27 then self:setfocus(false) return true end
		if nkey==13 then local text=self.t.text if text == "" then self:setfocus(false) return true else self.cursor=0 self.t.text="" self:addhistory(text) self.line=#self.history+1 self.currentline = "" return text end end --enter
		if nkey==273 then self:moveline(-1) return true end --up
		if nkey==274 then self:moveline(1) return true end --down
		if nkey==275 then self:movecursor(1) self.t:update(nil,self.cursor) return true end --right
		if nkey==276 then self:movecursor(-1) self.t:update(nil,self.cursor) return true end --left
		local modi = (modifier%1024)
		local newstr
		if nkey==8 and self.cursor > 0 then newstr=self.t.text:sub(1,self.cursor-1) .. self.t.text:sub(self.cursor+1) self:movecursor(-1) --back
		elseif nkey==127 then newstr=self.t.text:sub(1,self.cursor) .. self.t.text:sub(self.cursor+2) --delete
		elseif nkey==9 then --tab complete
			local nickstart,nickend,nick = self.t.text:sub(1,self.cursor+1):find("([^%s%c]+)"..(self.cursor == #self.t.text and "" or " ").."$")
			if con.members and nick then
				for k,v in pairs(con.members) do
					if v.name:sub(1,#nick) == nick then
						nick = v.name if nickstart == 1 then nick = nick..":" end newstr = self.t.text:sub(1,nickstart-1)..nick.." "..self.t.text:sub(nickend+1,#self.t.text) self.cursor = nickstart+#nick
					end
				end
			end
		else
			if nkey<32 or nkey>=127 then return true end --normal key
			local shiftkey = (modi==1 or modi==2)
			if math.floor((modifier%16384)/8192)==1 and key >= 'a' and key <= 'z' then shiftkey = not shiftkey end
			local addkey = shiftkey and shift(key) or key
			if (math.floor(modi/512))==1 then addkey=altgr(key) end
			newstr = self.t.text:sub(1,self.cursor) .. addkey .. self.t.text:sub(self.cursor+1)
			self.currentline = newstr
			self.t:update(newstr,self.cursor+1)
			self:movecursor(1)
			return true
		end
		if newstr then
			self.t:update(newstr,self.cursor)
			return true
		end
		--some actual text processing, lol
	end
	return intext
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
		if wheel~=0 and not hidden_mode then
			if self.total > self.numshown then
				local previous = self.pos
				self:move(wheel)
				if self.pos~=previous then
					return wheel
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
	b.almostselected=false
	b.invert=true
	b:drawadd(function(self)
		if self.invert and self.almostselected then
			self.almostselected=false
			tpt.fillrect(self.x,self.y,self.w,self.h)
			local tr=self.t.r local tg=self.t.g local tb=self.t.b
			b.t:setcolour(0,0,0)
			b.t:draw()
			b.t:setcolour(tr,tg,tb)
		else
			b.t:draw()
		end
	end)
	b:moveadd(function(self,x,y)
		self.t:onmove(x,y)
	end)
	function b:process(mx,my,button,event,wheel)
		if mx<self.x or mx>self.x2 or my<self.y or my>self.y2 then return false end
		if event==3 then self.almostselected=true end
		if event==2 then self:f() end
		return true
	end
	return b
end
}
ui_chatbox = {
new=function(x,y,w,h)
	local chat=ui_box.new(x,y,w,h)
	chat.moving=false
	chat.lastx=0
	chat.lasty=0
	chat.relx=0
	chat.rely=0
	chat.shown_lines=math.floor(chat.h/10)-2 --one line for top, one for chat
	chat.max_width=chat.w-4
	chat.max_lines=200
	chat.lines = {}
	chat.scrollbar = ui_scrollbar.new(chat.x2-2,chat.y+11,chat.h-22,0,chat.shown_lines)
	chat.inputbox = ui_inputbox.new(x,chat.y2-10,w,10)
	chat.minimize = ui_button.new(chat.x2-15,chat.y,15,10,function() chat.moving=false chat.inputbox:setfocus(false) L.chatHidden=true TPTMP.chatHidden=true end,">>")
	chat:drawadd(function(self)
		if self.w > 175 and jacobsmod then
			tpt.drawtext(self.x+self.w/2-tpt.textwidth("TPT Multiplayer, by cracker64")/2,self.y+2,"TPT Multiplayer, by cracker64")
		elseif self.w > 100 then
			tpt.drawtext(self.x+self.w/2-tpt.textwidth("TPT Multiplayer")/2,self.y+2,"TPT Multiplayer")
		end
		tpt.drawline(self.x+1,self.y+10,self.x2-1,self.y+10,120,120,120)
		self.scrollbar:draw()
		local count=0
		for i,line in ipairs(self.lines) do
			if i>self.scrollbar.pos and i<= self.scrollbar.pos+self.shown_lines then
				line:draw(self.x+3,self.y+12+(count*10))
				count = count+1
			end
		end
		self.inputbox:draw()
		self.minimize:draw()
	end)
	chat:moveadd(function(self,x,y)
		for i,line in ipairs(self.lines) do
			line:onmove(x,y)
		end
		self.scrollbar:onmove(x,y)
		self.inputbox:onmove(x,y)
		self.minimize:onmove(x,y)
	end)
	function chat:addline(line,r,g,b,noflash)
		if not line or line=="" then return end --No blank lines
		local linebreak,lastspace = 0,nil
		for i=0,#line do
			local width = tpt.textwidth(line:sub(linebreak,i+1))
			if width > self.max_width/2 and line:sub(i,i):match("[%s,_%.%-?!]") then
				lastspace = i
			end
			if width > self.max_width or i==#line then
			local pos = (i==#line or not lastspace) and i or lastspace
				table.insert(self.lines,ui_text.new(line:sub(linebreak,pos),self.x,0,r,g,b))
				linebreak = pos+1
				lastspace = nil
			end
		end
		while #self.lines>self.max_lines do table.remove(self.lines,1) end
		self.scrollbar:update(#self.lines,self.shown_lines,#self.lines-self.shown_lines)
		if L.chatHidden and not noflash then L.flashChat=true end
	end
	chat:addline("TPTMP v"..versionstring..": Type '/connect' to join server, or /list for a list of commands.",200,200,200,true)
	function chat:process(mx,my,button,event,wheel)
		if L.chatHidden then return false end
		self.minimize:process(mx,my,button,event,wheel)
		if self.moving and event==3 then
			local newx,newy = mx-self.relx,my-self.rely
			local ax,ay = 0,0
			if newx<0 then ax = newx end
			if newy<0 then ay = newy end
			if (newx+self.w)>=sim.XRES then ax = newx+self.w-sim.XRES end
			if (newy+self.h)>=sim.YRES then ay = newy+self.h-sim.YRES end
			self:onmove(mx-self.lastx-ax,my-self.lasty-ay)
			self.lastx=mx-ax
			self.lasty=my-ay
			return true
		end
		local which = math.floor((my-self.y)/10)
		if self.moving and event==2 then self.moving=false return true end
		if mx<self.x or mx>self.x2 or my<self.y or my>self.y2 then if button == 0 then return false end self.inputbox:setfocus(false) return false elseif event==1 and which ~= 0 and not self.inputbox.focus then self.inputbox:setfocus(true) end
		self.scrollbar:process(mx,my,button,event,wheel)
		if event==1 and which==0 then self.moving=true self.lastx=mx self.lasty=my self.relx=mx-self.x self.rely=my-self.y return true end
		if event==1 and which==self.shown_lines+1 then self.inputbox:setfocus(true) return true elseif self.inputbox.focus then return true end --trigger input_box
		if which>0 and which<self.shown_lines+1 and self.lines[which+self.scrollbar.pos] then self.lines[which+self.scrollbar.pos]:process(mx,my,button,event,wheel) end
		return event==1
	end
	--commands for chat window
	chatcommands = {
	connect = function(self,msg,args)
		if not issocket then self:addline("No luasockets found") return end
		local newname = pcall(string.dump, get_name) and "Gue".."st"..math["random"](1111,9888) or get_name()
		local s,r = connectToServer(args[1],tonumber(args[2]), newname~="" and newname or username)
		if not s then self:addline(r,255,50,50) end
		pressedKeys = nil
	end,
	send = function(self,msg,args)
		if tonumber(args[1]) and args[2] then
		local withNull=false
		if args[2]=="true" then withNull=true end
		msg = msg:sub(#args[1]+1+(withNull and #args[2]+2 or 0))
		conSend(tonumber(args[1]),msg,withNull)
		end
	end,
	quit = function(self,msg,args)
		disconnected("Disconnected")
	end,
	disconnect = function(self,msg,args)
		disconnected("Disconnected")
	end,
	join = function(self,msg,args)
		if args[1] then
			joinChannel(args[1])
			self:addline("joined channel "..args[1],50,255,50)
		end
	end,
	sync = function(self,msg,args)
		if con.connected then L.sendScreen=true end --need to send 67 clear screen
		self:addline("Synced screen to server",255,255,50)
	end,
	help = function(self,msg,args)
		if not args[1] then self:addline("/help <command>, type /list for a list of commands") end
		if args[1] == "connect" then self:addline("(/connect [ip] [port]) -- connect to a TPT multiplayer server, or no args to connect to the default one")
		--elseif args[1] == "send" then self:addline("(/send <something> <somethingelse>) -- send raw data to the server") -- send a raw command
		elseif args[1] == "quit" or args[1] == "disconnect" then self:addline("(/quit, no arguments) -- quit the game")
		elseif args[1] == "join" then self:addline("(/join <channel> -- joins a room on the server")
		elseif args[1] == "sync" then self:addline("(/sync, no arguments) -- syncs your screen to everyone else in the room")
		elseif args[1] == "me" then self:addline("(/me <message>) -- say something in 3rd person") -- send a raw command
		elseif args[1] == "kick" then self:addline("(/kick <nick> <reason>) -- kick a user, only works if you have been in a channel the longest")
		elseif args[1] == "size" then self:addline("(/size <width> <height>) -- sets the size of the chat window")
		end
	end,
	list = function(self,msg,args)
		local list = ""
		for name in pairs(chatcommands) do
			list=list..name..", "
		end
		self:addline("Commands: "..list:sub(1,#list-2))
	end,
	me = function(self, msg, args)
		if not con.connected then return end
		self:addline("* " .. username .. " ".. table.concat(args, " "),200,200,200)
		conSend(20,table.concat(args, " "),true)
	end,
	kick = function(self, msg, args)
		if not con.connected then return end
		if not args[1] then self:addline("Need a nick! '/kick <nick> [reason]'") return end
		conSend(21, args[1].."\0"..table.concat(args, " ", 2),true)
	end,
	size = function(self, msg, args)
		if args[2] then
			local w, h = tonumber(args[1]), tonumber(args[2])
			if w < 75 or h < 50 then self:addline("size too small") return
			elseif w > sim.XRES-100 or h > sim.YRES-100 then self:addline("size too large") return
			end
			chatwindow = ui_chatbox.new(100,100,w,h)
			chatwindow:setbackground(10,10,10,235) chatwindow.drawbackground=true
			if using_manager then
				MANAGER.savesetting("tptmp", "width", w)
				MANAGER.savesetting("tptmp", "height", h)
			end
		end
	end
	}
	function chat:textprocess(key,nkey,modifier,event)
		if L.chatHidden then return nil end
		local text = self.inputbox:textprocess(key,nkey,modifier,event)
		if type(text)=="boolean" then return text end
		if text and text~="" then

			local cmd = text:match("^/([^%s]+)")
			if cmd then
				local msg=text:sub(#cmd+3)
				local args = getArgs(msg)
				if chatcommands[cmd] then
					chatcommands[cmd](self,msg,args)
					--self:addline("Executed "..cmd.." "..rest)
					return
				end
			end
			--normal chat
			if con.connected then
				conSend(19,text,true)
				self:addline(username .. ": ".. text,200,200,200)
			else
				self:addline("Not connected to server!",255,50,50)
			end
		end
	end
	return chat
end
}
local fadeText = {}
--A little text that fades away, (align text (left/center/right)?)
local function newFadeText(text,frames,x,y,r,g,b,noremove)
	local t = {ticks=frames,max=frames,text=text,x=x,y=y,r=r,g=g,b=b,keep=noremove}
	t.reset = function(self,text) self.ticks=self.max if text then self.text=text end end
	table.insert(fadeText,t)
	return t
end
--Some text locations for repeated usage
local infoText = newFadeText("",150,245,370,255,255,255,true)
local cmodeText = newFadeText("",120,250,180,255,255,255,true)

local showbutton = ui_button.new(613,using_manager and 119 or 136,14,14,function() if using_manager and not MANAGER.hidden then _print("minimize the manager before opening TPTMP") return end if not hooks_enabled then TPTMP.enableMultiplayer() end L.chatHidden=false TPTMP.chatHidden=false L.flashChat=false end,"<<")
if jacobsmod and tpt.oldmenu()~=0 then
	showbutton:onmove(0, 256)
end
local flashCount=0
showbutton.drawbox = true showbutton:drawadd(function(self) if L.flashChat then self.almostselected=true flashCount=flashCount+1 if flashCount%25==0 then self.invert=not self.invert end end end)
if using_manager then
	local loadsettings = function() chatwindow = ui_chatbox.new(100, 100, tonumber(MANAGER.getsetting("tptmp", "width")), tonumber(MANAGER.getsetting("tptmp", "height"))) end
	if not pcall(loadsettings) then chatwindow = ui_chatbox.new(100, 100, 225, 150) end
else
	chatwindow = ui_chatbox.new(100, 100, 225, 150)
end
chatwindow:setbackground(10,10,10,235) chatwindow.drawbackground=true

local eleNameTable = {
["DEFAULT_PT_LIFE_GOL"] = 256,["DEFAULT_PT_LIFE_HLIF"] = 257,["DEFAULT_PT_LIFE_ASIM"] = 258,["DEFAULT_PT_LIFE_2x2"] = 259,["DEFAULT_PT_LIFE_DANI"] = 260,
["DEFAULT_PT_LIFE_AMOE"] = 261,["DEFAULT_PT_LIFE_MOVE"] = 262,["DEFAULT_PT_LIFE_PGOL"] = 263,["DEFAULT_PT_LIFE_DMOE"] = 264,["DEFAULT_PT_LIFE_34"] = 265,
["DEFAULT_PT_LIFE_LLIF"] = 276,["DEFAULT_PT_LIFE_STAN"] = 267,["DEFAULT_PT_LIFE_SEED"] = 268,["DEFAULT_PT_LIFE_MAZE"] = 269,["DEFAULT_PT_LIFE_COAG"] = 270,
["DEFAULT_PT_LIFE_WALL"] = 271,["DEFAULT_PT_LIFE_GNAR"] = 272,["DEFAULT_PT_LIFE_REPL"] = 273,["DEFAULT_PT_LIFE_MYST"] = 274,["DEFAULT_PT_LIFE_LOTE"] = 275,
["DEFAULT_PT_LIFE_FRG2"] = 276,["DEFAULT_PT_LIFE_STAR"] = 277,["DEFAULT_PT_LIFE_FROG"] = 278,["DEFAULT_PT_LIFE_BRAN"] = 279,
["DEFAULT_WL_0"] = 280,["DEFAULT_WL_1"] = 281,["DEFAULT_WL_2"] = 282,["DEFAULT_WL_3"] = 283,["DEFAULT_WL_4"] = 284,
["DEFAULT_WL_5"] = 285,["DEFAULT_WL_6"] = 286,["DEFAULT_WL_7"] = 287,["DEFAULT_WL_8"] = 288,["DEFAULT_WL_9"] = 289,["DEFAULT_WL_10"] = 290,
["DEFAULT_WL_11"] = 291,["DEFAULT_WL_12"] = 292,["DEFAULT_WL_13"] = 293,["DEFAULT_WL_14"] = 294,["DEFAULT_WL_15"] = 295,
["DEFAULT_UI_SAMPLE"] = 296,["DEFAULT_UI_SIGN"] = 297,["DEFAULT_UI_PROPERTY"] = 298,["DEFAULT_UI_WIND"] = 299,
["DEFAULT_TOOL_HEAT"] = 300,["DEFAULT_TOOL_COOL"] = 301,["DEFAULT_TOOL_VAC"] = 302,["DEFAULT_TOOL_AIR"] = 303,["DEFAULT_TOOL_PGRV"] = 304,["DEFAULT_TOOL_NGRV"] = 305,
["DEFAULT_DECOR_SET"] = 306,["DEFAULT_DECOR_ADD"] = 307,["DEFAULT_DECOR_SUB"] = 308,["DEFAULT_DECOR_MUL"] = 309,["DEFAULT_DECOR_DIV"] = 310,["DEFAULT_DECOR_SMDG"] = 311,["DEFAULT_DECOR_CLR"] = 312,["DEFAULT_DECOR_LIGH"] = 313, ["DEFAULT_DECOR_DARK"] = 314,
["DEFAULT_WL_16"] = 315
}
local function convertDecoTool(c)
	return c
end
if jacobsmod then
	function convertDecoTool(c)
		if c >= 307 and c <= 311 then
			c = c + 1
		elseif c == 312 then
			c = 307
		end
		return c
	end
	local modNameTable = {
	["DEFAULT_WL_ERASE"] = 280,["DEFAULT_WL_CNDTW"] = 281,["DEFAULT_WL_EWALL"] = 282,["DEFAULT_WL_DTECT"] = 283,["DEFAULT_WL_STRM"] = 284,
	["DEFAULT_WL_FAN"] = 285,["DEFAULT_WL_LIQD"] = 286,["DEFAULT_WL_ABSRB"] = 287,["DEFAULT_WL_WALL"] = 288,["DEFAULT_WL_AIR"] = 289,["DEFAULT_WL_POWDR"] = 290,
	["DEFAULT_WL_CNDTR"] = 291,["DEFAULT_WL_EHOLE"] = 292,["DEFAULT_WL_GAS"] = 293,["DEFAULT_WL_GRVTY"] = 294,["DEFAULT_WL_ENRGY"] = 295,["DEFAULT_WL_ERASEA"] = 280,
	["DEFAULT_WL_NOAIR"] = 315
	}
	for k,v in pairs(modNameTable) do
		eleNameTable[k] = v
	end
end
local gravList= {[0]="Vertical",[1]="Off",[2]="Radial"}
local airList= {[0]="On",[1]="Pressure Off",[2]="Velocity Off",[3]="Off",[4]="No Update"}
local noFlood = {[15]=true,[55]=true,[87]=true,[128]=true,[158]=true}
local noShape = {[55]=true,[87]=true,[128]=true,[158]=true}
local createOverride = {[55]=function(x,y,rx,ry,c,brush) sim.createParts(x,y,0,0,c,brush) end ,[87]=function(x,y,rx,ry,c,brush) sim.createParts(x,y,0,0,c,brush) end,[128]=function(x,y,rx,ry,c,brush) sim.createParts(x,y,0,0,c,brush) end,[158]=function(x,y,rx,ry,c,brush) sim.createParts(x,y,0,0,c,brush) end}
local golStart,golEnd=256,279
local wallStart,wallEnd=280,295
local toolStart,toolEnd=300,305
local decoStart,decoEnd=306,314

--Functions that do stuff in powdertoy
local function createBoxAny(x1,y1,x2,y2,c,user)
	if noShape[c] then return end
	if c>=wallStart then
		if c<= wallEnd then
			sim.createWallBox(x1,y1,x2,y2,c-wallStart)
		elseif c<=toolEnd then
			if c>=toolStart then sim.toolBox(x1,y1,x2,y2,c-toolStart) end
		elseif c<= decoEnd then
			sim.decoBox(x1,y1,x2,y2,user.dcolour[2],user.dcolour[3],user.dcolour[4],user.dcolour[1],convertDecoTool(c)-decoStart)
		end
		return
	elseif c>=golStart then
		c = 78+(c-golStart)*256
	end
	sim.createBox(x1,y1,x2,y2,c,user and user.replacemode)
end
local function createPartsAny(x,y,rx,ry,c,brush,user)
	if c>=wallStart then
		if c<= wallEnd then
			if c == 284 then rx,ry = 0,0 end
			sim.createWalls(x,y,rx,ry,c-wallStart,brush)
		elseif c<=toolEnd then
			if c>=toolStart then sim.toolBrush(x,y,rx,ry,c-toolStart,brush) end
		elseif c<= decoEnd then
			sim.decoBrush(x,y,rx,ry,user.dcolour[2],user.dcolour[3],user.dcolour[4],user.dcolour[1],convertDecoTool(c)-decoStart,brush)
		end
		return
	elseif c>=golStart then
		c = 78+(c-golStart)*256
	end
	if createOverride[c] then createOverride[c](x,y,rx,ry,c,brush) return end
	sim.createParts(x,y,rx,ry,c,brush,user.replacemode)
end
local function createLineAny(x1,y1,x2,y2,rx,ry,c,brush,user)
	if noShape[c] then return end
	if jacobsmod and c == tpt.element("ball") and not user.shift then return end
	if c>=wallStart then
		if c<= wallEnd then
			if c == 284 then rx,ry = 0,0 end
			sim.createWallLine(x1,y1,x2,y2,rx,ry,c-wallStart,brush)
		elseif c<=toolEnd then
			if c>=toolStart then local str=1.0 if user.drawtype==4 then if user.shift then str=10.0 elseif user.alt then str=0.1 end end sim.toolLine(x1,y1,x2,y2,rx,ry,c-toolStart,brush,str) end
		elseif c<= decoEnd then
			sim.decoLine(x1,y1,x2,y2,rx,ry,user.dcolour[2],user.dcolour[3],user.dcolour[4],user.dcolour[1],convertDecoTool(c)-decoStart,brush)
		end
		return
	elseif c>=golStart then
		c = 78+(c-golStart)*256
	end
	sim.createLine(x1,y1,x2,y2,rx,ry,c,brush,user.replacemode)
end
local function floodAny(x,y,c,cm,bm,user)
	if noFlood[c] then return end
	if c>=wallStart then
		if c<= wallEnd then
			sim.floodWalls(x,y,c-wallStart,bm)
		end
		--other tools shouldn't flood
		return
	elseif c>=golStart then --GoL adjust
		c = 78+(c-golStart)*256
	end
	sim.floodParts(x,y,c,cm,user.replacemode)
end
local function lineSnapCoords(x1,y1,x2,y2)
	local nx,ny
	local snapAngle = math.floor(math.atan2(y2-y1, x2-x1)/(math.pi*0.25)+0.5)*math.pi*0.25;
	local lineMag = math.sqrt(math.pow(x2-x1,2)+math.pow(y2-y1,2));
	nx = math.floor(lineMag*math.cos(snapAngle)+x1+0.5);
	ny = math.floor(lineMag*math.sin(snapAngle)+y1+0.5);
	return nx,ny
end

local function rectSnapCoords(x1,y1,x2,y2)
	local nx,ny
	local snapAngle = math.floor((math.atan2(y2-y1, x2-x1)+math.pi*0.25)/(math.pi*0.5)+0.5)*math.pi*0.5 - math.pi*0.25;
	local lineMag = math.sqrt(math.pow(x2-x1,2)+math.pow(y2-y1,2));
	nx = math.floor(lineMag*math.cos(snapAngle)+x1+0.5);
	ny = math.floor(lineMag*math.sin(snapAngle)+y1+0.5);
	return nx,ny
end
local renModes = {[0xff00f270]=1,[-16715152]=1,[0x0400f381]=2,[0xf382]=4,[0xf388]=8,[0xf384]=16,[0xfff380]=32,[1]=0xff00f270,[2]=0x0400f381,[4]=0xf382,[8]=0xf388,[16]=0xf384,[32]=0xfff380}
local function getViewModes()
	local t={0,0,0}
	for k,v in pairs(ren.displayModes()) do
		t[1] = t[1]+v
	end
	for k,v in pairs(ren.renderModes()) do
		t[2] = t[2]+(renModes[v] or 0)
	end
	t[3] = ren.colourMode()
	return t
end

--clicky click
local function playerMouseClick(id,btn,ev)
	local user = con.members[id]
	local createE, checkBut

	--_print(tostring(btn)..tostring(ev))
	if ev==0 then return end
	if btn==1 then
		user.rbtn,user.abtn = false,false
		createE,checkBut=user.selectedl,user.lbtn
	elseif btn==2 then
		user.rbtn,user.lbtn = false,false
		createE,checkBut=user.selecteda,user.abtn
	elseif btn==4 then
		user.lbtn,user.abtn = false,false
		createE,checkBut=user.selectedr,user.rbtn
	else return end

	if user.mousex>=sim.XRES or user.mousey>=sim.YRES then user.drawtype=false return end

	if ev==1 then
		user.pmx,user.pmy = user.mousex,user.mousey
		if not user.drawtype then
			--left box
			if user.ctrl and not user.shift then user.drawtype = 2 return end
			--left line
			if user.shift and not user.ctrl then user.drawtype = 1 return end
			--floodfill
			if user.ctrl and user.shift then floodAny(user.mousex,user.mousey,createE,-1,-1,user) user.drawtype = 3 return end
			--an alt click
			if user.alt then return end
			user.drawtype=4 --normal hold
		end
		createPartsAny(user.mousex,user.mousey,user.brushx,user.brushy,createE,user.brush,user)
	elseif ev==2 and checkBut and user.drawtype then
		if user.drawtype==2 then
			if user.alt then user.mousex,user.mousey = rectSnapCoords(user.pmx,user.pmy,user.mousex,user.mousey) end
			createBoxAny(user.mousex,user.mousey,user.pmx,user.pmy,createE,user)
		else
			if user.alt then user.mousex,user.mousey = lineSnapCoords(user.pmx,user.pmy,user.mousex,user.mousey) end
			createLineAny(user.mousex,user.mousey,user.pmx,user.pmy,user.brushx,user.brushy,createE,user.brush,user)
		end
		user.drawtype=false
		user.pmx,user.pmy = user.mousex,user.mousey
	end
end
--To draw continued lines
local function playerMouseMove(id)
	local user = con.members[id]
	local createE, checkBut
	if user.lbtn then
		createE,checkBut=user.selectedl,user.lbtn
	elseif user.rbtn then
		createE,checkBut=user.selectedr,user.rbtn
	elseif user.abtn then
		createE,checkBut=user.selecteda,user.abtn
	else return end
	if user.drawtype~=4 then if user.drawtype==3 then floodAny(user.mousex,user.mousey,createE,-1,-1,user) end return end
	if checkBut==3 then
		if user.mousex>=sim.XRES then user.mousex=sim.XRES-1 end
		if user.mousey>=sim.YRES then user.mousey=sim.YRES-1 end
		createLineAny(user.mousex,user.mousey,user.pmx,user.pmy,user.brushx,user.brushy,createE,user.brush,user)
		user.pmx,user.pmy = user.mousex,user.mousey
	end
end
local function loadStamp(size,x,y,reset)
	con.socket:settimeout(10)
	local s = con.socket:receive(size)
	con.socket:settimeout(0)
	if s then
		local f = io.open(".tmp.stm","wb")
		f:write(s)
		f:close()
		if reset then sim.clearSim() end
		if not sim.loadStamp(".tmp.stm",x,y) then
			infoText:reset("Error loading stamp")
		end
		os.remove".tmp.stm"
	else
		infoText:reset("Error loading empty stamp")
	end
end
local function saveStamp(x, y, w, h)
	local stampName = sim.saveStamp(x, y, w, h) or "errorsavingstamp"
	local fullName = "stamps/"..stampName..".stm"
	return stampName, fullName
end
local function deleteStamp(name)
	if sim.deleteStamp then
		sim.deleteStamp(name)
	else
		os.remove("stamps/"..name..".stm")
	end
end

local dataCmds = {
	[16] = function()
	--room members
		con.members = {}
		local amount = cByte()
		local peeps = {}
		for i=1,amount do
			local id = cByte()
			con.members[id]={name=conGetNull(),mousex=0,mousey=0,brushx=4,brushy=4,brush=0,selectedl=1,selectedr=0,selecteda=296,replacemode=0,dcolour={0,0,0,0},lbtn=false,abtn=false,rbtn=false,ctrl=false,shift=false,alt=false}
			local name = con.members[id].name
			table.insert(peeps,name)
		end
		chatwindow:addline("Online: "..table.concat(peeps," "),255,255,50)
	end,
	[17]= function()
		local id = cByte()
		con.members[id] ={name=conGetNull(),mousex=0,mousey=0,brushx=4,brushy=4,brush=0,selectedl=1,selectedr=0,selecteda=296,replacemode=0,dcolour={0,0,0,0},lbtn=false,abtn=false,rbtn=false,ctrl=false,shift=false,alt=false}
		chatwindow:addline(con.members[id].name.." has joined",100,255,100)
	end,
	[18] = function()
		local id = cByte()
		chatwindow:addline(con.members[id].name.." has left",255,255,100)
		con.members[id]=nil
	end,
	[19] = function()
		chatwindow:addline(con.members[cByte()].name .. ": " .. conGetNull())
	end,
	[20] = function()
		chatwindow:addline("* "..con.members[cByte()].name .. " " .. conGetNull())
	end,
	[22] = function()
		chatwindow:addline("[SERVER] "..conGetNull(), cByte(), cByte(), cByte())
	end,
	--Mouse Position
	[32] = function()
		local id = cByte()
		local b1,b2,b3=cByte(),cByte(),cByte()
		con.members[id].mousex,con.members[id].mousey=((b1*16)+math.floor(b2/16)),((b2%16)*256)+b3
		playerMouseMove(id)
	end,
	--Mouse Click
	[33] = function()
		local id = cByte()
		local d=cByte()
		local btn,ev=math.floor(d/16),d%16
		playerMouseClick(id,btn,ev)
		if ev==0 then return end
		if btn==1 then
			con.members[id].lbtn=ev
		elseif btn==2 then
			con.members[id].abtn=ev
		elseif btn==4 then
			con.members[id].rbtn=ev
		end
	end,
	--Brush size
	[34] = function()
		local id = cByte()
		con.members[id].brushx,con.members[id].brushy=cByte(),cByte()
	end,
	--Brush Shape change, no args
	[35] = function()
		local id = cByte()
		con.members[id].brush=(con.members[id].brush+1)%3
	end,
	--Modifier (mod and state)
	[36] = function()
		local id = cByte()
		local d=cByte()
		local mod,state=math.floor(d/16),d%16~=0
		if mod==0 then
			con.members[id].ctrl=state
		elseif mod==1 then
			con.members[id].shift=state
		elseif mod==2 then
			con.members[id].alt=state
		end
	end,
	--selected elements (2 bits button, 14-element)
	[37] = function()
		local id = cByte()
		local b1,b2=cByte(),cByte()
		local btn,el=math.floor(b1/64),(b1%64)*256+b2
		if btn==0 then
			con.members[id].selectedl=el
		elseif btn==1 then
			con.members[id].selecteda=el
		elseif btn==2 then
			con.members[id].selectedr=el
		elseif btn==3 then
			--sync replace mode element between all players since apparently you have to set tpt.selectedreplace to use replace mode ...
			tpt.selectedreplace = elem.property(el, "Identifier")
		end
	end,
	--replace mode / specific delete
	[38] = function()
		local id = cByte()
		local mod = cByte()
		con.members[id].replacemode = mod
	end,
	--cmode defaults (1 byte mode)
	[48] = function()
		local id = cByte()
		tpt.display_mode(cByte())
		cmodeText:reset(con.members[id].name.." set:")
	end,
	--pause set (1 byte state)
	[49] = function()
		local id = cByte()
		local p,str = cByte(),"Pause"
		tpt.set_pause(p)
		if p==0 then str="Unpause" end
		infoText:reset(str.." from "..con.members[id].name)
	end,
	--step frame, no args
	[50] = function()
		local id = cByte()
		tpt.set_pause(0)
		L.pauseNextFrame=true
	end,

	--deco mode, (1 byte state)
	[51] = function()
		local id = cByte()
		tpt.decorations_enable(cByte())
		cmodeText:reset(con.members[id].name.." set:")
	end,
	--[[HUD mode, (1 byte state), deprecated
	[52] = function()
		local id = cByte()
		local hstate = cByte()
		tpt.hud(hstate)
	end,
	--]]
	--amb heat mode, (1 byte state)
	[53] = function()
		local id = cByte()
		tpt.ambient_heat(cByte())
	end,
	--newt_grav mode, (1 byte state)
	[54] = function()
		local id = cByte()
		tpt.newtonian_gravity(cByte())
	end,

	--[[
	--debug mode (1 byte state?) can't implement
	[55] = function()
		local id = cByte()
		--local dstate = cByte()
		tpt.setdebug()
	end,
	--]]
	--legacy heat mode, (1 byte state)
	[56] = function()
		local id = cByte()
		tpt.heat(cByte())
	end,
	--water equal, (1 byte state)
	[57] = function()
		local id = cByte()
		sim.waterEqualisation(cByte())
	end,

	--grav mode, (1 byte state)
	[58] = function()
		local id = cByte()
		local mode = cByte()
		sim.gravityMode(mode)
		cmodeText:reset(con.members[id].name.." set: Gravity: "..gravList[mode])
	end,
	--air mode, (1 byte state)
	[59] = function()
		local id = cByte()
		local mode=cByte()
		sim.airMode(mode)
		cmodeText:reset(con.members[id].name.." set: Air: "..airList[mode])
	end,

	--clear sparks (no args)
	[60] = function()
		local id = cByte()
		tpt.reset_spark()
	end,
	--clear pressure/vel (no args)
	[61] = function()
		local id = cByte()
		tpt.reset_velocity()
		tpt.set_pressure()
	end,
	--invert pressure (no args)
	[62] = function()
		local id = cByte()
		for x=0,152 do
			for y=0,95 do
				sim.pressure(x,y,-sim.pressure(x,y))
			end
		end
	end,
	--Clearsim button (no args)
	[63] = function()
		local id = cByte()
		sim.clearSim()
		L.lastSave=nil
		infoText:reset(con.members[id].name.." cleared the screen")
	end,
	--Full graphics view mode (for manual changes in display menu) (3 bytes)
	[64] = function()
		local id = cByte()
		local disM,renM,colM = cByte(),cByte(),cByte()
		ren.displayModes({disM})
		local t,i={},1
		while i<=32 do
			if bit.band(renM,i)>0 then table.insert(t,renModes[i]) end
			i=i*2
		end
		ren.renderModes(t)
		ren.colourMode(colM)
	end,
	--Selected deco colour (4 bytes)
	[65] = function()
		local id = cByte()
		con.members[id].dcolour = {cByte(),cByte(),cByte(),cByte()}
	end,
	--Recieve a stamp, with location (6 bytes location(3),size(3))
	[66] = function()
		local id = cByte()
		local b1,b2,b3=cByte(),cByte(),cByte()
		local x,y =((b1*16)+math.floor(b2/16)),((b2%16)*256)+b3
		local d = cByte()*65536+cByte()*256+cByte()
		loadStamp(d,x,y,false)
		infoText:reset("Stamp from "..con.members[id].name)
	end,
	--Clear an area, helper for cut (6 bytes, start(3), end(3))
	[67] = function()
		local id = cByte()
		local b1,b2,b3,b4,b5,b6=cByte(),cByte(),cByte(),cByte(),cByte(),cByte()
		local x1,y1 =((b1*16)+math.floor(b2/16)),((b2%16)*256)+b3
		local x2,y2 =((b4*16)+math.floor(b5/16)),((b5%16)*256)+b6
		--clear walls and parts
		createBoxAny(x1,y1,x2,y2,280)
		createBoxAny(x1,y1,x2,y2,0)
	end,
	--Edge mode (1 byte state)
	[68] = function()
		local id = cByte()
		sim.edgeMode(cByte())
	end,
	--Load a save ID (3 bytes ID)
	[69] = function()
		local id = cByte()
		local saveID = cByte()*65536+cByte()*256+cByte()
		L.lastSave=saveID
		sim.loadSave(saveID,1)
		L.browseMode=3
	end,
	--Reload sim(from a stamp right now, no args)
	[70] = function()
		local id = cByte()
		sim.clearSim()
		if not sim.loadStamp("stamps/tmp.stm",0,0) then
			infoText:reset("Error reloading save from "..con.members[id].name)
		end
	end,
	--A request to sync a player, from server, send screen, and various settings
	[128] = function()
		local id = cByte()
		conSend(130,string.char(id,49,tpt.set_pause()))
		local stampName,fullName = saveStamp(0,0,sim.XRES-1,sim.YRES-1)
		local f = assert(io.open(fullName,"rb"))
		local s = f:read"*a"
		f:close()
		deleteStamp(stampName)
		local d = #s
		conSend(128,string.char(id,math.floor(d/65536),math.floor(d/256)%256,d%256)..s)
		conSend(130,string.char(id,53,tpt.ambient_heat()))
		conSend(130,string.char(id,54,tpt.newtonian_gravity()))
		conSend(130,string.char(id,56,tpt.heat()))
		conSend(130,string.char(id,57,sim.waterEqualisation()))
		conSend(130,string.char(id,58,sim.gravityMode()))
		conSend(130,string.char(id,59,sim.airMode()))
		conSend(130,string.char(id,68,sim.edgeMode()))
		conSend(64,string.char(unpack(getViewModes())))
		conSend(34,string.char(tpt.brushx,tpt.brushy))
	end,
	--Recieve sync stamp
	[129] = function()
		local d = cByte()*65536+cByte()*256+cByte()
		loadStamp(d,0,0,true)
	end,
}

local function connectThink()
	if not con.connected then return end
	if not con.socket then disconnected() return end
	--read all messages
	while 1 do
		local s,r = con.socket:receive(1)
		if s then
			local cmd = string.byte(s)
			--_print("GOT "..tostring(cmd))
			if dataCmds[cmd] then dataCmds[cmd]() else _print("TPTMP: Unknown protocol "..tostring(cmd),255,20,20) end
		else
			if r ~= "timeout" then disconnected() end
			break
		end
	end

	--ping every minute
	if os.time()>con.pingTime then conSend(2) con.pingTime=os.time()+60 end
end
--Track if we have STKM2 out, for WASD key changes
elements.property(128,"Update",function() L.stick2=true end)

local function drawStuff()
	if con.members then
		for i,user in pairs(con.members) do
			local x,y = user.mousex,user.mousey
			local brx,bry=user.brushx,user.brushy
			local brush,drawBrush=user.brush,true
			gfx.drawText(x,y,("%s %dx%d"):format(user.name,brx,bry),0,255,0,192)
			if user.drawtype then
				if user.drawtype==1 then
					if user.alt then x,y = lineSnapCoords(user.pmx,user.pmy,x,y) end
					tpt.drawline(user.pmx,user.pmy,x,y,0,255,0,128)
				elseif user.drawtype==2 then
					if user.alt then x,y = rectSnapCoords(user.pmx,user.pmy,x,y) end
					local tpmx,tpmy = user.pmx,user.pmy
					if tpmx>x then tpmx,x=x,tpmx end
					if tpmy>y then tpmy,y=y,tpmy end
					tpt.drawrect(tpmx,tpmy,x-tpmx,y-tpmy,0,255,0,128)
					drawBrush=false
				elseif user.drawtype==3 then
					tpt.drawline(x,y,x+5,y,0,255,0,128)
					tpt.drawline(x,y,x-5,y,0,255,0,128)
					tpt.drawline(x,y,x,y+5,0,255,0,128)
					tpt.drawline(x,y,x,y-5,0,255,0,128)
					drawBrush=false
				end
			end
			if drawBrush then
				if brush==0 then
					gfx.drawCircle(x,y,brx,bry,0,255,0,128)
				elseif brush==1 then
					gfx.drawRect(x-brx,y-bry,brx*2+1,bry*2+1,0,255,0,128)
				elseif brush==2 then
					gfx.drawLine(x-brx,y+bry,x,y-bry,0,255,0,128)
					gfx.drawLine(x-brx,y+bry,x+brx,y+bry,0,255,0,128)
					gfx.drawLine(x,y-bry,x+brx,y+bry,0,255,0,128)
				end
			end
		end
	end
	for k,v in pairs(fadeText) do
		if v.ticks > 0 then
			local a = math.floor(255*(v.ticks/v.max))
			tpt.drawtext(v.x,v.y,v.text,v.r,v.g,v.b,a)
			v.ticks = v.ticks-1
		else if not v.keep then table.remove(fadeText,k) end
		end
	end
end

local function sendStuff()
	if not con.connected then return end
	--mouse position every frame, not exactly needed, might be better/more accurate from clicks
	local nmx,nmy = tpt.mousex,tpt.mousey
	if nmx<sim.XRES and nmy<sim.YRES then nmx,nmy = sim.adjustCoords(nmx,nmy) end
	if L.mousex~= nmx or L.mousey~= nmy then
		L.mousex,L.mousey = nmx,nmy
		local b1,b2,b3 = math.floor(L.mousex/16),((L.mousex%16)*16)+math.floor(L.mousey/256),(L.mousey%256)
		conSend(32,string.char(b1,b2,b3))
	end
	if tpt.brushx > 255 then tpt.brushx = 255 end
	if tpt.brushy > 255 then tpt.brushy = 255 end
	local nbx,nby = tpt.brushx,tpt.brushy
	if L.brushx~=nbx or L.brushy~=nby then
		L.brushx,L.brushy = nbx,nby
		conSend(34,string.char(L.brushx,L.brushy))
	end
	--check selected elements
	local nsell,nsela,nselr,nselrep = elements[tpt.selectedl] or eleNameTable[tpt.selectedl],elements[tpt.selecteda] or eleNameTable[tpt.selecteda],elements[tpt.selectedr] or eleNameTable[tpt.selectedr],elements[tpt.selectedreplace] or eleNameTable[tpt.selectedreplace]
	if L.sell~=nsell then
		L.sell=nsell
		conSend(37,string.char(math.floor(L.sell/256),L.sell%256))
	elseif L.sela~=nsela then
		L.sela=nsela
		conSend(37,string.char(math.floor(64 + L.sela/256),L.sela%256))
	elseif L.selr~=nselr then
		L.selr=nselr
		conSend(37,string.char(math.floor(128 + L.selr/256),L.selr%256))
	elseif L.selrep~=nselrep then
		L.selrep=nselrep
		conSend(37,string.char(math.floor(192 + L.selrep/256),L.selrep%256))
	end
	local ncol = sim.decoColour()
	if L.dcolour~=ncol then
		L.dcolour=ncol
		conSend(65,string.char(math.floor(ncol/16777216),math.floor(ncol/65536)%256,math.floor(ncol/256)%256,ncol%256))
	end

	--Tell others to open this save ID, or send screen if opened local browser
	if jacobsmod and L.browseMode and L.browseMode > 3 then
		--hacky hack
		L.browseMode = L.browseMode - 3
	elseif L.browseMode==1 then
		--loaded online save
		local id=sim.getSaveID()
		if L.lastSave~=id then
			L.lastSave=id
			--save a backup for the reload button
			local stampName,fullName = saveStamp(0,0,sim.XRES-1,sim.YRES-1)
			os.remove("stamps/tmp.stm") os.rename(fullName,"stamps/tmp.stm")
			conSend(69,string.char(math.floor(id/65536),math.floor(id/256)%256,id%256))
			deleteStamp(stampName)
		end
		L.browseMode=nil
	elseif L.browseMode==2 then
		--loaded local save (should probably clear sim first instead?)
		L.sendScreen=true
		L.browseMode=nil
	elseif L.browseMode==3 and L.lastSave==sim.getSaveID() then
		L.browseMode=nil
		--save this as a stamp for reloading (unless an api function exists to do this)
		local stampName,fullName = saveStamp(0,0,sim.XRES-1,sim.YRES-1)
		os.remove("stamps/tmp.stm") os.rename(fullName,"stamps/tmp.stm")
		deleteStamp(stampName)
	end

	--Send screen (or an area for known size) for stamps
	if jacobsmod and L.sendScreen == 2 then
		L.sendScreen = true
	elseif L.sendScreen then
		local x,y,w,h = 0,0,sim.XRES-1,sim.YRES-1
		if L.smoved then
			local stm
			if L.copying then stm=L.lastCopy else stm=L.lastStamp end
			if L.rotate then stm.w,stm.h=stm.h,stm.w end
			x,y,w,h = math.floor((L.mousex-stm.w/2)/4)*4,math.floor((L.mousey-stm.h/2)/4)*4,stm.w,stm.h
			L.smoved=false
			L.copying=false
		end
		L.sendScreen=false
		local stampName,fullName = saveStamp(x,y,w,h)
		local f = assert(io.open(fullName,"rb"))
		local s = f:read"*a"
		f:close()
		deleteStamp(stampName)
		local d = #s
		local b1,b2,b3 = math.floor(x/16),((x%16)*16)+math.floor(y/256),(y%256)
		conSend(67,string.char(math.floor(x/16),((x%16)*16)+math.floor(y/256),(y%256),math.floor((x+w)/16),(((x+w)%16)*16)+math.floor((y+h)/256),((y+h)%256)))
		conSend(66,string.char(b1,b2,b3,math.floor(d/65536),math.floor(d/256)%256,d%256)..s)
	end

	--Check if custom modes were changed
	if jacobsmod and L.checkRen == 2 then
		L.checkRen = true
	elseif L.checkRen then
		L.checkRen=false
		local t,send=getViewModes(),false
		for k,v in pairs(t) do
			if v~=L.pModes[k] then
				send=true break
			end
		end
		if send then conSend(64,string.char(t[1],t[2],t[3])) end
	end

	--Send option menu settings
	if L.checkOpt then
		L.checkOpt=false
		conSend(56,string.char(tpt.heat()))
		conSend(53,string.char(tpt.ambient_heat()))
		conSend(54,string.char(tpt.newtonian_gravity()))
		conSend(57,string.char(sim.waterEqualisation()))
		conSend(58,string.char(sim.gravityMode()))
		conSend(59,string.char(sim.airMode()))
		conSend(68,string.char(sim.edgeMode()))
	end

end
local function updatePlayers()
	if con.members then
		for k,v in pairs(con.members) do
			playerMouseMove(k)
		end
	end
	--Keep last frame of stick2
	L.lastStick2=L.stick2
	L.stick2=false
end

local pressedKeys
local function step()
	if not L.chatHidden then chatwindow:draw() else showbutton:draw() end
	if hooks_enabled then
		if pressedKeys and pressedKeys["repeat"] < socket.gettime() then
			if pressedKeys["repeat"] < socket.gettime()-.05 then
				pressedKeys = nil
			else
				chatwindow:textprocess(pressedKeys["key"],pressedKeys["nkey"],pressedKeys["modifier"],pressedKeys["event"])
				pressedKeys["repeat"] = socket.gettime()+.065
			end
		end
		drawStuff()
		sendStuff()
		if L.pauseNextFrame then L.pauseNextFrame=false tpt.set_pause(1) end
		connectThink()
		updatePlayers()
	end
end

--some button locations that emulate tpt, return false will disable button
local tpt_buttons = {
	["open"] = {x1=1, y1=408, x2=17, y2=422, f=function() if not L.ctrl then L.browseMode=1 else L.browseMode=2 end L.lastSave=sim.getSaveID() end},
	["rload"] = {x1=19, y1=408, x2=355, y2=422, firstClick = true, f=function() if L.lastSave then if L.ctrl then infoText:reset("If you re-opened the save, please type /sync") else conSend(70) end else infoText:reset("Reloading local saves is not synced currently. Type /sync") end end},
	["clear"] = {x1=470, y1=408, x2=486, y2=422, f=function() conSend(63) L.lastSave=nil end},
	["opts"] = {x1=581, y1=408, x2=595, y2=422, f=function() L.checkOpt=true end},
	["disp"] = {x1=597, y1=408, x2=611, y2=422, f=function() L.checkRen=true L.pModes=getViewModes() end},
	["pause"] = {x1=613, y1=408, x2=627, y2=422, firstClick = true, f=function() conSend(49,tpt.set_pause()==0 and "\1" or "\0") end},
	["deco"] = {x1=613, y1=33, x2=627, y2=47, f=function() conSend(51,tpt.decorations_enable()==0 and "\1" or "\0") end},
	["newt"] = {x1=613, y1=49, x2=627, y2=63, f=function() conSend(54,tpt.newtonian_gravity()==0 and "\1" or "\0") end},
	["ambh"] = {x1=613, y1=65, x2=627, y2=79, f=function() conSend(53,tpt.ambient_heat()==0 and "\1" or "\0") end},
}
if jacobsmod then
	tpt_buttons["tab"] = {x1=613, y1=1, x2=627, y2=15, firstClick = true, f=function() L.tabs = not L.tabs end}
	tpt_buttons["opts"] = {x1=470, y1=408, x2=484, y2=422, f=function() L.checkOpt=true end}
	tpt_buttons["clear"] = {x1=486, y1=408, x2=502, y2=422, firstClick = true, f=function() conSend(63) L.lastSave=nil end}
	tpt_buttons["disp"] = {x1=597, y1=408, x2=611, y2=422, firstClick = true, f=function() L.checkRen=2 L.pModes=getViewModes() end}
	tpt_buttons["open"] = {x1=1, y1=408, x2=17, y2=422, firstClick = true, f=function() if not L.ctrl then L.browseMode=4 else L.browseMode=5 end L.lastSave=sim.getSaveID() end}
end

local function mouseclicky(mousex,mousey,button,event,wheel)
	if L.chatHidden then showbutton:process(mousex,mousey,button,event,wheel) if not hooks_enabled then return true end end
	if L.stamp and button>0 and button~=2 then
		if event==1 and button==1 and L.stampx == -1 then
			--initial stamp coords
			L.stampx,L.stampy = mousex,mousey
		elseif event==2 then
			if L.skipClick then L.skipClick=false return true end
			--stamp has been saved, make our own copy
			if button==1 then
				--save stamp ourself for data, delete it
				local sx,sy = mousex,mousey
				if sx<L.stampx then L.stampx,sx=sx,L.stampx end
				if sy<L.stampy then L.stampy,sy=sy,L.stampy end
				--cheap cut hook to send a clear
				if L.copying==1 then
					--maybe this is ctrl+x? 67 is clear area
					conSend(67,string.char(math.floor(L.stampx/16),((L.stampx%16)*16)+math.floor(L.stampy/256),(L.stampy%256),math.floor(sx/16),((sx%16)*16)+math.floor(sy/256),(sy%256)))
				end
				local w,h = sx-L.stampx,sy-L.stampy
				local stampName,fullName = saveStamp(L.stampx,L.stampy,w,h)
				sx,sy,L.stampx,L.stampy = math.ceil((sx+1)/4)*4,math.ceil((sy+1)/4)*4,math.floor(L.stampx/4)*4,math.floor(L.stampy/4)*4
				w,h = sx-L.stampx, sy-L.stampy
				local f = assert(io.open(fullName,"rb"))
				if L.copying then L.lastCopy = {data=f:read"*a",w=w,h=h} else L.lastStamp = {data=f:read"*a",w=w,h=h} end
				f:close()
				deleteStamp(stampName)
			end
			L.stamp=false
			L.copying=false
		end
		return true
	elseif L.placeStamp and button>0 and button~=2 then
		if event==2 then
			if L.skipClick then L.skipClick=false return true end
			if button==1 then
				local stm
				if L.copying then stm=L.lastCopy else stm=L.lastStamp end
				if stm then
					if not stm.data then
						--unknown stamp, send full screen on next step, how can we read last created stamp, timestamps on files?
						L.sendScreen = (jacobsmod and 2 or true)
					else
						--send the stamp
						if L.smoved then
							--moved from arrows or rotate, send area next frame
							L.placeStamp=false
							L.sendScreen=true
							return true
						end
						local sx,sy = mousex-math.floor(stm.w/2),mousey-math.floor((stm.h)/2)
						if sx<0 then sx=0 end
						if sy<0 then sy=0 end
						if sx+stm.w>sim.XRES-1 then sx=sim.XRES-stm.w end
						if sy+stm.h>sim.YRES-1 then sy=sim.YRES-stm.h end
						local b1,b2,b3 = math.floor(sx/16),((sx%16)*16)+math.floor(sy/256),(sy%256)
						local d = #stm.data
						conSend(66,string.char(b1,b2,b3,math.floor(d/65536),math.floor(d/256)%256,d%256)..stm.data)
					end
				end
			end
			L.placeStamp=false
			L.copying=false
		end
		return true
	end

	if button > 0 and L.skipClick then L.skipClick=false return true end
	if chatwindow:process(mousex,mousey,button,event,wheel) then return false end
	if mousex<sim.XRES and mousey<sim.YRES then mousex,mousey = sim.adjustCoords(mousex,mousey) end

	local obut,oevnt = L.mButt,L.mEvent
	if button~=obut or event~=oevnt then
		L.mButt,L.mEvent = button,event
		--More accurate mouse from here (because this runs BEFORE step function, it would draw old coords)
		local b1,b2,b3 = math.floor(mousex/16),((mousex%16)*16)+math.floor(mousey/256),(mousey%256)
		conSend(32,string.char(b1,b2,b3))
		L.mousex,L.mousey = mousex,mousey
		conSend(33,string.char(L.mButt*16+L.mEvent))
	elseif L.mEvent==3 and (L.mousex~=mousex or L.mousey~=mousey) then
		local b1,b2,b3 = math.floor(mousex/16),((mousex%16)*16)+math.floor(mousey/256),(mousey%256)
		conSend(32,string.char(b1,b2,b3))
		L.mousex,L.mousey = mousex,mousey
	end
	--Temporary SPRK floodfill crash block
	if L.shift and L.ctrl and event == 1 and ((button == 1 and L.sell==15) or (button == 4 and L.selr==15) or (button == 2 and L.sela==15)) then
		return false
	end

	--Click inside button first
	if button==1 then
		if event==1 then
			if jacobsmod and (L.tabs or L.ctrl) and mousex>=613 and mousex<=627 and mousey >=17 and mousey<=143 then
				L.sendScreen = 2
			end
			for k,v in pairs(tpt_buttons) do
				if mousex>=v.x1 and mousex<=v.x2 and mousey>=v.y1 and mousey<=v.y2 then
					if jacobsmod and tpt_buttons[k].firstClick then
						return tpt_buttons[k].f()~=false
					else
						L.downInside = k
					end
					break
				end
			end
		--Up inside the button we started with
		elseif event==2 and L.downInside then
			local butt = tpt_buttons[L.downInside]
			if (jacobsmod and not butt.firstClick) or (mousex>=butt.x1 and mousex<=butt.x2 and mousey>=butt.y1 and mousey<=butt.y2) then
				L.downInside = nil
				return butt.f()~=false
			end
		--Mouse hold, we MUST stay inside button or don't trigger on up
		elseif event==3 and L.downInside then
			local butt = tpt_buttons[L.downInside]
			if mousex<butt.x1 or mousex>butt.x2 or mousey<butt.y1 or mousey>butt.y2 then
				L.downInside = nil
			end
		end
	end
end

local keypressfuncs = {
	--TAB
	[9] = function() if not jacobsmod or not L.ctrl then conSend(35) end end,

	--ESC
	[27] = function() if not L.chatHidden then L.chatHidden = true TPTMP.chatHidden = true return false end end,

	--space, pause toggle
	[32] = function() conSend(49,tpt.set_pause()==0 and "\1" or "\0") end,

	--View modes 0-9
	[48] = function() conSend(48,"\10") end,
	[49] = function() if L.shift then conSend(48,"\9") tpt.display_mode(9)--[[force local display mode, screw debug check for now]] return false end conSend(48,"\0") end,
	[50] = function() conSend(48,"\1") end,
	[51] = function() conSend(48,"\2") end,
	[52] = function() conSend(48,"\3") end,
	[53] = function() conSend(48,"\4") end,
	[54] = function() conSend(48,"\5") end,
	[55] = function() conSend(48,"\6") end,
	[56] = function() conSend(48,"\7") end,
	[57] = function() conSend(48,"\8") end,

	--semicolon / ins / del for replace mode
	[59] = function() if L.ctrl then  L.replacemode = bit.bxor(L.replacemode, 2) else  L.replacemode = bit.bxor(L.replacemode, 1) end conSend(38, L.replacemode) end,
	[277] = function() L.replacemode = bit.bxor(L.replacemode, 1) conSend(38, L.replacemode) end,
	[127] = function() L.replacemode = bit.bxor(L.replacemode, 2) conSend(38, L.replacemode) end,

	--= key, pressure/spark reset
	[61] = function() if L.ctrl then conSend(60) else conSend(61) end end,

	--`, console
	[96] = function() if not L.shift and con.connected then infoText:reset("Console does not sync, use shift+` to open instead") return false end end,

	--b , deco, pauses sim
	[98] = function() if L.ctrl then conSend(51,tpt.decorations_enable()==0 and "\1" or "\0") else conSend(49,"\1") conSend(51,"\1") end end,

	--c , copy
	[99] = function() if L.ctrl then L.stamp=true L.copying=true L.stampx = -1 L.stampy = -1 end end,

	--d key, debug, api broken right now
	--[100] = function() conSend(55) end,

	--F , frame step
	[102] = function() if not jacobsmod or not L.ctrl then conSend(50) end end,

	--H , HUD and intro text
	[104] = function() if L.ctrl and jacobsmod then return false end end,

	--I , invert pressure
	[105] = function() conSend(62) end,

	--K , stamp menu, abort our known stamp, who knows what they picked, send full screen?
	[107] = function() L.lastStamp={data=nil,w=0,h=0} L.placeStamp=true end,

	--L , last Stamp
	[108] = function() if L.lastStamp then L.placeStamp=true end end,

	--N , newtonian gravity or new save
	[110] = function() if jacobsmod and L.ctrl then L.sendScreen=2 L.lastSave=nil else conSend(54,tpt.newtonian_gravity()==0 and "\1" or "\0") end end,

	--O, old menu in jacobs mod
	[111] = function() if jacobsmod and not L.ctrl then if tpt.oldmenu()==0 and showbutton.y < 150 then showbutton:onmove(0, 256) elseif showbutton.y > 150 then showbutton:onmove(0, -256) end end end,

	--R , for stamp rotate
	[114] = function() if L.placeStamp then L.smoved=true if L.shift then return end L.rotate=not L.rotate elseif L.ctrl then conSend(70) end end,

	--S, stamp
	[115] = function() if (L.lastStick2 and not L.ctrl) or (jacobsmod and L.ctrl) then return end L.stamp=true L.stampx = -1 L.stampy = -1 end,

	--T, tabs
	[116] = function() if jacobsmod then L.tabs = not L.tabs end end,

	--U, ambient heat toggle
	[117] = function() conSend(53,tpt.ambient_heat()==0 and "\1" or "\0") end,

	--V, paste the copystamp
	[118] = function() if L.ctrl and L.lastCopy then L.placeStamp=true L.copying=true end end,

	--X, cut a copystamp and clear
	[120] = function() if L.ctrl then L.stamp=true L.copying=1 L.stampx = -1 L.stampy = -1 end end,

	--W,Y (grav mode, air mode)
	[119] = function() if L.lastStick2 and not L.ctrl then return end conSend(58,string.char((sim.gravityMode()+1)%3)) return true end,
	[121] = function() conSend(59,string.char((sim.airMode()+1)%5)) return true end,
	--Z
	[122] = function() myZ=true L.skipClick=true end,

	--Arrows for stamp adjust
	[273] = function() if L.placeStamp then L.smoved=true end end,
	[274] = function() if L.placeStamp then L.smoved=true end end,
	[275] = function() if L.placeStamp then L.smoved=true end end,
	[276] = function() if L.placeStamp then L.smoved=true end end,

	--F1 , intro text
	[282] = function() if jacobsmod then return false end end,

	--F5 , save reload
	[286] = function() conSend(70) end,

	--SHIFT,CTRL,ALT
	[303] = function() L.shift=true conSend(36,string.char(17)) end,
	[304] = function() L.shift=true conSend(36,string.char(17)) end,
	[305] = function() L.ctrl=true conSend(36,string.char(1)) end,
	[306] = function() L.ctrl=true conSend(36,string.char(1)) end,
	[307] = function() L.alt=true conSend(36,string.char(33)) end,
	[308] = function() L.alt=true conSend(36,string.char(33)) end,
}
local keyunpressfuncs = {
	--Z
	[122] = function() myZ=false L.skipClick=false if L.alt then L.skipClick=true end end,
	--SHIFT,CTRL,ALT
	[303] = function() L.shift=false conSend(36,string.char(16)) end,
	[304] = function() L.shift=false conSend(36,string.char(16)) end,
	[305] = function() L.ctrl=false conSend(36,string.char(0)) end,
	[306] = function() L.ctrl=false conSend(36,string.char(0)) end,
	[307] = function() L.alt=false conSend(36,string.char(32)) end,
	[308] = function() L.alt=false conSend(36,string.char(32)) end,
}
local function keyclicky(key,nkey,modifier,event)
	if not hooks_enabled then
		if jacobsmod and bit.band(modifier, 0xC0) == 0 and key == 'o' and event == 1 then if tpt.oldmenu()==0 and showbutton.y < 150 then showbutton:onmove(0, 256) elseif showbutton.y > 150 then showbutton:onmove(0, -256) end end
		return
	end
	if chatwindow.inputbox.focus then
		if event == 1 and nkey~=13 and nkey~=27 then
			pressedKeys = {["repeat"] = socket.gettime()+.6, ["key"] = key, ["nkey"] = nkey, ["modifier"] = modifier, ["event"] = event}
		elseif event == 2 and pressedKeys and nkey == pressedKeys["nkey"] then
			pressedKeys = nil
		end
	end
	local check = chatwindow:textprocess(key,nkey,modifier,event)
	if type(check)=="boolean" then return not check end
	--_print(nkey)
	local ret
	if event==1 then
		if keypressfuncs[nkey] then
			ret = keypressfuncs[nkey]()
		end
	elseif event==2 then
		if keyunpressfuncs[nkey] then
			ret = keyunpressfuncs[nkey]()
		end
	end
	if ret~= nil then return ret end
end

function TPTMP.disableMultiplayer()
	tpt.unregister_step(step)
	tpt.unregister_mouseclick(mouseclicky)
	tpt.unregister_keypress(keyclicky)
	TPTMP = nil
	disconnected("TPTMP unloaded")
end

function TPTMP.enableMultiplayer()
	hooks_enabled = true
	TPTMP.enableMultiplayer = nil
	debug.sethook(nil,"",0)
	if jacobsmod then
		--clear intro text tooltip
		gfx.toolTip("", 0, 0, 0, 4)
	end
end
TPTMP.con = con
TPTMP.chatHidden = true
tpt.register_step(step)
tpt.register_mouseclick(mouseclicky)
tpt.register_keypress(keyclicky)
