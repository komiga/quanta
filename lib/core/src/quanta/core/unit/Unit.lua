u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Vessel = require "Quanta.Vessel"
local Measurement = require "Quanta.Measurement"
local Prop = require "Quanta.Prop"
local M = U.module(...)

M.element_name_pattern = "^[A-Z][0-9]+$"
M.step_id_pattern = "^RS[0-9]+$"

M.Type = {
	reference = 1,
	composition = 2,
	definition = 3,
	element = 4,
	step = 5,
}

M.DefinitionType = {
	{name = "none", notation = "U"},
	{name = "self", notation = "S"},
	{name = "meal", notation = "M"},
	{name = "familiar", notation = "FH"},
	{name = "familiar_joint", notation = "FHJ"}, -- FH + S
	{name = "foreign", notation = "FF" },
}
M.DefinitionTypeByNotation = {}

for i, t in ipairs(M.DefinitionType) do
	t.index = i
	M.DefinitionType[t.name] = i
	M.DefinitionType[t.notation] = i
	M.DefinitionTypeByNotation[t.notation] = i
end

M.ElementType = {
	{name = "generic", notation = "E"},
	{name = "primary", notation = "P"},
}

for _, t in ipairs(M.ElementType) do
	t.value = string.byte(t.notation)
	M.ElementType[t.name] = t.value
	M.ElementType[t.notation] = t.value
end

U.class(M)

function M:__init()
	self.type = M.Type.reference
	self.sub_type = M.DefinitionType.none
	self.seq_index = 0
	self.implicit = false
	self.name = nil
	self.name_hash = O.NAME_NULL
	self.id = nil
	self.id_hash = O.NAME_NULL
	self.id_arbitrary = false

	-- NB: can be shared!
	self.scope = nil
	-- Entity or Unit
	self.thing = nil
	self.thing_variant = nil

	self.description = Prop.Description.struct("")
	self.author = Prop.Author.struct({})
	self.note = Prop.Note.struct({})

	self.source = 0
	self.sub_source = 0
	self.source_certain = true
	self.sub_source_certain = true
	self.variant_certain = true
	self.presence_certain = true

	self.items = {}
	self.groups = {}

	self.modifiers = M.Modifier.struct_list({})
	self.measurements = Measurement.struct_list({})
end

function M.Reference()
	return M()
end

function M.Composition()
	local unit = M()
	unit.type = M.Type.composition
	return unit
end

function M.Definition()
	local unit = M()
	unit.type = M.Type.definition
	return unit
end

function M.Step(seq, implicit)
	U.type_assert_any(seq, {"number", "string"}, true)
	U.type_assert(implicit, "boolean", true)

	local unit = M()
	unit.type = M.Type.step
	unit.implicit = U.optional(implicit, false)
	if seq ~= nil then
		unit:set_seq(seq)
	end
	return unit
end

function M.Element(sub_type, seq_index, implicit)
	U.type_assert(sub_type, "number", true)
	U.type_assert(seq_index, "number", true)
	U.type_assert(implicit, "boolean", true)

	local unit = M()
	unit.type = M.Type.element
	unit.implicit = U.optional(implicit, false)
	unit.sub_type = U.optional(sub_type, M.ElementType.generic)
	if seq_index ~= nil then
		unit:set_seq(U.max(0, seq_index))
	end
	return unit
end

function M.ElementFromString(seq, implicit)
	U.type_assert(seq, "string")
	U.type_assert(implicit, "boolean", true)

	local unit = M()
	unit.type = M.Type.element
	unit.implicit = U.optional(implicit, false)
	unit:set_seq(seq)
	return unit
end

function M:is_empty()
	return not self.id and #self.items == 0
end

function M:make_copy()
	return U.make_empty_object(M):copy(self)
end

-- NB: doesn't actually copy common props for efficiency
function M:copy(unit)
	self.type = unit.type
	self.sub_type = unit.sub_type
	self.seq_index = unit.seq_index
	self.implicit = unit.implicit
	self.name = unit.name
	self.name_hash = unit.name_hash
	self.id = unit.id
	self.id_hash = unit.id_hash
	self.id_arbitrary = unit.id_arbitrary

	self.scope = unit.scope and T(unit.scope) or nil
	self.thing = unit.thing
	self.thing_variant = unit.thing_variant

	self.description = unit.description
	self.author = unit.author
	self.note = unit.note

	self.source = unit.source
	self.sub_source = unit.sub_source
	self.source_certain = unit.source_certain
	self.sub_source_certain = unit.sub_source_certain
	self.variant_certain = unit.variant_certain
	self.presence_certain = unit.presence_certain

	self.items = {}
	for _, item in ipairs(unit.items) do
		self:add(item:make_copy())
	end
	self:rebuild_groups()

	self.modifiers = M.Modifier.struct_list({})
	self.measurements = Measurement.struct_list({})
	for _, modifier in ipairs(unit.modifiers) do
		table.insert(self.modifiers, modifier:make_copy())
	end
	for _, measurement in ipairs(unit.measurements) do
		table.insert(self.measurements, measurement:make_copy())
	end

	return self
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

function M:set_id(id, arbitrary)
	U.type_assert(id, "string", true)
	U.type_assert(arbitrary, "boolean", true)

	if id and id ~= "" then
		-- utf8(¿) => C2 BF
		if string.sub(id, -2) == "¿" then
			id = string.sub(
				id, 1,
				string.sub(id, -3, -3) == "_" and -4 or -3
			)
			self.variant_certain = false
		else
			self.variant_certain = true
		end
		self.id = id
		self.id_hash = O.hash_name(self.id)
		self.id_arbitrary = arbitrary or false
	else
		self.id = nil
		self.id_hash = O.NAME_NULL
		self.id_arbitrary = false
	end
end

function M:set_seq(seq)
	if not U.is_type(seq, "string") then
		U.type_assert(seq, "number")
	elseif self.type == M.Type.element then
		U.assert(
			string.find(seq, M.element_name_pattern) ~= nil,
			"element name must be of the form '<capital group letter><index in group>'"
		)
		self.sub_type = string.byte(seq)
		seq = tonumber(string.sub(seq, 2))
	elseif self.type == M.Type.step then
		U.assert(
			string.find(seq, M.step_id_pattern) ~= nil,
			"step ID must be of the form 'RS<sequence index>'"
		)
		seq = tonumber(string.sub(seq, 3))
	else
		seq = tonumber(seq)
		U.assert(seq ~= nil, "failed to set sequence from string")
	end
	seq = U.max(0, seq)
	if self.seq_index == seq and (self.name_hash ~= O.NAME_NULL or self.id_hash ~= O.NAME_NULL) then
		return
	end
	self.seq_index = seq
	if self.seq_index == 0 then
		self:set_name(nil)
		self:set_id(nil)
	elseif self.type == M.Type.element then
		if self.sub_type == 0 then
			self.sub_type = M.ElementType.generic
		end
		self:set_name(string.char(self.sub_type) .. tostring(self.seq_index))
		--[[table.remove(parent:group(prev_sub_type), self.seq_index)
		parent:group(self.sub_type)[self.seq_index] = self--]]
	elseif self.type == M.Type.step then
		self:set_id("RS" .. tostring(self.seq_index))
	end
	return self.seq_index
end

function M:group(group_id)
	U.type_assert(group_id, "number")
	U.assert(group_id > 0, "group must be non-zero")
	local group = self.groups[group_id]
	if not group then
		group = {}
		self.groups[group_id] = group
	end
	return group
end

function M:rebuild_groups()
	self.groups = {}
	for _, item in ipairs(self.items) do
		if item.type == M.Type.element then
			self:group(item.sub_type)[item.seq_index] = item
		end
	end
end

function M:add(item)
	U.type_assert(item, M)
	table.insert(self.items, item)
	if item.type == M.Type.element and item.seq_index > 0 then
		self:group(item.sub_type)[item.seq_index] = item
	end
	if item.name_hash ~= O.NAME_NULL then
		self.items[item.name] = item
	end
	return item
end

local function unit_from_object(self, obj, implicit_scope, tree)
	U.type_assert(obj, "userdata")

	local context = Vessel.new_match_context(implicit_scope)
	if not context:consume(tree, obj, self) then
		return false, context.error:to_string()
	end
	return true
end

function M:from_object(obj, implicit_scope)
	return unit_from_object(self, obj, implicit_scope, M.t_branch_head)
end

function M:from_object_by_type(obj, implicit_scope)
	U.type_assert(obj, "userdata")

	return unit_from_object(self, obj, implicit_scope, M.t_head_by_type[self.type])
end

function M:to_object(obj, keep)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	elseif not keep then
		O.clear(obj)
	end

	if self.scope then
		O.set_time_date(obj, self.scope)
		obj = O.push_child(obj)
	end

	if self.name_hash ~= O.NAME_NULL then
		O.set_name(obj, self.name)
	end

	if self.type == M.Type.step then
		O.set_identifier(obj, self.id)
	elseif self.type == M.Type.definition then
		O.set_identifier(obj, M.DefinitionType[self.sub_type].notation)
	elseif self.type == M.Type.reference and self.id_hash ~= O.NAME_NULL then
		local id = self.id .. (self.variant_certain and "" or "¿")
		if self.id_arbitrary then
			O.set_string(obj, id)
		else
			O.set_identifier(obj, id)
		end
	else
		O.set_null(obj)
	end

	O.set_source(obj, self.source)
	O.set_sub_source(obj, self.sub_source)
	O.set_source_certain(obj, self.source_certain)
	O.set_sub_source_certain(obj, self.sub_source_certain)
	O.set_value_certain(obj, self.presence_certain)

	Prop.Description.struct_to_object(self.description, obj)
	Prop.Author.struct_to_object(self.author, obj)
	Prop.Note.struct_to_object(self.note, obj)
	for _, item in ipairs(self.items) do
		item:to_object(O.push_child(obj))
	end
	M.Modifier.struct_list_to_tags(self.modifiers, obj)
	Measurement.struct_list_to_quantity(self.measurements, obj)

	return obj
end

function M:is_resolved()
	if self.type == M.Type.reference and self.id ~= nil then
		return self.thing or self.id_arbitrary
	end
	return true
end

function M:resolve_refs(resolver)
	if
		self.type == M.Type.reference and
		self.thing == nil and
		self.id ~= nil and
		not self.id_arbitrary
	then
		self.thing, self.thing_variant = resolver:find_thing(self)
	end

	resolver:push(self)
	for _, item in ipairs(self.items) do
		item:resolve_refs(resolver)
	end
	resolver:pop()
end

M.Resolver = U.class(M.Resolver)

function M.Resolver:__init(select_searcher, scope_searcher, scope_searcher_data)
	U.type_assert(select_searcher, "function")
	U.type_assert(scope_searcher, "function", true)

	self.select_searcher = select_searcher
	self.scope_searcher = scope_searcher
	self.scope_searcher_data = scope_searcher_data
	self.stack = {}
end

function M.Resolver:push(part)
	U.assert(part ~= nil)
	local searcher = self.select_searcher(part)
	if searcher then
		table.insert(self.stack, {part = part, searcher = searcher})
	end
end

function M.Resolver:push_searcher(searcher)
	U.type_assert(searcher, "function")
	table.insert(self.stack, {searcher = searcher})
end

function M.Resolver:pop()
	table.remove(self.stack)
end

function M.Resolver:find_thing(unit)
	U.assert(unit ~= nil)

	local thing, variant
	if unit.scope and self.scope_searcher then
		thing, variant, _ = self.scope_searcher(self, self.scope_searcher_data, unit)
		if thing then
			return thing, variant
		end
	else
		local node, terminate
		local immediate_parent = nil
		for i = #self.stack, 1, -1 do
			node = self.stack[i]
			if not immediate_parent and node.part then
				immediate_parent = node.part
			end
			thing, variant, terminate = node.searcher(self, node.part, unit)
			if thing then
				return thing, variant
			end
			if terminate then
				break
			end
		end
	end
	if self.result then
		table.insert(self.result.not_found, {unit = unit, parent = immediate_parent})
	end
	return nil, nil
end

function M.Resolver:do_tree(part)
	U.assert(part ~= nil)
	U.assert(self.result == nil)

	local stack_size = #self.stack
	local result = {
		not_found = {},
	}
	self.result = result

	part.resolve_refs(part, self)

	self.result = nil
	U.assert(stack_size == #self.stack)

	return result
end

function M.Resolver.searcher_universe(universe, branches, handler)
	U.assert(universe ~= nil)
	U.type_assert(branches, "table", true)
	U.type_assert(handler, "function", true)
	return function(resolver, _, unit)
		local entity = universe:search(branches, unit.id, handler)
		if entity then
			return entity, entity:variant(unit.source, unit.sub_source)
		end
		return nil, nil, false
	end
end

function M.Resolver.searcher_unit_child_func(resolver, search_in, unit)
	if unit.source == 0 then
		return search_in.items[unit.id], nil, false
	end
	return nil, nil, false
end

function M.Resolver.searcher_unit_child(search_in)
	return function(resolver, _, unit)
		return M.Resolver.searcher_unit_child_func(resolver, search_in, unit)
	end
end

function M.Resolver.searcher_unit_selector(search_in, no_terminate)
	return function(resolver, _, unit)
		local thing = search_in.thing
		if thing then
			if U.is_type(thing, M) then
				return thing.items[unit.id], nil, not no_terminate
			elseif thing.children then
				return thing.children[unit.id_hash], nil, not no_terminate
			end
		end
		return nil, nil, false
	end
end

function M.Resolver.searcher_unit_sub_auto(search_in, selector_no_terminate)
	if search_in.type ~= M.Type.reference then
		return M.Resolver.searcher_unit_child(search_in)
	else
		return M.Resolver.searcher_unit_selector(search_in, selector_no_terminate)
	end
end

M.Resolver.select_searcher_default = M.Resolver.searcher_unit_sub_auto

M.Modifier = U.class(M.Modifier)

function M.Modifier:__init(id, id_hash, data)
	U.type_assert(id, "string", true)
	U.type_assert(id_hash, "number", true)

	self.id = id
	self.id_hash = id_hash or (id and O.hash_name(id) or O.NAME_NULL)
	self.data = data
end

function M.Modifier:make_copy()
	return U.make_empty_object(M.Modifier):copy(self)
end

function M.Modifier:copy(modifier)
	self.id = modifier.id
	self.id_hash = modifier.id_hash
	if modifier.data then
		self.data = modifier.data:make_copy()
	end
	return self
end

function M.Modifier:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	end

	O.set_name(obj, self.id)
	if self.data then
		self.data:to_object(self, obj)
	end
end

function M.Modifier.struct_list(list)
	U.type_assert(list, "table")
	return list
end

function M.Modifier.struct_list_to_tags(list, obj)
	for _, m in pairs(list) do
		m:to_object(O.push_tag(obj))
	end
end

function M.Modifier.adapt_struct_list()
	local p_element = Match.Pattern{
		name = true,
		children = Match.Any,
		acceptor = function(context, parent, obj)
			local modifier = M.Modifier(O.name(obj), O.name_hash(obj))
			table.insert(parent.modifiers, modifier)

			return context.user.director:read_modifier(context, parent, modifier, obj)
		end,
	}

	local t_head = Match.Tree({
	-- [..., ...]
	Match.Pattern{
		children = {
			p_element,
		},
	},
	-- [...]
	p_element,
	})
	t_head:build()

	return M.Modifier.struct_list_to_tags, t_head
end

_, M.Modifier.t_struct_list_head = M.Modifier.adapt_struct_list()

M.UnknownModifier = U.class(M.UnknownModifier)

function M.UnknownModifier:__init(obj)
	U.type_assert(obj, "userdata", true)

	self.obj = obj or O.create()
end

function M.UnknownModifier:make_copy()
	return M.UnknownModifier():copy(self)
end

function M.UnknownModifier:copy(unknown_modifier)
	O.copy_children(self.obj, unknown_modifier.obj)
	return self
end

function M.UnknownModifier:from_object(context, ref, modifier, obj)
	O.copy_children(self.obj, obj)
end

function M.UnknownModifier:to_object(modifier, obj)
	O.copy_children(obj, self.obj)
end

function M.UnknownModifier:compare_equal(other)
	-- TODO?
	return true
end

local common_props = {
	Prop.Description.t_struct_head,
	Prop.Author.t_struct_head,
	Prop.Note.t_struct_head,
}

local function child_post_branch(context, self, obj)
	local parent = context:value(1)
	if self.name_hash ~= O.NAME_NULL then
		if parent.items[self.name] then
			return Match.Error("name '%s' is not unique in this scope", self.name)
		end
	end
	parent:add(self)
end

local function translate_basic(context, self, obj)
	self:set_name(O.name(obj))
	self.source = O.source(obj)
	self.sub_source = O.sub_source(obj)
	self.source_certain = not O.marker_source_uncertain(obj)
	self.sub_source_certain = not O.marker_sub_source_uncertain(obj)
	self.presence_certain = not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))
end

M.t_reference_head = Match.Tree()
M.t_reference_body = Match.Tree()

M.t_composition_head = Match.Tree()
M.t_composition_head_gobble = Match.Tree()
M.t_composition_body = Match.Tree()

M.t_definition_head = Match.Tree()
M.t_definition_body = Match.Tree()
M.t_definition_element_body = Match.Tree()

-- type{...}
-- UNIT = type{...}
M.p_definition_head = Match.Pattern{
	name = Match.Any,
	vtype = O.Type.identifier,
	value = function(_, _, obj, _)
		return nil ~= M.DefinitionTypeByNotation[O.identifier(obj)]
	end,
	children = M.t_definition_body,
	tags = M.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	acceptor = function(context, self, obj)
		self.type = M.Type.definition
		self.sub_type = M.DefinitionTypeByNotation[O.identifier(obj)]
		translate_basic(context, self, obj)
	end,
}

-- x, x:m, x{...}
M.p_reference_head_id = Match.Pattern{
	name = Match.Any,
	vtype = {O.Type.identifier, O.Type.string},
	children = M.t_reference_body,
	tags = M.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	acceptor = function(context, self, obj)
		self.type = M.Type.reference
		translate_basic(context, self, obj)
		if O.is_identifier(obj) then
			self:set_id(O.identifier(obj))
		else
			self:set_id(O.string(obj), true)
		end
		if #context.user.scope > 0 then
			self.scope = U.table_last(context.user.scope)
		end
	end,
}

-- ?, :m
M.p_reference_head_empty = Match.Pattern{
	name = Match.Any,
	vtype = O.Type.null,
	tags = M.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	acceptor = function(context, self, obj)
		self.type = M.Type.reference
		translate_basic(context, self, obj)
	end,
}

M.t_reference_head:add({
M.p_reference_head_id,
M.p_reference_head_empty,
})

M.t_reference_body:add({
common_props,
Match.Pattern{
	any = true,
	branch = M.t_reference_head,
	acceptor = function(context, self, obj)
		table.insert(context.user.scope_save, context.user.scope)
		context.user.scope = {}
		return M.Reference()
	end,
	post_branch_pre = child_post_branch,
	post_branch = function(context, self, obj)
		context.user.scope = table.remove(context.user.scope_save)
	end,
},
})

M.t_reference_body:build()
M.t_reference_head:build()

local function ref_acceptor(context, self, obj)
	return M.Reference()
end

M.p_composition_items = {
-- sub def
Match.Pattern{
	layer = M.p_definition_head,
	acceptor = function(context, self, obj)
		return M.Definition()
	end,
	post_branch_pre = child_post_branch,
},
-- sub ref
Match.Pattern{
	layer = M.p_reference_head_id,
	acceptor = ref_acceptor,
	post_branch_pre = child_post_branch,
},
Match.Pattern{
	layer = M.p_reference_head_empty,
	acceptor = ref_acceptor,
	post_branch_pre = child_post_branch,
},
}

M.p_composition_body_typed = Match.Pattern{
	any = true,
	branch = M.t_composition_body,
	post_branch = function(context, self, obj)
		self.type = M.Type.composition
	end
}

-- time context
M.t_composition_head:add({
Match.Pattern{
	vtype = O.Type.time,
	tags = false,
	children = M.t_composition_body,
	quantity = false,
	acceptor = function(context, self, obj)
		self.type = M.Type.composition
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
	children = M.t_composition_body,
	tags = M.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	func = function(_, _, obj)
		return O.has_children(obj)
	end,
	acceptor = function(context, self, obj)
		self.type = M.Type.composition
		translate_basic(context, self, obj)
	end,
},
})

M.t_composition_head_gobble:add({
	M.t_composition_head,
	M.p_composition_items,
})

M.t_composition_body:add({
-- sub items
M.p_composition_items,
-- sub composition
Match.Pattern{
	any = true,
	branch = M.t_composition_head,
	acceptor = function(context, self, obj)
		return M.Composition()
	end,
	post_branch_pre = child_post_branch,
},
})

M.t_composition_body:build()
M.t_composition_head:build()
M.t_composition_head_gobble:build()

M.t_definition_head:add(M.p_definition_head)

M.t_definition_body:add(common_props)

local function element_name_filter(_, _, obj, _)
	return (
		O.is_named(obj) and
		string.find(O.name(obj), M.element_name_pattern) ~= nil
	)
end

local function element_acceptor(element, _, unit, _)
	local group = unit:group(element.sub_type)
	local current = group[element.seq_index]
	if element.seq_index <= 0 then
		return Match.Error("element %s index must be greater than 0", element.name)
	elseif current then
		if current.implicit then
			return Match.Error(
				"cannot mix implicit and explicit elements (at explicit element %s)",
				element.name
			)
		else
			return Match.Error("element %s already exists", element.name)
		end
	elseif element.seq_index ~= #group + 1 then
		return Match.Error("element %s is not sequentially ordered", element.name)
	end
	unit:add(element)
end

local function element_post_branch(context, element, obj)
	if #element.items == 0 then
		return Match.Error("no steps specified for element %s", element.name)
	end
	if not element.implicit and O.has_quantity(obj) then
		local last_step = U.table_last(element.items)
		if #last_step.measurements > 0 then
			return Match.Error(
				"element %s carries final measurement, but its last step (RS%s) already has a measurement",
				element.name, last_step.seq_index
			)
		end
		if not context:consume(Measurement.t_struct_list_head, O.quantity(obj), last_step) then
			return
		end
	end
end

-- Element
M.t_definition_body:add({
-- = {...}
Match.Pattern{
	name = element_name_filter,
	vtype = O.Type.null,
	children = M.t_definition_element_body,
	quantity = Match.Any,
	acceptor = function(context, unit, obj)
		local element = M.ElementFromString(O.name(obj))
		return element_acceptor(element, context, unit, obj) or element
	end,
	post_branch_pre = element_post_branch,
},
-- = ... (prototype shorthand)
Match.Pattern{
	name = element_name_filter,
	vtype = {O.Type.identifier, O.Type.string},
	children = Match.Any,
	tags = Match.Any,
	quantity = Match.Any,
	branch = M.t_reference_head,
	acceptor = function(context, unit, obj)
		local element = M.ElementFromString(O.name(obj))
		local err = element_acceptor(element, context, unit, obj)
		if err then
			return err
		end
		local step = element:add(M.Step(1, true))
		return step:add(M.Reference())
	end,
	post_branch_pre = function(_, ref, _)
		ref:set_name("prototype")
	end,
	-- post_branch_pre = element_post_branch,
},
-- implicit: P1
Match.Pattern{
	any = true,
	branch = M.t_definition_element_body,
	acceptor = function(_, unit, obj)
		local group = unit:group(M.ElementType.primary)
		local element = group[1]
		if not element then
			element = unit:add(M.Element(M.ElementType.primary, 1, true))
		elseif not element.implicit or #group > 1 then
			return Match.Error("sub-property defined in unit root after explicit element")
		end
		return element
	end,
	post_branch_pre = element_post_branch,
},
})

M.t_definition_element_body:add(common_props)

local function step_post_branch(context, step, obj)
	if #step.items == 0 then
		return Match.Error("no items specified for step")
	end
	if O.has_quantity(obj) then
		if not context:consume(Measurement.t_struct_list_head, O.quantity(obj), step) then
			return
		end
	end
end

-- Step
M.t_definition_element_body:add({
-- RS#{...}
Match.Pattern{
	vtype = O.Type.identifier,
	value = function(_, _, obj, _)
		return string.find(O.identifier(obj), M.step_id_pattern) ~= nil
	end,
	children = M.t_composition_body,
	quantity = Match.Any,
	acceptor = function(_, element, obj)
		local step = M.Step(O.identifier(obj))
		local current = element.items[step.seq_index]
		if step.seq_index <= 0 then
			return Match.Error("step RS%d index must be greater than 0", step.seq_index)
		elseif current then
			if element.items[1].implicit then
				return Match.Error("cannot mix implicit and explicit steps (at explicit step RS%d)", step.seq_index)
			else
				return Match.Error("step RS%d was already specified", step.seq_index)
			end
		elseif step.seq_index ~= #element.items + 1 then
			return Match.Error("step RS%d is not sequentially ordered", step.seq_index)
		end
		element:add(step)
		return step
	end,
	post_branch_pre = step_post_branch,
},
-- implicit: RS1
Match.Pattern{
	any = true,
	branch = M.t_composition_body,
	acceptor = function(_, element, obj)
		local step = element.items[1]
		if not step then
			step = element:add(M.Step(1, true))
		elseif not step.implicit or #element.items > 1 then
			return Match.Error("sub-property defined in element root after explicit step")
		end
		return step
	end,
	post_branch_pre = step_post_branch,
},
})

M.t_definition_element_body:build()
M.t_definition_body:build()
M.t_definition_head:build()

M.t_branch_head = Match.Tree()

M.t_branch_head:add({
	M.t_definition_head,
	M.t_composition_head,
	M.t_reference_head,
})

M.t_branch_head:build()

M.t_head_by_type = {
	M.t_reference_head,
	M.t_composition_head_gobble,
	M.t_definition_head,
}

return M

)"__RAW_STRING__"