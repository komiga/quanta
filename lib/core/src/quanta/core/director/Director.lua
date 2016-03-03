u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Prop = require "Quanta.Prop"
local Instance = require "Quanta.Instance"
local Tracker = require "Quanta.Tracker"
local M = U.module(...)

M.debug = false

U.class(M)

function M:__init()
	self.modifier_spec = Prop.Specializer()
	self.entity_spec = Prop.Specializer()
	self.action_spec = Prop.Specializer()
	self.attachment_spec = Prop.Specializer()
end

function M:register_modifier(id, class)
	return self.modifier_spec:register(id, class)
end

function M:register_entity(id, class)
	return self.entity_spec:register(id, class)
end

function M:register_action(id, class)
	return self.action_spec:register(id, class)
end

function M:register_attachment(id, class)
	return self.attachment_spec:register(id, class)
end

function M:find_entity_class(id, id_hash)
	return self.entity_spec:find(id, id_hash)
end

function M:read_modifier(context, instance, modifier, obj)
	return self.modifier_spec:read(context, instance, modifier, obj, Instance.UnknownModifier)
end

function M:read_action(context, entry, action, obj)
	return self.action_spec:read(context, entry, action, obj, Tracker.UnknownAction)
end

function M:read_attachment(context, tracker, attachment, obj)
	return self.attachment_spec:read(context, entry, attachment, obj, Tracker.UnknownAttachment)
end

return M

)"__RAW_STRING__"