
local U = require "Quanta.Util"
local O = require "Quanta.Object"
local Entity = require "Quanta.Entity"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local M = U.module("Quanta.Composition")

U.class(M)

function M:__init(obj, search_in, controllers)
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
	if O.is_type_any(obj, O.Type.null + O.Type.expression) and O.has_children(obj) then
		-- {...}, (x + y)
		for _, sub in O.children(obj) do
			U.assert(O.op(sub) == O.Operator.add)
			table.insert(self.items, Instance(sub, search_in, controllers))
		end

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
	else
		-- x, x:y, :y
		table.insert(self.items, Instance(obj, search_in, controllers))
	end
end

function M:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	end

	O.clear(obj)
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

return M
