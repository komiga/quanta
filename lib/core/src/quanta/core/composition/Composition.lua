u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Entity = require "Quanta.Entity"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local M = U.module(...)

U.class(M)

function M:__init(obj, search_in, controllers)
	-- Instance, Composition
	self.items = {}
	self.measurement = Measurement()

	if obj then
		self:from_object(obj, search_in, controllers)
	end
end

function M:from_object(obj, search_in, controllers)
	U.type_assert(obj, "userdata")
	U.type_assert(search_in, "table")
	U.type_assert(controllers, "table")

	self.items = {}
	self.measurement:__init()

	local context = Match.Context()
	context.user = {
		search_in = search_in,
		controllers = controllers,
	}
	if not context:consume(M.t_head, obj, self) then
		U.log("match error:\n%s", context.error:to_string(context.error_obj))
		return false
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

M.p_instance = Match.Pattern{
	vtype = {O.Type.null, O.Type.identifier},
	tags = Match.Any,
	func = function(_, _, obj, _)
		return O.op(obj) == O.Operator.add
	end,
	acceptor = function(context, self, obj)
		table.insert(self.items, Instance(obj, context.user.search_in, context.user.controllers))
	end,
}

-- {...}, (x + y ...)
M.t_head:add(Match.Pattern{
	vtype = {O.Type.null, O.Type.expression},
	children = M.t_body,
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
})

-- x, x:y, :y
M.t_head:add(M.p_instance)
M.t_body:add(M.p_instance)

-- sub composition
M.t_body:add(Match.Pattern{
	any_branch = M.t_head,
	acceptor = function(context, self, obj)
		return M()
	end,
})

return M

)"__RAW_STRING__"