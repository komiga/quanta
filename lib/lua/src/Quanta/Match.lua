
local U = require "Quanta.Util"
local O = require "Quanta.Object"
local M = U.module("Quanta.Match")

M.debug = false
M.Any = {}

local function filter_selector(x)
	if x == M.Any or U.is_type(x, "boolean") then
		return x
	end
	return U.type_class(x)
end

local function rule_value_to_table(v)
	return U.is_type(v, "table") and v or {v}
end

M.filters = {}

M.filters.func = {}
M.filters.func.init = function(p, r)
	if r.func then
		U.assertl(
			3, U.is_type(r.func, "function"),
			"pattern rule 'func' must be a function"
		)
		return r.func
	end
end
M.filters.func.yes = function(_, _, _, _)
	return true
end

M.filters.name = {}
M.filters.name.init = function(p, r)
	U.assertl(
		3, U.is_type_any(r.name, {"table", "function", "string", "boolean"}, true),
		"pattern rule 'name' has invalid value"
	)
	if r.name == nil then
		return M.filters.name[false]
	elseif r.name == M.Any then
		return nil
	elseif U.is_type(r.name, "table") then
		p.names = #r.name == 0 and r.name or U.table_inverse(r.name, true)
	elseif U.is_type(r.name, "function") then
		return r.name
	elseif U.is_type(r.name, "string") then
		p.name = r.name
	end
	local f = M.filters.name[filter_selector(r.name)]
	U.assertl(3, f ~= nil, "no matching filter for pattern rule 'name' of '%s'", tostring(r.name))
	return f
end

M.filters.name[false] 		= function(_, _, obj, p) return not O.is_named(obj) end
M.filters.name[true] 		= function(_, _, obj, p) return O.is_named(obj) end
M.filters.name["table"] 	= function(_, _, obj, p) return p.names[O.name(obj)] == true end
M.filters.name["string"] 	= function(_, _, obj, p) return p.name == O.name(obj) end

M.filters.value = {}
M.filters.value.init = function(p, r)
	U.assertl(3, r.value == nil or r.vtype ~= nil, "pattern rule 'vtype' must be present when 'value' is")
	if U.is_type(r.vtype, "number") then
		p.value_type_mask = r.vtype
	elseif U.is_type(r.vtype, "function") then
		return r.vtype
	elseif r.vtype == M.Any then
		return nil
	elseif U.is_type(r.vtype, "table") then
		local mask = 0
		for _, vtype in pairs(r.vtype) do
			mask = mask + vtype
		end
		p.value_type_mask = mask
	else
		U.assertl(3, r.vtype == nil, "pattern rule 'vtype' has invalid value")
		return M.filters.value[false]
	end
	if r.value == nil then
		return M.filters.value.typed
	elseif U.is_type(r.value, "function") then
		p.value_func = r.value
		return M.filters.value.typed
	else
		p.values = rule_value_to_table(r.value)
		for _, v in pairs(p.values) do
			local c = M.filters.value.compare[type(v)]
			U.assertl(3, c, "pattern rule 'value' contains invalid type: %s (of type %s)", tostring(v), type(v))
		end
		return M.filters.value.valued
	end
end

M.filters.value.reject = function(_, _, obj, p)
	return O.is_null(obj)
end

M.filters.value.typed = function(context, value, obj, p)
	if O.is_type_any(obj, p.value_type_mask) then
		if p.value_func then
			return p.value_func(context, value, obj, p)
		else
			return true
		end
	end
	return false
end

M.filters.value.compare = {}
M.filters.value.compare["number"] = function(_, _, obj, _, v)
	if O.is_decimal(obj) then
		return O.decimal(obj) == v
	elseif O.is_integer(obj) then
		return O.integer(obj) == v
	end
	return false
end
M.filters.value.compare["boolean"] = function(_, _, obj, _, v)
	return O.is_boolean(obj) and O.boolean(obj) == v
end
M.filters.value.compare["string"] = function(_, _, obj, _, v)
	return O.is_textual(obj) and O.text(obj) == v
end
M.filters.value.compare["function"] = function(context, value, obj, p, v)
	return v(context, value, obj, p)
end

M.filters.value.valued = function(context, value, obj, p)
	if not O.is_type_any(obj, p.value_type_mask) then
		return false
	elseif O.is_null(obj) then
		return true
	end
	for _, v in pairs(p.values) do
		if M.filters.value.compare[type(v)](context, value, obj, p, v) then
			return true
		end
	end
	return false
end

local function make_sub_filter(g, name, name_collect)
	local name_post = name_collect .. "_post"
	local name_num = "num_" .. name
	local has_func = O["has_" .. name]
	local num_func = O["num_" .. name]
	local is_children = has_func == O.has_children

	g.init = function(p, r)
		U.assertl(
			3, r[name] == nil or r[name_collect] == nil,
			"pattern rules '%s' and '%s' are mutually-exclusive",
			name, name_collect
		)
		U.assertl(
			3, not r[name_collect] or r[name_post],
			"pattern rule '%s' must be set with rule '%s'",
			name_collect, name_post
		)
		U.assertl(
			3, not r[name_collect] or U.is_type_any(r[name_collect], {"table", M.Tree}),
			"pattern rule '%s' must be a tree or table",
			name_collect
		)
		local rule_value = r[name] or r[name_collect]
		U.type_assert_any(rule_value, {"boolean", "number", "function", "table", M.Tree}, true, 3)
		if rule_value == nil then
			return (is_children and r.vtype == O.Type.expression) and g[true] or g[false]
		elseif U.is_type(rule_value, "number") then
			p[name_num] = rule_value
		elseif U.is_type(rule_value, "function") then
			return rule_value
		elseif rule_value == M.Any then
			return nil
		elseif not U.is_type(rule_value, "boolean") then
			p[name] = rule_value
		end
		p[name_post] = r[name_post]
		U.type_assert(p[name_post], "function", true, 3)
		return g[filter_selector(rule_value)]
	end

	g[false] 	= function(_, _, obj, p) return not has_func(obj) end
	g[true] 	= function(_, _, obj, p) return has_func(obj) end
	g["number"]	= function(_, _, obj, p) return num_func(obj) == p[name_num] end

	return g
end

M.filters.children = make_sub_filter({}, "children", "collect")
M.filters.tags     = make_sub_filter({}, "tags", "collect_tags")

M.Pattern = U.class(M.Pattern)

local function filter_wrapper(name, f)
	if M.debug then
		return function(context, value, obj, p)
			local r = f(context, value, obj, p)
			if M.debug then
				U.log("  filter %s => %s", name, tostring(r))
			end
			return r
		end
	end
	return f
end

function M.Pattern:__init(...)
	local r = ...
	U.type_assert(r, "table")
	self.filters = {}
	if r.any then
		table.insert(self.filters, M.filters.func.yes)
	elseif r.any_branch then
		U.type_assert(r.any_branch, M.Tree)
		self.any_branch = r.any_branch
		table.insert(self.filters, M.filters.func.yes)
	else
		for name, g in pairs(M.filters) do
			local f = g.init(self, r)
			if f then
				table.insert(self.filters, filter_wrapper(name, f))
			end
		end
	end
	self.acceptor = r.acceptor
	if M.debug then
		self.definition_location = U.get_trace(2)
	end
end

function M.Pattern:matches(context, value, obj)
	for _, f in pairs(self.filters) do
		if not f(context, value, obj, self) then
			return false
		end
	end
	return true
end

M.Tree = U.class(M.Tree)

function M.Tree:__init(patterns)
	self.patterns = U.type_assert(patterns, "table", true) or {}
end

function M.Tree:add(pattern)
	if U.is_type(pattern, M.Pattern) then
		table.insert(self.patterns, pattern)
	elseif U.is_type(pattern, "table") then
		for _, p in pairs(pattern) do
			U.type_assert(p, M.Pattern)
			table.insert(self.patterns, p)
		end
	else
		U.assert(false, "pattern must be a table or a Match.Pattern")
	end
	return pattern
end


local function object_debug_info(obj)
	local s = O.is_named(obj) and O.name(obj) or "<no-name>"
	s = s .. " = "
	if O.is_boolean(obj) then
		s = s .. tostring(O.boolean(obj))
	elseif O.is_integer(obj) then
		s = s .. tostring(O.integer(obj))
	elseif O.is_decimal(obj) then
		s = s .. tostring(O.decimal(obj))
	elseif O.is_time(obj) then
		s = s .. "<time>"
	elseif O.is_string(obj) then
		s = s .. "\"" .. O.string(obj) .. "\""
	elseif O.is_identifier(obj) then
		s = s .. O.identifier(obj)
	elseif O.is_expression(obj) then
		s = s .. "<expression>"
	else
		s = s .. "null"
	end
	if O.has_tags(obj) then
		s = s .. " has_tags"
	end
	if O.has_children(obj) then
		s = s .. " has_children"
	end
	return s
end

local do_object, do_sub

do_object = function(tree, context, patterns, obj, collection)
	if M.debug then
		U.log("stack level: %d", #context.stack)
		U.log("object: %s", object_debug_info(obj))
	end
	for _, p in pairs(patterns) do
		if M.debug then
			U.log("pattern: %s", p.definition_location)
		end
		if p:matches(context, context:value(), obj) then
			if M.debug then
				U.log("  [matches]")
			end
			local pushed = false
			if p.acceptor then
				local value = p.acceptor(context, context:value(), obj)
				if U.is_type(value, M.Error) then
					context:set_error(value, obj)
					return false
				end
				if value ~= nil then
					pushed = true
					context:push(tree, value)
					if collection then
						table.insert(collection, value)
					end
				end
			end
			if p.any_branch then
				if not do_object(p.any_branch, context, p.any_branch.patterns, obj, collection) then
					return false
				end
			else
				if not do_sub(tree, context, p.children, p.collect_post, obj, O.children) then
					return false
				end
				if not do_sub(tree, context, p.tags, p.collect_tags_post, obj, O.tags) then
					return false
				end
			end
			if pushed then
				context:pop()
			end
			return true
		else
			if M.debug then
				U.log("  [does not match]")
			end
		end
	end
	context:set_error(M.Error("no matching pattern for object: %s", object_debug_info(obj)), obj)
	return false
end

do_sub = function(tree, context, patterns, post, obj, iter_func)
	if patterns == nil then
		-- filter rule was not a pattern list/tree
		return true
	elseif U.is_type(patterns, M.Tree) then
		tree = patterns
		patterns = tree.patterns
	end
	local collection = post and {} or nil
	for _, sub in iter_func(obj) do
		if not do_object(tree, context, patterns, sub, collection) then
			return false
		end
	end
	if collection then
		local err = post(context, context:value(), obj, collection)
		if err ~= nil and err ~= true then
			if U.is_type(err, M.Error) then
				context:set_error(err, obj)
			elseif context.error == nil then
				context:set_error(M.Error("unknown error in collection post handler"), obj)
			end
			return false
		end
	end
	return true
end

M.Context = U.class(M.Context)

function M.Context:__init()
	self.stack = {}
	self.error = nil
	self.error_obj = nil
end

function M.Context:set_error(err, obj)
	self.error = err
	self.error_obj = obj
end

function M.Context:push(control, value)
	U.type_assert(control, M.Tree)
	U.assert(value ~= nil)
	table.insert(self.stack, {
		control,
		value,
	})
end

function M.Context:pop()
	U.assert(#self.stack > 0)
	table.remove(self.stack)
end

function M.Context:at(rel)
	rel = rel ~= nil and rel or 0
	return #self.stack > 0 and self.stack[#self.stack - rel] or nil
end

function M.Context:control(rel)
	local l = self:at(rel)
	return l and l[1] or nil
end

function M.Context:value(rel)
	local l = self:at(rel)
	return l and l[2] or nil
end

function M.Context:consume(tree, obj, root)
	if root ~= nil then
		self:push(tree, root)
	end
	local r = do_object(tree, self, tree.patterns, obj, nil)
	if root ~= nil then
		self:pop()
	end
	return r
end

function M.Context:consume_sub(tree, obj, root)
	if root ~= nil then
		self:push(tree, root)
	end
	local r = do_sub(tree, self, tree.patterns, nil, obj, O.children)
	if root ~= nil then
		self:pop()
	end
	return r
end

M.Error = U.class(M.Error)

function M.Error:__init(msg, ...)
	U.type_assert(msg, "string", false, 3)
	self.msg = string.format(msg, ...)
	self.location = U.get_trace(3)
end

function M.Error:to_string(error_obj)
	local str =  self.location .. ": error: " .. self.msg
	if error_obj then
		str = str .. "\nat object: ```\n" .. O.write_text_string(error_obj, true) .. "\n```"
	end
	return str
end

function M.Error:print()
	print(self:tostring())
end

return M
