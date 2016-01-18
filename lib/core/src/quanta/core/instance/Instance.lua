u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Entity = require "Quanta.Entity"
local Measurement = require "Quanta.Measurement"
local M = U.module(...)

U.class(M)

function M:__init(obj, search_in, controllers)
	self.name = ""
	self.name_hash = O.NAME_NULL
	-- Entity or Unit
	self.item = nil
	self.source = 0
	self.sub_source = 0
	self.source_certain = true
	self.sub_source_certain = true
	self.variant_certain = true
	self.presence_certain = true
	self.measurements = {}
	self.selection = {}
	self.modifiers = {}

	if obj then
		self:from_object(obj, search_in, controllers)
	end
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

function M:from_object(obj, search_in, controllers)
	U.type_assert(obj, "userdata")
	U.type_assert(search_in, "table")
	U.type_assert(controllers, "table")

	self:set_name(O.is_identifier(obj) and O.identifier(obj) or "")
	self.item = nil
	self.source = O.source(obj)
	self.sub_source = O.sub_source(obj)
	self.source_certain = not O.marker_source_uncertain(obj)
	self.sub_source_certain = not O.marker_sub_source_uncertain(obj)
	self.presence_certain = not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))

	self.measurements = {}
	self.selection = {}
	self.modifiers = {}

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

	if self.name_hash ~= O.NAME_NULL then
		for _, s in pairs(search_in) do
			if Entity.is_universe(s) then
				self.item = s.index_hash[self.name_hash]
			else
				U.type_assert(s, "table")
				self.item = s[self.name_hash]
			end
			if self.item then
				break
			end
		end
	end

	for _, sub in O.children(obj) do
		if O.is_named(sub) then
			-- TODO: pattern matching
			if O.name_hash(sub) == O.name_hash("d") then
				self.description = O.text(sub)
			end
		else
			table.insert(self.selection, M(sub, search_in, controllers))
		end
	end
	for _, sub in O.tags(obj) do
		table.insert(self.modifiers, M.Modifier(sub, controllers))
	end
end

function M:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
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

function M.Modifier:__init(obj, controllers)
	self.name = nil
	self.name_hash = nil
	self.controller = nil

	if obj then
		self:from_object(obj, controllers)
	end
end

function M.Modifier:from_object(obj, controllers)
	U.type_assert(obj, "userdata")
	U.type_assert(controllers, "table")

	self.name = O.name(obj)
	self.name_hash = O.name_hash(obj)
	self.controller = controllers[self.name_hash] -- or controllers[self.name]
	if self.controller then
		self.controller.from_object(self, obj, controllers)
	end
end

function M.Modifier:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	end

	O.set_name(obj, self.name)
	if self.controller then
		self.controller.to_object(self, obj)
	end
end

function M.Modifier:apply(items)
	if self.controller then
		return self.controller.apply(self, items)
	end
	return nil
end

return M

)"__RAW_STRING__"