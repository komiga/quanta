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
	self.name = nil
	self.name_hash = O.NAME_NULL
	self.id = nil
	self.id_hash = O.NAME_NULL

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
	self.selection = {}
	self.modifiers = M.Modifier.struct_list({})
	self.measurements = Measurement.struct_list({})
end

function M:set_name(name)
	U.type_assert(name, "string", true)

	if name and name ~= "" then
		self.name = name
		self.name_hash = O.hash_name(self.name)
	else
		self.name = nil
		self.name_hash = O.NAME_NULL
	end
end

function M:set_id(id)
	U.type_assert(id, "string", true)

	if id and id ~= "" then
		-- utf8(¿) => C2 BF
		if string.sub(id, -2) == "¿" then
			id = string.sub(
				id, 1,
				string.sub(id, -3, -3) == "_" and -4 or -3
			)
			self.variant_certain = false
		else
			self.variant_certain = true
		end
		self.id = id
		self.id_hash = O.hash_name(self.id)
	else
		self.id = nil
		self.id_hash = O.NAME_NULL
	end
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
		O.set_name(obj, self.name)
	else
		O.clear_name(obj)
	end
	if self.id_hash ~= O.NAME_NULL then
		O.set_identifier(obj, self.id .. (self.variant_certain and "" or "¿"))
	else
		O.set_null(obj)
	end

	O.set_source(obj, self.source)
	O.set_sub_source(obj, self.sub_source)
	O.set_source_certain(obj, self.source_certain)
	O.set_sub_source_certain(obj, self.sub_source_certain)
	O.set_value_certain(obj, self.presence_certain)

	O.clear_children(obj)
	Prop.Description.struct_to_object(self.description, obj)
	for _, sel in pairs(self.selection) do
		sel:to_object(O.push_child(obj))
	end
	O.clear_tags(obj)
	M.Modifier.struct_list_to_tags(self.modifiers, obj)
	Measurement.struct_list_to_quantity(self.measurements, obj)

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

function M.Modifier.struct_list(list)
	U.type_assert(list, "table")
	return list
end

function M.Modifier.struct_list_to_tags(list, obj)
	for _, m in pairs(list) do
		m:to_object(O.push_tag(obj))
	end
end

-- TODO: use a pattern instead of passively ignoring non-matching whole quantities
function M.Modifier.adapt_struct_list(property_name)
	local p_element = Match.Pattern{
		name = true,
		children = Match.Any,
		acceptor = function(context, parent, obj)
			local modifier = M.Modifier()
			modifier.id = O.name(obj)
			modifier.id_hash = O.name_hash(obj)
			table.insert(parent.modifiers, modifier)

			return context.user.director:read_modifier(context, parent, modifier, obj)
		end,
	}

	local t_head = Match.Tree({
	-- [..., ...]
	Match.Pattern{
		children = {
			p_element,
		},
	},
	-- [...]
	p_element,
	})
	t_head:build()

	return M.Modifier.struct_list_to_tags, t_head
end

_, M.Modifier.t_struct_list_head = M.Modifier.adapt_struct_list("measurements")

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

M.t_head = Match.Tree()
M.t_body = Match.Tree()

local function translate_basic(self, obj)
	self:set_name(O.name(obj))
	self.source = O.source(obj)
	self.sub_source = O.sub_source(obj)
	self.source_certain = not O.marker_source_uncertain(obj)
	self.sub_source_certain = not O.marker_sub_source_uncertain(obj)
	self.presence_certain = not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))
end

-- x, x:m, x{...}
M.p_head_id = Match.Pattern{
	name = Match.Any,
	vtype = {O.Type.identifier, O.Type.string},
	children = M.t_body,
	tags = M.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	acceptor = function(context, self, obj)
		translate_basic(self, obj)
		if O.is_identifier(obj) then
			self:set_id(O.identifier(obj))
		else
			self:set_id(O.string(obj))
		end
		if #context.user.scope > 0 then
			self.scope = U.table_last(context.user.scope)
		end
	end,
}

-- ?, :m
M.p_head_empty = Match.Pattern{
	name = Match.Any,
	vtype = O.Type.null,
	tags = M.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	acceptor = function(context, self, obj)
		self:set_name(O.name(obj))
		translate_basic(self, obj)
	end,
}

M.t_head:add({
M.p_head_id,
M.p_head_empty,
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