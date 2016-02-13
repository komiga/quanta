u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local M = U.module(...)

function M.adapt_list_to_object(prop_class, serialized_name)
	return function(list, obj)
		local num_items = #list + (list[0] ~= nil and 1 or 0)
		if num_items == 0 then
			return
		end
		local holder = O.push_child(obj)
		O.set_name(holder, serialized_name)
		if num_items == 1 then
			prop_class.to_object(list[0] or list[1], holder)
		else
			for _, item in pairs(list) do
				prop_class.to_object(item, O.push_child(holder))
			end
		end
	end
end

function M.prefix_certainty(obj, str)
	if not O.value_certain(obj) then
		str = "?" .. str
	end
	return str
end

--[[function M.string_or_expr_first(obj)
	if O.is_string(obj) then
		return M.prefix_certainty(obj, O.string(obj))
	elseif O.has_children(obj) then
		for _, sub in O.children(obj) do
			if O.is_string(sub) then
				return M.prefix_certainty(obj, O.string(obj))
			end
		end
	end
	return M.prefix_certainty(obj, "")
end--]]

M.Description = {}

function M.Description.struct(description)
	U.type_assert(description, "string")
	return description
end

function M.Description.adapt_struct(serialized_name, property_name)
	local to_object = function(description, obj)
		if description ~= nil and description ~= "" then
			local description_obj = O.push_child(obj)
			O.set_name(description_obj, serialized_name)
			O.set_string(description_obj, description)
		end
	end
	local t_head = Match.Tree({
	-- d = "..."
	Match.Pattern{
		name = serialized_name,
		vtype = O.Type.string,
		acceptor = function(_, thing, obj)
			thing[property_name] = O.string(obj)
		end,
	},
	})
	return to_object, t_head
end

M.Description.struct_to_object,
M.Description.t_struct_head
= M.Description.adapt_struct("d", "description")

M.Author = U.class(M.Author)

function M.Author:__init(name, certain, address, address_certain)
	self.name = U.type_assert(name, "string", true)
	self.certain = U.optional(U.type_assert(certain, "boolean", true), self.name ~= nil and #self.name > 0)
	self.address = U.type_assert(address, "string", true)
	self.address_certain = U.optional(U.type_assert(address_certain, "boolean", true), true)
end

function M.Author:to_object(obj)
	O.set_value_certain(obj, self.certain)
	if self.name ~= nil and self.name ~= "" then
		O.set_string(obj, self.name)
	end
	if not self.address_certain or (self.address ~= nil and self.address ~= "") then
		local addr_obj = O.push_tag(obj)
		O.set_name(addr_obj, "address")
		local value_obj = O.push_child(addr_obj)
		O.set_value_certain(value_obj, self.address_certain)
		if self.address ~= nil and self.address ~= "" then
			O.set_string(value_obj, self.address)
		end
	end
end

function M.Author.struct(list)
	U.type_assert(list, "table")
	return list
end

M.Author.t_head_tags = Match.Tree({
Match.Pattern{
	name = {"address", "addr"},
	children = {Match.Pattern{
		vtype = {O.Type.null, O.Type.string},
		acceptor = function(_, author, obj)
			if O.is_string(obj) then
				author.address = O.string(obj)
				author.address_certain = O.value_certain(obj)
			end
		end
	}},
},
})

function M.Author.adapt_struct(serialized_name, property_name)
	local function element_pattern(name, prelude)
		return Match.Pattern{
			name = name,
			vtype = {O.Type.null, O.Type.string},
			tags = M.Author.t_head_tags,
			acceptor = function(context, thing, obj)
				local r = prelude(context, thing, obj)
				if r then
					return r
				end
				local author = M.Author(
					O.is_string(obj) and O.string(obj) or nil,
					O.value_certain(obj),
					nil, true
				)
				table.insert(thing[property_name], author)
				return author
			end,
		}
	end

	local function acceptor(_, thing, obj)
		if #thing[property_name] > 0 then
			return Match.Error(serialized_name .. " was already specified")
		end
	end

	local to_object = M.adapt_list_to_object(M.Author, serialized_name)
	local t_head = Match.Tree({
	-- = ?
	-- = "..."
	element_pattern(serialized_name, acceptor),
	-- = ... + ...
	-- = {...}
	Match.Pattern{
		name = serialized_name,
		vtype = {O.Type.null, O.Type.expression},
		children = {
			element_pattern(nil, function() end),
		},
		acceptor = acceptor,
	},
	})
	return to_object, t_head
end

M.Author.struct_to_object,
M.Author.t_struct_head
= M.Author.adapt_struct("author", "author")

M.Note = U.class(M.Note)

function M.Note:__init(text, time)
	self.text = U.type_assert(text, "string")
	self.time = U.type_assert(time, "userdata", true)
end

function M.Note:to_object(obj)
	U.type_assert(obj, "userdata")

	if self.time then
		O.set_time(O.push_child(obj), self.time)
		O.set_string(O.push_child(obj), self.text)
	else
		O.set_string(obj, self.text)
	end
end

function M.Note.struct(list)
	U.type_assert(list, "table")
	return list
end

function M.Note.adapt_struct(serialized_name, property_name)
	local function add_timestamped_note(context, thing, obj)
		local t
		local obj_time = O.child_at(obj, 1)
		if not O.has_date(obj_time) or O.is_date_contextual(obj_time) then
			local parent_scope = U.table_last(context.user.scope, true) or context.user.implicit_scope
			if not parent_scope then
				return Match.Error("no time context provided; note time cannot be resolved")
			end
			t = O.time_resolved(obj_time, parent_scope)
		else
			t = T(O.time(obj_time))
		end
		table.insert(thing[property_name], M.Note(O.string(O.child_at(obj, 2)), t))
	end

	local to_object = M.adapt_list_to_object(M.Note, serialized_name)
	local t_head = Match.Tree({
	-- = "..."
	Match.Pattern{
		name = serialized_name,
		vtype = O.Type.string,
		acceptor = function(_, thing, obj)
			table.insert(thing[property_name], M.Note(O.string(obj)))
		end,
	},
	-- = {time, string}
	Match.Pattern{
		name = serialized_name,
		children = function(_, _, obj, _)
			return (
				O.num_children(obj) == 2 and
				O.is_type(O.child_at(obj, 1), O.Type.time) and
				O.is_type(O.child_at(obj, 2), O.Type.string)
			)
		end,
		acceptor = function(context, thing, obj)
			return add_timestamped_note(context, thing, obj)
		end,
	},
	-- = {...}
	Match.Pattern{
		name = serialized_name,
		children = {
			-- string
			Match.Pattern{
				vtype = O.Type.string,
				acceptor = function(_, thing, obj)
					table.insert(thing[property_name], M.Note(O.string(obj)))
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
				acceptor = function(context, thing, obj)
					return add_timestamped_note(context, thing, obj)
				end,
			},
		},
	},
	})
	return to_object, t_head
end

M.Note.struct_to_object,
M.Note.t_struct_head
= M.Note.adapt_struct("note", "note")

return M

)"__RAW_STRING__"