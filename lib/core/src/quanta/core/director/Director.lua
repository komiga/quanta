u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Instance = require "Quanta.Instance"
local Tracker = require "Quanta.Tracker"
local M = U.module(...)

M.debug = false

U.class(M)

function M:__init()
	self.modifier_classes = {}
	self.action_classes = {}
	self.attachment_classes = {}
end

local function register(bucket, name, id, class)
	U.type_assert(id, "string")
	U.assert(U.is_functable(class))

	local id_hash = O.hash_name(id)
	U.assert(id_hash ~= O.NAME_NULL, "%s id must be non-empty", kind)
	U.assert(not bucket[id_hash], "%s '%s' is already registered", kind, id)

	bucket[id_hash] = {
		id = id,
		id_hash = id_hash,
		class = class,
	}
end

function M:register_modifier(id, class)
	register(self.modifier_classes, "modifier", id, class)
end

function M:register_action(id, class)
	register(self.action_classes, "action", id, class)
end

function M:register_attachment(id, class)
	register(self.attachment_classes, "attachment", id, class)
end

local function read(bucket, unknown_class, context, parent, holder, obj)
	local class
	local registered = bucket[holder.id_hash]
	if not registered then
		class = unknown_class
	else
		class = registered.class
		if M.debug then
			U.assert(holder.id == registered.id)
		end
	end

	holder.data = class()
	return holder.data:from_object(context, parent, holder, obj)
end

function M:read_modifier(context, instance, modifier, obj)
	return read(self.modifier_classes, Instance.UnknownModifier, context, instance, modifier, obj)
end

function M:read_action(context, entry, action, obj)
	return read(self.action_classes, Tracker.UnknownAction, context, entry, action, obj)
end

function M:read_attachment(context, tracker, attachment, obj)
	return read(self.attachment_classes, Tracker.UnknownAttachment, context, entry, attachment, obj)
end

return M

)"__RAW_STRING__"