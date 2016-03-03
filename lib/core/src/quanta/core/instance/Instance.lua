u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Vessel = require "Quanta.Vessel"
local Measurement = require "Quanta.Measurement"
local Prop = require "Quanta.Prop"
local M = U.module(...)

U.class(M)

function M:__init()
	self.name = ""
	self.name_hash = O.NAME_NULL
	-- NB: can be shared!
	self.scope = nil
	-- Entity or Unit
	self.item = nil
	self.description = Prop.Description.struct("")
	self.source = 0
	self.sub_source = 0
	self.source_certain = true
	self.sub_source_certain = true
	self.variant_certain = true
	self.presence_certain = true
	self.measurements = {}
	self.selection = {}
	self.modifiers = {}
end

function M:set_name(name)
	self.name = name
	-- utf8(¿) => C2 BF
	if string.sub(self.name, -2) == "¿" then
		self.name = string.sub(
			self.name, 1,
			string.sub(self.name, -3, -3) == "_" and -4 or -3
		)
		self.variant_certain = false
	else
		self.variant_certain = true
	end
	self.name_hash = O.hash_name(self.name)
end

function M:from_object(obj, implicit_scope)
	U.type_assert(obj, "userdata")

	self:__init()

	local context = Vessel.new_match_context(implicit_scope)
	if not context:consume(M.t_head, obj, self) then
		return false, context.error:to_string()
	end
	return true
end

function M:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	elseif self.scope then
		O.clear(obj)
	end

	if self.scope then
		O.set_time_date(obj, self.scope)
		obj = O.push_child(obj)
	end

	if self.name_hash ~= O.NAME_NULL then
		O.set_identifier(obj, self.name .. (self.variant_certain and "" or "¿"))
	else
		O.set_null(obj)
	end

	O.set_source(obj, self.source)
	O.set_sub_source(obj, self.sub_source)
	O.set_source_certain(obj, self.source_certain)
	O.set_sub_source_certain(obj, self.sub_source_certain)
	O.set_value_certain(obj, self.presence_certain)

	if #self.measurements == 0 or self.measurements[1]:is_empty() then
		O.release_quantity(obj)
	elseif #self.measurements == 1 then
		self.measurements[1]:to_object(O.make_quantity(obj))
	elseif #self.measurements > 1 then
		local quantity = O.make_quantity(obj)
		for _, measurement in pairs(self.measurements) do
			if not measurement:is_empty() then
				measurement:to_object(O.push_child(quantity))
			end
		end
		if not O.has_children(quantity) then
			O.release_quantity(quantity)
		end
	end

	O.clear_children(obj)
	Prop.Description.struct_to_object(self.description, obj)
	for _, sel in pairs(self.selection) do
		sel:to_object(O.push_child(obj))
	end
	O.clear_tags(obj)
	for _, modifier in pairs(self.modifiers) do
		modifier:to_object(O.push_tag(obj))
	end

	return obj
end

M.Modifier = U.class(M.Modifier)

function M.Modifier:__init()
	self.id = nil
	self.id_hash = O.NAME_NULL
	self.data = nil
end

function M.Modifier:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	end

	O.set_name(obj, self.id)
	if self.data then
		self.data:to_object(self, obj)
	end
end

M.UnknownModifier = U.class(M.UnknownModifier)

function M.UnknownModifier:__init(obj)
	U.type_assert(obj, "userdata", true)

	self.obj = obj or O.create()
end

function M.UnknownModifier:from_object(context, instance, modifier, obj)
	O.copy_children(self.obj, obj)
end

function M.UnknownModifier:to_object(modifier, obj)
	O.copy_children(obj, self.obj)
end

function M.UnknownModifier:compare_equal(other)
	-- TODO?
	return true
end

M.Modifier.p_head = Match.Pattern{
	name = true,
	children = Match.Any,
	acceptor = function(context, instance, obj)
		local modifier = M.Modifier()
		modifier.id = O.name(obj)
		modifier.id_hash = O.name_hash(obj)
		table.insert(instance.modifiers, modifier)

		return context.user.director:read_modifier(context, instance, modifier, obj)
	end,
}

M.t_head = Match.Tree()
M.t_body = Match.Tree()

M.t_head:add({
-- x, x:m, x{...}
Match.Pattern{
	vtype = O.Type.identifier,
	children = M.t_body,
	tags = {M.Modifier.p_head},
	quantity = Match.Any,
	acceptor = function(context, self, obj)
		self:set_name(O.identifier(obj))
		if #context.user.scope > 0 then
			self.scope = U.table_last(context.user.scope)
		end

		self.source = O.source(obj)
		self.sub_source = O.sub_source(obj)
		self.source_certain = not O.marker_source_uncertain(obj)
		self.sub_source_certain = not O.marker_sub_source_uncertain(obj)
		self.presence_certain = not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))

		if O.has_quantity(obj) then
			local quantity = O.quantity(obj)
			if O.is_null(quantity) and O.has_children(quantity) then
				for _, sub in O.children(quantity) do
					local m = Measurement(sub)
					if not m:is_empty() then
						table.insert(self.measurements, m)
					end
				end
			elseif not O.is_null(quantity) then
				local m = Measurement(quantity)
				if not m:is_empty() then
					table.insert(self.measurements, m)
				end
			end
		end
	end,
},
-- :m
Match.Pattern{
	vtype = O.Type.null,
	tags = {M.Modifier.p_head},
},
})

M.t_body:add({
Prop.Description.t_struct_head,
Match.Pattern{
	any = true,
	branch = M.t_head,
	acceptor = function(context, self, obj)
		table.insert(context.user.scope_save, context.user.scope)
		context.user.scope = {}
		local instance = M()
		table.insert(self.selection, instance)
		return instance
	end,
	post_branch = function(context, self, obj)
		context.user.scope = table.remove(context.user.scope_save)
	end,
},
})

M.t_body:build()
M.t_head:build()

return M

)"__RAW_STRING__"