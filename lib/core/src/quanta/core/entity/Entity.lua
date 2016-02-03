u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Measurement = require "Quanta.Measurement"
local M = U.module(...)

U.class(M)

function M:__init(name)
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
	self.name = name
	self.name_hash = O.hash_name(name)
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
	return e
end)

function M.is_category(entity)
	return U.is_type(entity, Entity) and entity.is_category
end

function M.is_universe(entity)
	return M.is_category(entity) and entity.universe == entity.universe
end

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

return M

)"__RAW_STRING__"