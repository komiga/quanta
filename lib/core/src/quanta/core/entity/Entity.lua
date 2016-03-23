u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Vessel = require "Quanta.Vessel"
local Match = require "Quanta.Match"
local Measurement = require "Quanta.Measurement"
local Prop = require "Quanta.Prop"
local Unit = require "Quanta.Unit"
local M = U.module(...)

M.Type = {
	thing = 1,
	category = 2,
	universe = 3,
}

U.class(M)

function M:__init(name, id, id_hash, class)
	U.type_assert(id, "string", true)
	U.type_assert(id_hash, "number", true)

	self.type = M.Type.thing
	self.name = nil
	self.name_hash = O.NAME_NULL

	self.id = id
	self.id_hash = id_hash or O.NAME_NULL
	self.data = nil

	self.universe = nil
	self.parent = nil
	self.children = {}

	self:set_name(name)
	if class then
		self.data = class(self)
	else
		self.data = nil
	end
	self.generic = M.Source(self)
end

function M:is_thing()
	return self.type == M.Type.thing
end

function M:is_category()
	return self.type == M.Type.category
end

function M:is_universe()
	return self.type == M.Type.universe
end

function M:is_specialized()
	return self.id_hash ~= O.NAME_NULL
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

function M:any_sources()
	return self.generic:any_sources()
end

function M:has_variant(lead, sub)
	if lead == 0 then
		return true
	end
	local s = self.generic.sources[lead]
	if s then
		return sub == 0 or s.sources[sub] ~= nil
	end
	return false
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
	return e
end)

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

local function find_part(handler, entity, search_depth, parts, index, hash)
	if index > #parts then
		if handler then
			return handler(entity) and entity or nil
		else
			return entity
		end
	end
	local next_index = index + 1
	local next_hash = parts[next_index]
	local child = entity.children[hash]
	if child then
		local f = find_part(handler, child, 0, parts, next_index, next_hash)
		if f then
			return f
		end
	end

	if search_depth > 0 and next(entity.children) ~= nil then
		-- BFS
		local search_list = {}
		for _, child in pairs(entity.children) do
			table.insert(search_list, {child, 1})
		end
		repeat
			local node = table.remove(search_list)
			local above_depth = node[2] <= search_depth
			for child_hash, child in pairs(node[1].children) do
				if child_hash == hash then
					local f = find_part(handler, child, 0, parts, next_index, next_hash)
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

function M:search(branches, ref, handler)
	U.type_assert(branches, "table", true)
	U.type_assert(ref, "string")
	U.type_assert(handler, "function", true)

	local parts, root_ref = ref_parts(ref)
	if #parts == 0 then
		return root_ref and self or nil
	end

	local index = 1
	local hash = parts[index]
	if root_ref or not branches or #branches == 0 then
		return find_part(handler, self, 0, parts, index, hash)
	end
	for _, branch in ipairs(branches) do
		local f = find_part(handler, branch[1], branch[2], parts, index, hash)
		if f then
			return f
		end
	end
	return nil
end

local generic_id_by_type = {
	"Generic",
	"GenericCategory",
}

function M:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	end

	if self.type == M.Type.universe then
		for _, entity in pairs(self.children) do
			entity:to_object(O.push_child(obj))
		end
		return obj
	end

	if self.name_hash ~= O.NAME_NULL then
		O.set_name(obj, self.name)
	else
		O.set_name(obj, "NO_NAME")
	end

	if self.id_hash ~= O.NAME_NULL then
		O.set_identifier(obj, self.id)
	else
		O.set_identifier(obj, generic_id_by_type[self.type])
	end

	self.generic:to_object(obj)

	if self.data then
		self.data:to_object(obj)
	end

	if next(self.children) ~= nil then
		local children_obj = O.push_child(obj)
		O.set_name(children_obj, "children")
		for _, entity in pairs(self.children) do
			entity:to_object(O.push_child(children_obj))
		end
	end

	return obj
end

M.Source = U.class(M.Source)

function M.Source:__init(parent)
	U.assert(not parent or U.is_instance(parent))

	self.description = Prop.Description.struct("")
	self.label = Prop.Description.struct("")
	self.author = Prop.Author.struct({})
	self.vendor = Prop.Author.struct({})
	self.note = Prop.Note.struct({})
	self.composition = Unit.Composition()
	self.sources = {}
	self.data = nil

	self.parent = parent
	if self.parent and self.parent.data then
		self.data = self.parent.data.Source(self)
	end
end

function M.Source:any_sources()
	return #self.sources > 0
end

function M.Source:has_author()
	return #self.author > 0
end

function M.Source:has_vendor()
	return #self.vendor > 0
end

local function parent_source_value(name)
	local function value(context, n)
		local p = context:value(n)
		local v
		if U.is_instance(p, M) then
			v = p.parent.generic[name]
		elseif U.is_instance(p, M.Source) then
			v = p[name]
		else
			v = nil
		end

		if v == "" then
			v = nil
		end
		return v
	end

	return function(context)
		return value(context, 1) or value(context, 2)
	end
end

local source_description_to_object, t_source_description = Prop.Description.adapt_struct(
	"d", "description",
	parent_source_value("description")
)
local source_label_to_object, t_source_label = Prop.Description.adapt_struct(
	"label", "label",
	parent_source_value("label")
)
local source_vendor_to_object, t_source_vendor = Prop.Author.adapt_struct(
	"vendor", "vendor",
	parent_source_value("vendor")
)

function M.Source:to_object(obj, depth)
	U.type_assert(obj, "userdata", true)
	depth = U.optional(U.type_assert(depth, "number", true), 0)

	if not obj then
		obj = O.create()
	end

	source_description_to_object(self.description, obj)
	source_label_to_object(self.label, obj)
	Prop.Author.struct_to_object(self.author, obj)
	source_vendor_to_object(self.vendor, obj)
	Prop.Note.struct_to_object(self.note, obj)

	if #self.composition.items > 0 then
		local composition_obj = O.push_child(obj)
		self.composition:to_object(composition_obj)
		O.set_name(composition_obj, "composition")
	end

	if self.data then
		self.data:to_object(obj)
	end

	if next(self.sources) ~= nil then
		local sources_obj = O.push_child(obj)
		O.set_name(sources_obj, depth == 0 and "sources" or "sub_sources")
		for i, source in pairs(self.sources) do
			local source_obj = O.push_child(sources_obj)
			O.set_integer(source_obj, i)
			source:to_object(source_obj, depth + 1)
		end
	end

	return obj
end

function M.specialize_generic_fallthrough(source_body)
	return Match.Pattern{
		any = true,
		branch = source_body,
		acceptor = function(_, e, obj)
			return e.generic
		end,
	}
end

function M.specialize_source_head(source_body, get_lead)
	U.assert(source_body)
	return Match.Pattern{
		vtype = O.Type.integer,
		children = source_body,
		acceptor = function(_, p, obj)
			local i = O.integer(obj)
			if i < 1 then
				return Match.Error("source %2d index must be greater than 0", i)
			elseif get_lead(p).sources[i] then
				return Match.Error("source %2d is not unique", i)
			end
			local sub = M.Source(p)
			get_lead(p).sources[i] = sub
			return sub
		end
	}
end

function M.specialize_sub_sources(source_body)
	return Match.Pattern{
		name = "sub_sources",
		children = {M.specialize_source_head(
			source_body,
			function(lead)
				return lead
			end
		)},
		acceptor = function(_, lead, obj)
			if lead:any_sources() then
				return Match.Error("sub-sources already defined")
			end
		end,
	}
end

function M.specialize_sources(source_body)
	return Match.Pattern{
		name = "sources",
		children = {M.specialize_source_head(
			source_body,
			function(e)
				return e.generic
			end
		)},
		acceptor = function(_, e, obj)
			if e:any_sources() then
				return Match.Error("entity sources already defined")
			end
		end,
	}
end

M.Source.t_body_generic = Match.Tree()
M.Source.t_body = Match.Tree()

M.Source.t_body:add({
t_source_description,
t_source_label,

Prop.Author.t_struct_head,
t_source_vendor,
Prop.Note.t_struct_head,

Match.Pattern{
	name = "composition",
	vtype = Match.Any,
	children = Match.Any,
	tags = Match.Any,
	quantity = Match.Any,
	branch = Unit.t_composition_head_gobble,
	acceptor = function(context, self, obj)
		return self.composition
	end,
},

-- TODO
Match.Pattern{
	name = "base_model",
	vtype = {O.Type.null, O.Type.string},
	tags = Match.Any,
},
Match.Pattern{
	name = "model",
	vtype = {O.Type.null, O.Type.string},
	tags = Match.Any,
},

Match.Pattern{
	name = "state",
	vtype = O.Type.string,
},
Match.Pattern{
	name = "state",
	children = {
		Match.Pattern{vtype = O.Type.string}
	},
},
Match.Pattern{
	name = "config",
	children = true,
},

-- TODO
Match.Pattern{
	name = "properties",
	children = true,
},
})

M.Source.t_body_generic:add({
	M.Source.t_body,
	M.specialize_sub_sources(M.Source.t_body),
})

M.Source.t_body:build()
M.Source.t_body_generic:build()

M.t_root = Match.Tree()

M.t_shared_body = Match.Tree({
Match.Pattern{
	name = "aliases",
	children = {Match.Pattern{
		children = true,
		acceptor = function(_, cat, obj)
			if O.num_children(obj) < 2 then
				return Match.Error("alias definition must have at least one source")
			end
			-- TODO
		end,
	}},
},
Match.Pattern{
	name = "children",
	children = M.t_root,
},
})

M.t_entity_body = Match.Tree()
M.t_entity_body_generic = Match.Tree()

M.t_entity_head = Match.Tree({
Match.Pattern{
	name = true,
	vtype = O.Type.identifier,
	value = "Generic",
	children = M.t_entity_body_generic,
	acceptor = function(_, parent, obj)
		return parent:add(M(O.name(obj)))
	end
},
})

M.t_entity_body:add({
	M.t_shared_body,
})

M.t_entity_body_generic:add({
	M.t_entity_body,
	M.specialize_sources(M.Source.t_body_generic),
	M.specialize_generic_fallthrough(M.Source.t_body_generic),
})

M.t_category_body = Match.Tree()
M.t_category_body_generic = Match.Tree()

M.t_category_head = Match.Tree({
Match.Pattern{
	name = true,
	vtype = O.Type.identifier,
	value = "GenericCategory",
	children = M.t_category_body_generic,
	acceptor = function(_, parent, obj)
		return parent:add(M.Category(O.name(obj)))
	end
},
})

M.p_include = Match.Pattern{
	name = "include",
	collect = {Match.Pattern{
		vtype = O.Type.string,
		acceptor = function(_, _, obj)
			return O.string(obj)
		end
	}},
	collect_post = function(context, cat, obj, collection)
		local sub = O.create()
		for _, path in ipairs(collection) do
			-- TODO: expand pattern-match path against filesystem
			path = Vessel.data_path("entity/" .. path)
			if not O.read_text_file(sub, path) then
				return Match.Error("failed to load include file: %s", path)
			end
			if not context:consume_sub(M.t_root, sub, cat, path) then
				return false
			end
		end
		return true
	end
}

M.t_category_body:add({
	M.t_shared_body,
	M.p_include,
})

M.t_category_body_generic:add({
	M.t_category_body,
	M.specialize_generic_fallthrough(M.Source.t_body_generic),
})

M.p_specialization = Match.Pattern{
	name = true,
	vtype = O.Type.identifier,
	tags = Match.Any,
	children = Match.Any,
	acceptor = function(context, parent, obj)
		local id = O.identifier(obj)
		local id_hash = O.identifier_hash(obj)
		local class = context.user.director:find_entity_class(id, id_hash)
		if not class then
			return Match.Error("entity specialization not found: %s", id)
		end

		local entity = parent:add(M(O.name(obj), id, id_hash, class))
		return entity.data:from_object(context, parent, entity, obj)
	end,
}

M.t_root:add({
	M.t_entity_head,
	M.t_category_head,
	M.p_specialization,
	M.p_include,
})

M.t_entity_body_generic:build()
M.t_category_body_generic:build()
M.t_root:build()

function M.read_universe(rp, name)
	U.type_assert(name, "string", true)

	local path, root
	if U.is_type(rp, "string") then
		path = rp
		root = O.create()
		if not O.read_text_file(root, path) then
			U.log("error: failed to read root")
			return nil
		end
	else
		U.type_assert(rp, "userdata")
		root = rp
	end

	local universe = M.Universe(name or "universe")
	local context = Vessel.new_match_context(nil)
	if context:consume_sub(M.t_root, root, universe, path) then
		return universe
	end
	return nil, context.error:to_string()
end

return M

)"__RAW_STRING__"