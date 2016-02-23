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
	-- Instance, Composition
	self.items = {}
	self.measurement = Measurement()
end

function M:from_object(obj, implicit_scope)
	U.type_assert(obj, "userdata")

	self.items = {}
	self.measurement:__init()

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

	for _, item in pairs(self.items) do
		item:to_object(O.push_child(obj))
	end

	if self.measurement:is_empty() then
		O.release_quantity(obj)
	else
		self.measurement:to_object(O.make_quantity(obj))
	end
	return obj
end

M.t_head = Match.Tree()
M.t_body = Match.Tree()

local function instance_acceptor(context, self, obj)
	local item = Instance()
	table.insert(self.items, item)
	return context:consume(Instance.t_head, obj, item)
end

M.p_instance = {
-- x, x:m, x{...}
Match.Pattern{
	vtype = O.Type.identifier,
	children = Match.Any,
	tags = Match.Any,
	quantity = Match.Any,
	func = function(_, _, obj, _)
		return O.op(obj) == O.Operator.add
	end,
	acceptor = instance_acceptor,
},
-- :m
Match.Pattern{
	vtype = O.Type.null,
	tags = true,
	func = function(_, _, obj, _)
		return O.op(obj) == O.Operator.add
	end,
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
	vtype = {O.Type.null, O.Type.expression},
	children = M.t_body,
	quantity = Match.Any,
	acceptor = function(context, self, obj)
		-- [XXX, 1g], [1g]
		if O.has_quantity(obj) then
			local quantity = O.quantity(obj)
			if O.is_null(quantity) and O.has_children(quantity) then
				for _, sub in O.children(quantity) do
					if self.measurement:from_object(sub) then
						break
					end
				end
			elseif not O.is_null(quantity) then
				self.measurement:from_object(quantity)
			end
		end
	end,
},
})

-- instance
M.t_head:add(M.p_instance)
M.t_body:add(M.p_instance)

-- sub composition
M.t_body:add(Match.Pattern{
	any_branch = M.t_head,
	acceptor = function(context, self, obj)
		local item = M()
		table.insert(self.items, item)
		return item
	end,
})

return M

)"__RAW_STRING__"