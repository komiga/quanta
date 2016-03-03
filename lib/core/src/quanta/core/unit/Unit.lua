u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Vessel = require "Quanta.Vessel"
local Prop = require "Quanta.Prop"
local Instance = require "Quanta.Instance"
local Composition = require "Quanta.Composition"
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
	-- table.insert(M.Type, t)
	M.Type[t.name] = i
	M.Type[t.notation] = i
	M.TypeByNotation[t.notation] = i
end

U.class(M)

function M:__init()
	self.type = M.Type.none
	self.name = ""
	self.name_hash = O.NAME_NULL
	self.description = Prop.Description.struct("")
	self.author = Prop.Author.struct({})
	self.note = Prop.Note.struct({})
	self.elements = {
		{}, -- M.Element.Type.generic
		{}, -- M.Element.Type.primary
	}
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
	U.type_assert(name, "string")

	self.name = name
	self.name_hash = O.hash_name(self.name)
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

	O.set_name(obj, self.name)
	O.set_identifier(obj, M.Type[self.type].notation)

	to_object_shared(self, obj)
	for _, bucket in ipairs(self.elements) do
		for _, e in ipairs(bucket) do
			e:to_object(O.push_child(obj))
		end
	end

	return obj
end

M.Step = U.class(M.Step)

function M.Step:__init()
	self.index = 1
	self.composition = Composition()
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
M.t_head:add(Match.Pattern{
	name = Match.Any,
	vtype = O.Type.identifier,
	value = function(_, unit, obj, _)
		local t = M.TypeByNotation[O.identifier(obj)]
		if t then
			unit.type = t
			return true
		end
		return false
	end,
	children = M.t_body,
	acceptor = function(_, unit, obj)
		unit:set_name(O.name(obj))
	end,
	--[[post_branch = function(_, unit, _)
		local primary_bucket = unit.elements[M.Element.Type.primary]
		if #primary_bucket == 0 then
			return Match.Error("no primary elements specified for unit")
		end
	end,--]]
})

M.t_body:add(shared_props)

local function element_post_branch(_, element, _)
	if #element.steps == 0 then
		return Match.Error("no steps specified for element")
	end
end

-- Element
M.t_body:add({
-- P# = {...}
-- E# = {...}
Match.Pattern{
	name = function(_, _, obj, _)
		return (
			O.is_named(obj) and
			string.find(O.name(obj), "^[EP][0-9]+$") ~= nil
		)
	end,
	vtype = O.Type.null,
	children = M.t_element_body,
	acceptor = function(_, unit, obj)
		local element = M.Element()
		local name = O.name(obj)
		element.type = M.Element.Type[string.sub(name, 1, 1)]
		U.assert(element.type ~= nil)
		element.index = tonumber(string.sub(name, 2))

		local bucket = unit.elements[element.type]
		if element.index <= 0 then
			return Match.Error(
				"element '%s' (%s, %d) index must be greater than 0",
				name, M.Element.Type[element.type].notation, element.index
			)
		elseif element.index ~= #bucket + 1 then
			return Match.Error(
				"element '%s' (%s, %d) is not sequentially ordered",
				name, M.Element.Type[element.type].notation, element.index
			)
		elseif bucket[element.index] then
			return Match.Error(
				"element '%s' (%s, %d) already exists",
				name, M.Element.Type[element.type].notation, element.index
			)
		end
		bucket[element.index] = element
		return element
	end,
	post_branch_pre = element_post_branch,
},
-- implicit: P1
Match.Pattern{
	any = true,
	branch = M.t_element_body,
	acceptor = function(_, unit, obj)
		local element = M.Element()
		element.type = M.Element.Type.primary
		element.index = 1

		local bucket = unit.elements[element.type]
		if bucket[element.index] then
			return Match.Error(
				"implicit element '%s%d' already exists",
				M.Element.Type[element.type].notation, element.index
			)
		end
		bucket[element.index] = element
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

-- Step
M.t_element_body:add({
-- RS#{...}
Match.Pattern{
	vtype = O.Type.identifier,
	value = function(_, _, obj, _)
		return string.find(O.identifier(obj), "^RS[0-9]+$") ~= nil
	end,
	children = Composition.t_body,
	acceptor = function(_, element, obj)
		local step = M.Step()
		step.index = tonumber(string.sub(O.identifier(obj), 3))
		if step.index <= 0 then
			return Match.Error("step 'RS%d' index must be greater than 0", step.index)
		elseif step.index ~= #element.steps + 1 then
			return Match.Error("step 'RS%d' is not sequentially ordered", step.index)
		elseif element.steps[step.index] then
			return Match.Error("step 'RS%d' was already specified", step.index)
		end
		element.steps[step.index] = step
		return step.composition
	end,
	post_branch_pre = step_post_branch,
},
-- implicit: RS1
Match.Pattern{
	any = true,
	branch = Composition.t_body,
	acceptor = function(_, element, obj)
		local step = M.Step()
		step.index = 1
		if element.steps[step.index] then
			return Match.Error("implicit step 'RS%d' was already specified", step.index)
		end
		element.steps[step.index] = step
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