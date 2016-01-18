u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Entity = require "Quanta.Entity"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local Composition = require "Quanta.Composition"
local M = U.module(...)

M.Type = {
	{name = "none", notation = ""},
	{name = "self", notation = "S"},
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
	self.description = ""
	self.author = {}
	self.note = {}
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

function M:from_object(obj, search_in, controllers)
	U.type_assert(obj, "userdata")
	U.type_assert(search_in, "table")
	U.type_assert(controllers, "table")

	local context = Match.Context()
	context.user = {
		search_in = search_in,
		controllers = controllers,
	}
	if not context:consume(M.t_head, obj, self) then
		U.log("match error:\n%s", context.error:to_string(context.error_obj))
		return false
	end
	return true
end

local function to_object_shared(self, obj)
	if self.description ~= "" then
		local description_obj = O.push_child(obj)
		O.set_name(description_obj, "d")
		O.set_string(description_obj, self.description)
	end

	local author_obj
	if #self.author > 0 then
		author_obj = O.push_child(obj)
		O.set_name(author_obj, "author")
	end
	if #self.author == 1 then
		O.set_string(author_obj, self.author[1])
	elseif #self.author > 1 then
		for _, author in pairs(self.author) do
			if author ~= "" then
				O.set_string(O.push_child(author_obj), author)
			end
		end
	end

	local note_obj
	if #self.note > 0 then
		note_obj = O.push_child(obj)
		O.set_name(note_obj, "note")
	end
	if #self.note == 1 then
		self.note[1]:to_object(O.push_child(note_obj))
	elseif #self.note > 1 then
		for _, note in pairs(self.note) do
			note:to_object(O.push_child(note_obj))
		end
	end
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
	if self.type == M.Type.none then
		O.set_null(obj)
	else
		O.set_identifier(obj, M.Type[self.type].notation)
	end

	to_object_shared(self, obj)
	for _, bucket in pairs(self.elements) do
		for _, e in pairs(bucket) do
			e:to_object(O.push_child(obj))
		end
	end

	return obj
end

M.Note = U.class(M.Note)

function M.Note:__init(text, time)
	self.text = U.type_assert(text, "string")
	self.time = U.type_assert(time, "userdata", true)
end

function M.Note:to_object(obj)
	U.type_assert(obj, "userdata")

	if self.time then
		local properties = O.push_child(obj)
		O.set_time(O.push_child(properties), self.time)
		O.set_string(O.push_child(properties), self.text)
	else
		O.set_string(obj, self.text)
	end
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
	self.description = ""
	self.author = {}
	self.note = {}
	self.steps = {}
end

function M.Element:to_object(obj)
	U.type_assert(obj, "userdata")

	O.set_name(obj, M.Element.Type[self.type].notation .. tostring(self.index))
	for _, s in pairs(self.steps) do
		s:to_object(O.push_child(obj))
	end
end

-- Pattern matching for M:from_object()

M.t_head = Match.Tree()
M.t_body = Match.Tree()

M.t_element_body = Match.Tree()

-- Unit, Element
M.p_shared = {
-- d = "..."
Match.Pattern{
	name = "d",
	vtype = O.Type.string,
	acceptor = function(_, thing, obj)
		thing.description = O.string(obj)
	end,
},
-- author = "..."
Match.Pattern{
	name = "author",
	vtype = O.Type.string,
	acceptor = function(_, thing, obj)
		if #thing.author > 0 then
			return Match.Error("author was already specified")
		end
		table.insert(thing.author, O.string(obj))
	end,
},
-- author = {...}
Match.Pattern{
	name = "author",
	vtype = O.Type.null,
	children = {
		Match.Pattern{
			vtype = O.Type.string,
			acceptor = function(_, thing, obj)
				table.insert(thing.author, O.string(obj))
			end,
		},
	},
	acceptor = function(_, thing, obj)
		if #thing.author > 0 then
			return Match.Error("author was already specified")
		end
	end,
},
-- note = "..."
Match.Pattern{
	name = "note",
	vtype = O.Type.string,
	acceptor = function(_, thing, obj)
		table.insert(thing.note, M.Note(O.string(obj)))
	end,
},
-- note = {time, string}
Match.Pattern{
	name = "note",
	vtype = O.Type.null,
	children = function(_, _, obj, _)
		return (
			O.num_children(obj) == 2 and
			O.is_type(O.child_at(obj, 1), O.Type.time) and
			O.is_type(O.child_at(obj, 2), O.Type.string)
		)
	end,
	acceptor = function(_, thing, obj)
		table.insert(thing.note, M.Note(
			O.string(O.child_at(obj, 2)),
			-- TODO: make managed Time object
			-- O.time(O.child_at(obj, 1))
			nil
		))
	end,
},
-- note = {...}
Match.Pattern{
	name = "note",
	vtype = O.Type.null,
	children = {
		-- string
		Match.Pattern{
			vtype = O.Type.string,
			acceptor = function(_, thing, obj)
				table.insert(thing.note, M.Note(O.string(obj)))
			end,
		},
		-- {time, string}
		Match.Pattern{
			vtype = O.Type.null,
			children = function(_, _, obj, _)
				return (
					O.num_children(obj) == 2 and
					O.is_type(O.child_at(obj, 1), O.Type.time) and
					O.is_type(O.child_at(obj, 2), O.Type.string)
				)
			end,
			acceptor = function(_, thing, obj)
				table.insert(thing.note, M.Note(
					O.string(O.child_at(obj, 2)),
					-- TODO: make managed Time object
					-- O.time(O.child_at(obj, 1))
					nil
				))
			end,
		},
	},
},
}

-- {...}
-- UNIT = {...}
-- UNIT = type{...}
M.t_head:add(Match.Pattern{
	name = Match.Any,
	vtype = {O.Type.null, O.Type.identifier},
	children = M.t_body,
	acceptor = function(_, unit, obj)
		local type_name = O.is_type(obj, O.Type.identifier) and O.identifier(obj) or ""
		unit.type = M.TypeByNotation[type_name]
		if unit.type == nil then
			return Match.Error("'%s' is not a valid unit type", type_name)
		end
		unit:set_name(O.name(obj))
	end,
	--[[post_branch = function(_, unit, _)
		local primary_bucket = unit.elements[M.Element.Type.primary]
		if #primary_bucket == 0 then
			return Match.Error("no primary elements specified for unit")
		end
	end,--]]
})

M.t_body:add(M.p_shared)

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
		if bucket[element.index] then
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
	any_branch = M.t_element_body,
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

M.t_element_body:add(M.p_shared)

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
		if element.steps[step.index] then
			return Match.Error("step 'RS%d' was already specified", step.index)
		end
		element.steps[step.index] = step
		return step.composition
	end,
	post_branch_pre = step_post_branch,
},
-- implicit: RS1
Match.Pattern{
	any_branch = Composition.t_body,
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

return M

)"__RAW_STRING__"