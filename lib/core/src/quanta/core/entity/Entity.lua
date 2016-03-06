u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Vessel = require "Quanta.Vessel"
local Match = require "Quanta.Match"
local Measurement = require "Quanta.Measurement"
local Prop = require "Quanta.Prop"
local Composition = require "Quanta.Composition"
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

	self.sources = {}
	self.universe = nil
	self.parent = nil
	self.children = {}

	self:set_name(name)
	if class then
		self.data = class(self)
	else
		self.data = nil
	end
	self.sources[0] = M.Source(self)
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

function M:has_variant(source, sub_source)
	if source == 0 then
		return true
	end
	local s = self.sources[source]
	if s then
		return sub_source == 0 or s.vendor[sub_source] ~= nil
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

M.Source = U.class(M.Source)

function M.Source:__init(entity)
	self.description = Prop.Description.struct("")
	self.label = Prop.Description.struct("")
	self.author = Prop.Author.struct({})
	self.vendor = Prop.Author.struct({})
	self.note = Prop.Note.struct({})
	self.composition = Composition()
	self.data = nil

	if entity.data then
		self.data = entity.data.Source(self)
	end
end

function M.Source:has_author()
	return #self.author > 0
end

function M.Source:set_author(i, author)
	U.type_assert(i, "number")
	U.type_assert(author, Prop.Author)
	U.assert(i > 0)
	self.author[i] = author
	return author
end

function M.Source:add_author(author)
	U.type_assert(author, Prop.Author)
	table.insert(self.author, author)
	return author
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
	return self.vendor[0] ~= nil or #self.vendor > 0
end

function M.Source:set_vendor(i, vendor)
	U.type_assert(i, "number")
	U.type_assert(vendor, Prop.Author)
	self.vendor[i] = vendor
	return vendor
end

function M.Source:add_vendor(vendor)
	U.type_assert(vendor, Prop.Author)
	table.insert(self.vendor, vendor)
	return vendor
end

local function parent_source_value(name)
	return function(context)
		local parent_entity = context:value(1).parent
		return parent_entity and parent_entity.sources[0][name]
	end
end

M.Source.t_body = Match.Tree()

function M.specialize_source_fallthrough(body)
	return Match.Pattern{
		any = true,
		branch = body,
		acceptor = function(_, e, obj)
			return e.sources[0]
		end,
	}
end

function M.specialize_sources(body)
	return Match.Pattern{
		name = "sources",
		children = {Match.Pattern{
			children = body,
			acceptor = function(_, e, obj)
				return e:add_source(M.Source(e))
			end
		}},
		acceptor = function(_, e, obj)
			if e:any_sources() then
				return Match.Error("entity source(s) already defined")
			end
		end
	}
end

local _, t_source_description = Prop.Description.adapt_struct(
	"d", "description",
	parent_source_value("description")
)
local _, t_source_label = Prop.Description.adapt_struct(
	"label", "label",
	parent_source_value("label")
)

M.Source.t_body:add({
Prop.Note.t_struct_head,
Prop.Author.t_struct_head,
t_source_description,
t_source_label,
Match.Pattern{
	name = "vendor",
	vtype = {O.Type.null, O.Type.string},
	tags = Prop.Author.t_head_tags,
	acceptor = function(_, self, obj)
		return self:set_vendor(0, Prop.Author(
			O.is_string(obj) and O.string(obj) or nil,
			O.value_certain(obj),
			nil, true
		))
	end
},
Match.Pattern{
	name = "vendors",
	children = {Match.Pattern{
		vtype = {O.Type.null, O.Type.string},
		tags = Prop.Author.t_head_tags,
		acceptor = function(_, self, obj)
			return self:add_vendor(Prop.Author(
				O.is_string(obj) and O.string(obj) or nil,
				O.value_certain(obj),
				nil, true
			))
		end
	}},
},

Match.Pattern{
	name = "composition",
	vtype = Match.Any,
	children = Match.Any,
	tags = Match.Any,
	quantity = Match.Any,
	branch = Composition.t_body,
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
M.specialize_sources(M.Source.t_body),
M.specialize_source_fallthrough(M.Source.t_body),
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

M.t_category_body:add({
M.t_shared_body,
Match.Pattern{
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
			if not context:consume_sub(context:control(), sub, cat, path) then
				return false
			end
		end
		return true
	end
},
})

M.t_category_body_generic:add({
M.t_category_body,
M.specialize_source_fallthrough(M.Source.t_body),
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
})

M.Source.t_body:build()
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