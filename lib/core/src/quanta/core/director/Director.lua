u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Tracker = require "Quanta.Tracker"
local M = U.module(...)

M.debug = false

U.class(M)

function M:__init()
	self.action_classes = {}
end

function M:read_action(context, entry, action, obj)
	local action_class
	local registered = self.action_classes[action.id_hash]
	if not registered then
		action_class = Tracker.UnknownAction
	else
		action_class = registered.class
		if M.debug then
			U.assert(action.id == registered.id)
		end
	end

	action.data = action_class()
	return action.data:from_object(context, entry, action, obj)
end

function M:register(id, class)
	U.type_assert(id, "string")
	U.assert(U.is_functable(class))

	local id_hash = O.hash_name(id)
	U.assert(id_hash ~= O.NAME_NULL, "action id must be non-empty")
	U.assert(not self.action_classes[id], "action '%s' is already registered", id)

	self.action_classes[id_hash] = {
		id = id,
		id_hash = id_hash,
		class = class,
	}
end

return M

)"__RAW_STRING__"