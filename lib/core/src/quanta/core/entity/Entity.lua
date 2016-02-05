u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Measurement = require "Quanta.Measurement"
local M = U.module(...)

M.Type = {
	generic = 1,
	category = 2,
	universe = 3,
}

U.class(M)

function M:__init(name)
	self.type = M.Type.generic
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

function M:is_category()
	return U.is_type(self, M) and self.type == M.Type.category
end

function M:is_universe()
	return U.is_type(self, M) and self.type == M.Type.universe
end

function M:set_name(name)
	U.type_assert(name, "string")
	U.assert(#name > 0, "name cannot be empty")
	if self.parent then
		self.parent.children[self.name_hash] = nil
	end
	self.name = name
	self.name_hash = O.hash_name(name)
	if self.parent then
		self.parent:add_key(self)
	end
end

function M:ref()
	local parts = {}
	local entity = self
	while entity and entity.type ~= M.Type.universe do
		table.insert(parts, 1, entity.name)
		entity = entity.parent
	end
	return table.concat(parts, '.')
end

function M:set_compositor(compositor)
	U.type_assert(compositor, Match.Tree)
	self.compositor = compositor
end

function M:any_sources()
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

function M:has_variant(source, sub_source)
	if source == 0 then
		return true
	end
	local s = self:source(source)
	if s then
		return sub_source == 0 or s:vendor(sub_source) ~= nil
	end
	return nil
end

function M:add(entity)
	U.type_assert(entity, M)
	U.assert(self.universe)
	if entity.parent == self then
		return
	end
	U.assert(entity.parent == nil)
	entity.universe = self.universe
	entity.parent = self
	self:add_key(entity)
	if not entity.compositor and self.compositor then
		entity.compositor = self.compositor
	end
	return entity
end

function M:add_key(entity)
	U.type_assert(entity, M)
	local current = self.children[entity.name_hash]
	if current then
		if current == entity then
			return
		end
		U.assert(
			false,
			"entity name '%s' is not unique by hash (%d => '%s')",
			entity.name, entity.name_hash, current.name
		)
	end
	self.children[entity.name_hash] = entity
end

function M:find(name)
	U.type_assert_any(name, {"string", "number"})
	local name_hash
	if U.is_type(name, "string") then
		name_hash = O.hash_name(name)
	else
		name_hash = name
	end
	return self.children[name_hash]
end

M.Category = {}

U.set_functable(M.Category, function(_, name)
	local e = M(name)
	e.type = M.Type.category
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
	e.type = M.Type.universe
	e.universe = e
	return e
end)

function M.make_search_branch(entity, depth)
	U.type_assert(entity, M)
	U.type_assert(depth, "number")
	return {entity, depth}
end

local BYTE_DOT = string.byte('.')

local function ref_parts(ref)
	local parts = {}
	local root_ref = string.byte(ref, 1, 1) == BYTE_DOT
	local s = root_ref and 1 or 0
	local e = s
	local hash
	repeat
		e, _ = string.find(ref, ".", s, true)
		if e == nil then
			e = #ref + 1
		end
		hash = O.hash_name(string.sub(ref, s, e - 1))
		if hash ~= O.NAME_NULL then
			table.insert(parts, hash)
		end
		s = e + 1
	until e >= #ref
	return parts, root_ref
end

local function find_part(entity, search_depth, parts, index, hash)
	if index > #parts then
		return entity
	end
	local next_index = index + 1
	local next_hash = parts[next_index]
	local child = entity.children[hash]
	if child then
		local f = find_part(child, 0, parts, next_index, next_hash)
		if f then
			return f
		end
	end

	if search_depth > 0 then
		-- BFS
		local search_list = {{entity, 0}}
		repeat
			local node = table.remove(search_list)
			local above_depth = node[2] <= search_depth
			for child_hash, child in pairs(node[1].children) do
				if child_hash == hash then
					local f = find_part(child, 0, parts, next_index, next_hash)
					if f then
						return f
					end
				end
				if above_depth and next(child.children) ~= nil then
					table.insert(search_list, {child, node[2] + 1})
				end
			end
		until #search_list == 0
	end
	return nil
end

function M:search(branches, ref)
	U.type_assert(branches, "table", true)
	U.type_assert(ref, "string")

	local parts, root_ref = ref_parts(ref)
	if #parts == 0 then
		return root_ref and self or nil
	end

	local index = 1
	local hash = parts[index]
	if root_ref or not branches or #branches == 0 then
		return find_part(self, 0, parts, index, hash)
	end
	for _, branch in ipairs(branches) do
		local f = find_part(branch[1], branch[2], parts, index, hash)
		if f then
			return f
		end
	end
	return nil
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