u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Tracker = require "Quanta.Tracker"
local M = U.module(...)

U.class(M)

function M:__init()
	self.action_templates = {}
end

function M:read_action(context, into, obj)
	-- TODO
end

return M

)"__RAW_STRING__"