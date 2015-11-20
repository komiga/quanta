
local U = require "Quanta.Util"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Measurement = require "Quanta.Measurement"
local M = U.module("Quanta.Entity")

U.class(M)

function M:__init(name)
	U.type_assert(name, "string")

	self.is_category = false
	self.name = nil
	self.name_hash = O.NAME_NULL
	self.description = nil
	self.compositor = nil
	self.sources = {}
	self.sources[0] = M.Source()
	self.universe = nil
	self.parent = nil
	self.children = {}
	self:set_name(name)
end

function M:set_name(name)
	U.type_assert(name, "string")
	U.assert(#name > 0, "name cannot be empty")
	if self.universe then
		M.Universe.remove_index(self.universe, self)
	end
	self.name = name
	self.name_hash = O.hash_name(name)
	if self.universe then
		M.Universe.add_index(self.universe, self)
	end
end

function M:set_compositor(compositor)
	U.type_assert(compositor, Match.Tree)
	self.compositor = compositor
end

function M:has_source()
	return #self.sources > 0
end

function M:set_source(i, source)
	U.type_assert(i, "number")
	U.type_assert(source, M.Source)
	self.sources[i] = source
	return source
end

function M:add_source(source)
	U.type_assert(source, M.Source)
	table.insert(self.sources, source)
	return source
end

function M:source(i)
	U.type_assert(i, "number", true)
	if i == nil then
		return self.sources[0]
	end
	return self.sources[i]
end

function M:add(entity)
	U.type_assert(entity, M)
	U.assert(self.universe)
	if entity.parent == self then
		return
	end
	U.assert(entity.parent == nil)
	table.insert(self.children, entity)
	entity.universe = self.universe
	entity.parent = self
	if not entity.compositor and self.compositor then
		entity.compositor = self.compositor
	end
	M.Universe.add_index(self.universe, entity)
	return entity
end

function M:find_child(name)
	U.type_assert_any(name, {"string", "number"})
	local name_hash
	if U.is_type(name, "string") then
		name_hash = O.hash_name(name)
	else
		name_hash = name
	end
	for _, e in ipairs(self.children) do
		if e.name_hash == name_hash then
			return e
		end
	end
	return nil
end

M.Category = {}

U.set_functable(M.Category, function(_, name)
	local e = M(name)
	e.is_category = true
	e.sources = {}
	return e
end)

function M.Category:set_description(description)
	U.type_assert(description, "string")
	self.description = description
end

function M.Category:description()
	return self.description
end

M.Universe = {}

U.set_functable(M.Universe, function(_, name)
	local e = M.Category(name)
	e.universe = e
	e.index = {}
	e.index_hash = {}
	return e
end)

function M.Universe:add_index(entity)
	U.assert(self.index ~= nil)
	U.assert(self.index[entity.name] == nil, "entity name '%s' is not unique within the universe", entity.name)
	local h = O.hash_name(entity.name)
	U.assert(
		self.index_hash[h] == nil, "entity name '%s' is not unique by hash (%d, '%s') the universe",
		entity.name, h, self.index_hash[entity.name]
	)
	self.index[entity.name] = entity
	self.index_hash[h] = entity
end

function M.Universe:remove_index(entity)
	self.index[entity.name] = nil
	self.index_hash[entity.name_hash] = nil
end

M.Unit = {}

U.set_functable(M.Unit, function(_, name)
	local e = M(name)
	e.is_unit = true
	e.sources = {}
	return e
end)

M.Source = U.class(M.Source)

function M.Source:__init()
	self.description = ""
	self.label = nil
	self.author = nil
	self.vendors = {}
	self.base_model = nil
	self.model = nil
	self.composition = {}
end

function M.Source:set_description(description)
	U.type_assert(description, "string")
	self.description = description
end

function M.Source:set_label(label)
	U.type_assert(label, "string")
	self.label = label
end

function M.Source:set_author(author)
	U.type_assert(author, M.Place)
	self.author = author
end

function M.Source:set_base_model(base_model)
	U.type_assert(base_model, M.Model)
	self.base_model = base_model
end

function M.Source:set_model(model)
	U.type_assert(model, M.Model)
	self.model = model
end

function M.Source:has_vendor()
	return self.vendor[0] ~= nil or #self.vendors
end

function M.Source:set_vendor(i, vendor)
	U.type_assert(i, "number")
	U.type_assert(vendor, M.Place)
	self.vendors[i] = vendor
	return vendor
end

function M.Source:add_vendor(vendor)
	U.type_assert(vendor, M.Place)
	table.insert(self.vendors, vendor)
	return vendor
end

function M.Source:vendor(i)
	U.type_assert(i, "number")
	return self.vendors[i]
end

M.Place = U.class(M.Place)

function M.Place:__init()
	self.name = nil
	self.address = nil
end

function M.Place:set_name(name)
	U.type_assert(name, "string", true)
	self.name = name
end

function M.Place:set_address(address)
	U.type_assert(address, "string", true)
	self.address = address
end

M.Model = U.class(M.Model)

function M.Model:__init()
	self.name = nil
	self.id = nil
end

function M.Model:set_name(name)
	U.type_assert(name, "string", true)
	self.name = name
end

function M.Model:set_id(id)
	U.type_assert(id, "string", true)
	self.id = id
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
		return self.controller(self, items)
	end
	return nil
end

M.Instance = U.class(M.Instance)

function M.Instance:__init(obj, search_in, controllers)
	self.name = nil
	self.name_hash = O.NAME_NULL
	-- Entity or Unit?
	self.entity = nil
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

function M.Instance:from_object(obj, search_in, controllers)
	U.type_assert(obj, "userdata")
	U.type_assert(search_in, "table")
	U.type_assert(controllers, "table")

	self.name = O.text(obj)

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
	self.entity = nil
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
		if not O.is_expression(quantity) and O.has_children(quantity) then
			for _, sub in O.children(quantity) do
				local m = Measurement(sub)
				if not m:is_empty() then
					table.insert(self.measurements, m)
				end
			end
		else
			local m = Measurement(quantity)
			if not m:is_empty() then
				table.insert(self.measurements, m)
			end
		end
	end

	for _, universe in pairs(search_in) do
		local entity = universe.index[self.name]
		if entity then
			self.entity = entity
			break
		end
	end

	for _, sub in O.children(obj) do
		if O.is_named(sub) then
			-- TODO: pattern matching
			if O.name_hash(sub) == O.name_hash("d") then
				self.description = O.text(sub)
			end
		else
			table.insert(self.selection, M.Instance(sub, search_in, controllers))
		end
	end
	for _, sub in O.tags(obj) do
		table.insert(self.modifiers, M.Modifier(sub, controllers))
	end
end

function M.Instance:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	end

	O.set_identifier(obj, self.name .. (self.variant_certain and "" or "¿"))
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

M.InstanceBlock = U.class(M.InstanceBlock)

function M.InstanceBlock:__init(obj, search_in, controllers)
	self.items = {}

	if obj then
		self:from_object(obj, search_in, controllers)
	end
end

function M.InstanceBlock:from_object(obj, search_in, controllers)
	U.type_assert(obj, "userdata")
	U.type_assert(search_in, "table")
	U.type_assert(controllers, "table")

	self.items = {}
	if O.is_null(obj) then
		for _, sub in O.children(obj) do
			table.insert(self.items, M.Instance(sub, search_in, controllers))
		end
	else
		table.insert(self.items, M.Instance(obj, search_in, controllers))
	end
end

function M.InstanceBlock:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	end

	O.clear(obj)
	O.release_quantity(obj)
	for _, item in pairs(self.items) do
		item:to_object(O.push_child(obj))
	end
	return obj
end

return M
