u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Vessel = require "Quanta.Vessel"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local M = U.module(...)

U.class(M)

function M:__init()
	self.name = nil
	self.name_hash = O.NAME_NULL

	-- Instance, Composition, Unit
	self.items = {}
	self.modifiers = Instance.Modifier.struct_list({})
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

function M:from_object(obj, implicit_scope)
	U.type_assert(obj, "userdata")

	self.items = {}
	self.measurements = {}
	self.modifiers = {}

	local context = Vessel.new_match_context(implicit_scope)
	if not context:consume(M.t_head, obj, self) then
		return false, context.error:to_string()
	end
	return true
end

function M:to_object(obj, keep)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	elseif not keep then
		O.clear(obj)
	end

	if self.name_hash ~= O.NAME_NULL then
		O.set_name(obj, self.name)
	else
		O.clear_name(obj)
	end

	for _, item in pairs(self.items) do
		item:to_object(O.push_child(obj))
	end
	Instance.Modifier.struct_list_to_tags(self.modifiers, obj)
	Measurement.struct_list_to_quantity(self.measurements, obj)

	return obj
end

M.t_head = Match.Tree()
M.t_body = Match.Tree()

-- FIXME: cyclic dependency
if not Quanta.Unit then
	require "Quanta.Unit"
end

local function instance_acceptor(context, self, obj)
	local item = Instance()
	table.insert(self.items, item)
	return item
end

M.p_item = {
-- sub unit
Match.Pattern{
	layer = Quanta.Unit.p_head,
	acceptor = function(context, self, obj)
		local item = Quanta.Unit()
		table.insert(self.items, item)
		return item
	end,
},
-- sub instance
Match.Pattern{
	layer = Instance.p_head_id,
	acceptor = instance_acceptor,
},
Match.Pattern{
	layer = Instance.p_head_empty,
	acceptor = instance_acceptor,
},
}

-- time context
M.t_head:add({
Match.Pattern{
	vtype = O.Type.time,
	tags = false,
	children = M.t_head,
	quantity = false,
	acceptor = function(context, self, obj)
		local t
		if O.has_clock(obj) then
			return Match.Error("contextual block must not have clock time")
		elseif O.num_children(obj) == 0 then
			return Match.Error("contextual block is empty")
		elseif O.is_date_contextual(obj) then
			local parent_scope = U.table_last(context.user.scope, true) or context.user.implicit_scope
			if not parent_scope then
				return Match.Error("no time context provided; contextual block time cannot be resolved")
			end
			t = O.time_resolved(obj, parent_scope)
		else
			t = T(O.time(obj))
		end
		T.clear_clock(t)
		T.adjust_zone_utc(t)
		table.insert(context.user.scope, t)
	end,
	post_branch = function(context, self, obj)
		table.remove(context.user.scope)
	end,
},
-- {...}, (x + y ...)
Match.Pattern{
	name = Match.Any,
	vtype = {O.Type.null, O.Type.expression},
	children = M.t_body,
	tags = Instance.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	func = function(_, _, obj)
		return O.has_children(obj)
	end,
	acceptor = function(context, self, obj)
		self:set_name(O.name(obj))
	end,
},
})

-- sub-items
M.t_head:add(M.p_item)

M.t_body:add({
-- sub items
M.p_item,
-- sub composition
Match.Pattern{
	any = true,
	branch = M.t_head,
	acceptor = function(context, self, obj)
		local item = M()
		table.insert(self.items, item)
		return item
	end,
},
})

M.t_body:build()
M.t_head:build()

return M

)"__RAW_STRING__"