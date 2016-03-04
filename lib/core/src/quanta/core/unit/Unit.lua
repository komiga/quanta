u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Vessel = require "Quanta.Vessel"
local Measurement = require "Quanta.Measurement"
local Prop = require "Quanta.Prop"
local Instance = require "Quanta.Instance"
local M = U.module(...)

M.Type = {
	{name = "none", notation = "U"},
	{name = "self", notation = "S"},
	{name = "meal", notation = "M"},
	{name = "familiar", notation = "FH"},
	{name = "familiar_joint", notation = "FHJ"}, -- FH + S
	{name = "foreign", notation = "FF" },
}
M.TypeByNotation = {}

for i, t in ipairs(M.Type) do
	t.index = i
	M.Type[t.name] = i
	M.Type[t.notation] = i
	M.TypeByNotation[t.notation] = i
end

U.class(M)

function M:__init()
	self.type = M.Type.none
	self.name = nil
	self.name_hash = O.NAME_NULL
	self.description = Prop.Description.struct("")
	self.author = Prop.Author.struct({})
	self.note = Prop.Note.struct({})
	self.elements = {
		{}, -- M.Element.Type.generic
		{}, -- M.Element.Type.primary
	}

	self.modifiers = Instance.Modifier.struct_list({})
	self.measurements = Measurement.struct_list({})
end

function M:element_generic(index)
	U.type_assert(index, "number")
	return self.elements[M.Element.Type.generic][index]
end

function M:element_primary(index)
	U.type_assert(index, "number")
	return self.elements[M.Element.Type.primary][index]
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

function M:from_object(obj, implicit_scope)
	U.type_assert(obj, "userdata")

	local context = Vessel.new_match_context(implicit_scope)
	if not context:consume(M.t_head, obj, self) then
		return false, context.error:to_string()
	end
	return true
end

local function to_object_shared(self, obj)
	Prop.Description.struct_to_object(self.description, obj)
	Prop.Author.struct_to_object(self.author, obj)
	Prop.Note.struct_to_object(self.note, obj)
end

function M:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	else
		O.clear(obj)
		O.release_quantity(obj)
	end

	if self.name_hash ~= O.NAME_NULL then
		O.set_name(obj, self.name)
	else
		O.clear_name(obj)
	end
	O.set_identifier(obj, M.Type[self.type].notation)

	to_object_shared(self, obj)
	for _, bucket in ipairs(self.elements) do
		for _, e in ipairs(bucket) do
			e:to_object(O.push_child(obj))
		end
	end
	Instance.Modifier.struct_list_to_tags(self.modifiers, obj)
	Measurement.struct_list_to_quantity(self.measurements, obj)

	return obj
end

M.Step = U.class(M.Step)

function M.Step:__init()
	self.index = 1
	self.implicit = false
	self.composition = Quanta.Composition()
end

function M.Step:to_object(obj)
	U.type_assert(obj, "userdata")

	O.set_identifier(obj, "RS" .. tostring(self.index))
	self.composition:to_object(obj, true)
end

M.Element = U.class(M.Element)

M.Element.Type = {
	{name = "generic", notation = "E"},
	{name = "primary", notation = "P"},
}

for i, t in ipairs(M.Element.Type) do
	t.index = i
	M.Element.Type[t.name] = i
	M.Element.Type[t.notation] = i
end

function M.Element:__init()
	self.type = M.Element.Type.generic
	self.index = 1
	self.implicit = false
	self.description = Prop.Description.struct("")
	self.author = Prop.Author.struct({})
	self.note = Prop.Note.struct({})
	self.steps = {}
end

function M.Element:to_object(obj)
	U.type_assert(obj, "userdata")

	O.set_name(obj, M.Element.Type[self.type].notation .. tostring(self.index))
	to_object_shared(self, obj)
	for _, s in ipairs(self.steps) do
		s:to_object(O.push_child(obj))
	end
end

M.t_head = Match.Tree()
M.t_body = Match.Tree()
M.t_element_body = Match.Tree()

local shared_props = {
	Prop.Description.t_struct_head,
	Prop.Author.t_struct_head,
	Prop.Note.t_struct_head,
}

-- type{...}
-- UNIT = type{...}
M.p_head = Match.Pattern{
	name = Match.Any,
	vtype = O.Type.identifier,
	value = function(context, unit, obj, _)
		return nil ~= M.TypeByNotation[O.identifier(obj)]
	end,
	children = M.t_body,
	tags = Instance.Modifier.t_struct_list_head,
	quantity = Measurement.t_struct_list_head,
	acceptor = function(_, unit, obj)
		unit.type = M.TypeByNotation[O.identifier(obj)]
		unit:set_name(O.name(obj))
	end,
	--[[post_branch = function(_, unit, _)
		local primary_bucket = unit.elements[M.Element.Type.primary]
		if #primary_bucket == 0 then
			return Match.Error("no primary elements specified for unit")
		end
	end,--]]
}
M.t_head:add(M.p_head)

M.t_body:add(shared_props)

local function element_post_branch(_, element, _)
	if #element.steps == 0 then
		return Match.Error("no steps specified for element")
	end
end

local function element_name_filter(_, _, obj, _)
	return (
		O.is_named(obj) and
		string.find(O.name(obj), "^[EP][0-9]+$") ~= nil
	)
end

local function element_acceptor(element, _, unit, obj)
	local name = O.name(obj)
	element.type = M.Element.Type[string.sub(name, 1, 1)]
	U.assert(element.type ~= nil)
	element.index = tonumber(string.sub(name, 2))

	local bucket = unit.elements[element.type]
	local current = bucket[element.index]
	if element.index <= 0 then
		return Match.Error(
			"element %s%d index must be greater than 0",
			M.Element.Type[element.type].notation, element.index
		)
	elseif current then
		if current.implicit then
			return Match.Error(
				"cannot mix implicit and explicit elements (at explicit element %s%d)",
				M.Element.Type[element.type].notation, element.index
			)
		else
			return Match.Error(
				"element %s%d already exists",
				M.Element.Type[element.type].notation, element.index
			)
		end
	elseif element.index ~= #bucket + 1 then
		return Match.Error(
			"element %s%d is not sequentially ordered",
			M.Element.Type[element.type].notation, element.index
		)
	end
	bucket[element.index] = element
end

-- Element
M.t_body:add({
-- = {...}
Match.Pattern{
	name = element_name_filter,
	vtype = O.Type.null,
	children = M.t_element_body,
	acceptor = function(context, unit, obj)
		local element = M.Element()
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
	branch = Instance.t_head,
	acceptor = function(context, unit, obj)
		local element = M.Element()
		local err = element_acceptor(element, context, unit, obj)
		if err then
			return err
		end
		local step = M.Step()
		step.index = 1
		step.implicit = true
		table.insert(element.steps, step)

		local instance = Instance()
		table.insert(step.composition.items, instance)
		return instance
	end,
	post_branch_pre = function(_, instance, _)
		instance:set_name("prototype")
	end,
	-- post_branch_pre = element_post_branch,
},
-- implicit: P1
Match.Pattern{
	any = true,
	branch = M.t_element_body,
	acceptor = function(_, unit, obj)
		local bucket = unit.elements[M.Element.Type.primary]
		local element = bucket[1]
		if not element then
			element = M.Element()
			element.type = M.Element.Type.primary
			element.index = 1
			element.implicit = true
			bucket[element.index] = element
		elseif not element.implicit or #bucket > 1 then
			return Match.Error("sub-property defined in unit root after explicit element")
		end
		return element
	end,
	post_branch_pre = element_post_branch,
},
})

M.t_element_body:add(shared_props)

local function step_post_branch(_, composition, _)
	if #composition.items == 0 then
		return Match.Error("no items specified for step")
	end
end

-- FIXME: cyclic dependency
if not Quanta.Composition then
	require "Quanta.Composition"
end

-- Step
M.t_element_body:add({
-- RS#{...}
Match.Pattern{
	vtype = O.Type.identifier,
	value = function(_, _, obj, _)
		return string.find(O.identifier(obj), "^RS[0-9]+$") ~= nil
	end,
	children = Quanta.Composition.t_body,
	acceptor = function(_, element, obj)
		local step = M.Step()
		step.index = tonumber(string.sub(O.identifier(obj), 3))

		local current = element.steps[step.index]
		if step.index <= 0 then
			return Match.Error("step RS%d index must be greater than 0", step.index)
		elseif current then
			if element.steps[1].implicit then
				return Match.Error("cannot mix implicit and explicit steps (at explicit step RS%d)", step.index)
			else
				return Match.Error("step RS%d was already specified", step.index)
			end
		elseif step.index ~= #element.steps + 1 then
			return Match.Error("step RS%d is not sequentially ordered", step.index)
		end
		element.steps[step.index] = step
		return step.composition
	end,
	post_branch_pre = step_post_branch,
},
-- implicit: RS1
Match.Pattern{
	any = true,
	branch = Quanta.Composition.t_body,
	acceptor = function(_, element, obj)
		local step = element.steps[1]
		if not step then
			step = M.Step()
			step.index = 1
			step.implicit = true
			element.steps[step.index] = step
		elseif not step.implicit or #element.steps > 1 then
			return Match.Error("sub-property defined in element root after explicit step")
		end
		return step.composition
	end,
	post_branch_pre = step_post_branch,
},
})

M.t_element_body:build()
M.t_body:build()
M.t_head:build()

return M

)"__RAW_STRING__"