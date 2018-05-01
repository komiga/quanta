u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local M = U.module(...)

function M.set_debug(general, trace)
	M.debug = general
	M.debug_trace = general or trace
end

M.set_debug(false, false)
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
M.filters.name["table"] 	= function(_, _, obj, p) return p.names[O.name(obj)] ~= nil end
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

M.filters.value[false] = function(_, _, obj, p)
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

local function make_sub_filter(g, name, name_collect, alt_has_name)
	local name_post = name_collect .. "_post"
	local name_num = "num_" .. name
	local has_func = O["has_" .. (alt_has_name or name)]
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

M.filters.expression = make_sub_filter({}, "expression", "collect_expression", "operands")
M.filters.children = make_sub_filter({}, "children", "collect")
M.filters.tags     = make_sub_filter({}, "tags", "collect_tags")

M.filters.quantity = {}

M.filters.quantity.init = function(p, r)
	U.assertl(
		3, U.is_type_any(r.quantity, {M.Tree, "table", "function", "boolean"}, true),
		"pattern rule 'quantity' has invalid value"
	)
	if r.quantity == nil then
		return M.filters.quantity[false]
	elseif r.quantity == M.Any then
		return nil
	elseif U.is_type(r.quantity, "function") then
		return r.quantity
	elseif not U.is_type(r.quantity, "boolean") then
		p.quantity = r.quantity
	end
	return M.filters.quantity[filter_selector(r.quantity)]
end

M.filters.quantity[false] 	= function(_, _, obj, p) return not O.has_quantity(obj) end
M.filters.quantity[true] 	= function(_, _, obj, p) return O.has_quantity(obj) end

M.filters_ordered = {}

local function add_filter_group(name)
	local group = M.filters[name]
	U.assert(group)
	table.insert(M.filters_ordered, {name = name, group = group})
end

add_filter_group("name")
add_filter_group("value")
add_filter_group("func")
add_filter_group("expression")
add_filter_group("children")
add_filter_group("tags")
add_filter_group("quantity")

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
	if r.layer then
		U.type_assert(r.layer, M.Pattern)
		self.filters = r.layer.filters
		for k, v in pairs(r.layer) do
			self[k] = v
		end
		self.children = nil
		self.children_post = nil
		self.tags = nil
		self.tags_post = nil
		self.quantity = nil
		self.quantity_post = nil

		self.branch = M.Pattern{any = true}
		for k, v in pairs(r.layer) do
			self.branch[k] = v
		end
		self.branch.filters = {}
		self.branch.name = nil
		self.branch.names = nil

		if M.debug_trace then
			self.branch.definition_location = string.format("[layer] %s", self.branch.definition_location)
			self.definition_location = string.format(
				"%s ==> %s",
				U.get_trace(2),
				r.layer.definition_location
			)
		end

		self.branch = {self.branch}
	else
		self.filters = {}
		if r.branch then
			if U.is_type(r.branch, M.Pattern) then
				self.branch = {r.branch}
			elseif U.is_type_any(r.branch, {M.Tree, "table"}) then
				self.branch = r.branch
			else
				U.assert(false, "branch rule must be a Match.Tree, a Match.Pattern, or a table containing either")
			end
		end
		if not r.any then
			for _, filter in ipairs(M.filters_ordered) do
				local f = filter.group.init(self, r)
				if f then
					table.insert(self.filters, filter_wrapper(filter.name, f))
				end
			end
		end
		if M.debug_trace then
			self.definition_location = U.get_trace(2)
		end
	end

	self.acceptor = U.type_assert(r.acceptor, "function", true)
	self.post_branch = U.type_assert(r.post_branch, "function", true)
	self.post_branch_pre = U.type_assert(r.post_branch_pre, "function", true)
end

function M.Pattern:matches(context, value, obj, keyed)
	local start = 1
	if keyed and (self.names or self.name) then
		start = 2
	end
	local filters = self.filters
	for i = start, #filters do
		local value = filters[i](context, value, obj, self)
		U.type_assert(value, "boolean")
		if not value then
			return false
		end
	end
	return true
end

M.Tree = U.class(M.Tree)

function M.Tree:__init(patterns)
	self.built = false
	self.nodes = {}
	self.positional = nil
	self.keyed = nil
	if M.debug_trace then
		self.definition_location = U.get_trace(2)
	end

	if patterns ~= nil then
		self:add(patterns)
	end
end

function M.Tree:add(n)
	if U.is_type_any(n, {M.Pattern, M.Tree}) then
		table.insert(self.nodes, n)
	elseif U.is_type(n, "table") then
		for _, pattern in pairs(n) do
			self:add(pattern)
		end
	else
		U.assert(false, "n must be a Match.Tree, a Match.Pattern, or a table containing either")
	end
	return n
end

function M.Tree:check_built()
	if not self.built then
		U.assertl(1, false, self.definition_location)
	end
end

function M.Tree:build()
	U.assert(not self.built)

	self.positional = {}
	self.keyed = {}

	local function add_keyed(p, name_hash)
		local key_patterns = self.keyed[name_hash]
		if not key_patterns then
			key_patterns = {}
			self.keyed[name_hash] = key_patterns
		end
		table.insert(key_patterns, p)
	end
	local function add_positional(p)
		table.insert(self.positional, p)
	end
	for _, node in ipairs(self.nodes) do
		if U.is_type(node, M.Pattern) then
			if node.name then
				add_keyed(node, O.hash_name(node.name))
			elseif node.names then
				for name, _ in pairs(node.names) do
					add_keyed(node, O.hash_name(name))
				end
			else
				add_positional(node)
			end
		elseif U.is_type(node, M.Tree) then
			if not node.built then
				node:build()
			end
			for name_hash, key_patterns in pairs(node.keyed) do
				for _, p in ipairs(key_patterns) do
					add_keyed(p, name_hash)
				end
			end
			for _, p in ipairs(node.positional) do
				table.insert(self.positional, p)
			end
		else
			U.assert(false)
		end
	end
	self.built = true
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

local function into_tree(tree, keyed, patterns)
	if U.is_type(patterns, M.Tree) then
		tree = patterns
		return tree, tree.keyed, tree.positional
	else
		return tree, keyed, patterns
	end
end

local do_pattern, do_object, do_sub

do_pattern = function(context, tree, p, obj, collection, keyed)
	local function postamble(v)
		if M.debug_trace then
			context:trace_pop()
		end
		return v
	end

	if M.debug then
		U.log("pattern: %s%s", keyed and "[keyed] " or "", p.definition_location)
	end
	if not p:matches(context, context:value(), obj, keyed) then
		if M.debug then
			U.log("  [does not match]")
		end
		return
	end
	if M.debug then
		U.log("  [matches]")
	end
	if M.debug_trace then
		context:trace_push(p)
	end
	local pushed = false
	if p.acceptor then
		local value = p.acceptor(context, context:value(), obj)
		if U.is_type(value, M.Error) then
			context:set_error(value, obj)
		end
		if context.error ~= nil then
			return postamble(false)
		end
		if value ~= nil then
			pushed = true
			context:push(tree, value)
			if collection then
				table.insert(collection, value)
			end
		end
	end
	if O.is_expression(obj) then
		if not do_sub(context, tree, nil, p.expression, p.collect_expression_post, obj, O.expression) then
			return postamble(false)
		end
	end
	if not do_sub(context, tree, nil, p.children, p.collect_post, obj, O.children) then
		return postamble(false)
	end
	if not do_sub(context, tree, nil, p.tags, p.collect_tags_post, obj, O.tags) then
		return postamble(false)
	end
	if p.quantity and O.has_quantity(obj) then
		local b_tree, b_keyed, b_positional = into_tree(tree, nil, p.quantity)
		if not do_object(context, b_tree, b_keyed, b_positional, O.quantity(obj), nil) then
			return postamble(false)
		end
	end
	if p.branch then
		local b_tree, b_keyed, b_positional = into_tree(tree, nil, p.branch)
		if not do_object(context, b_tree, b_keyed, b_positional, obj, collection) then
			return postamble(false)
		end
	end

	local function do_post_branch(f)
		local err = f(context, context:value(), obj)
		if U.is_type(err, M.Error) then
			context:set_error(err, obj)
		end
		if context.error ~= nil then
			return false
		end
		return true
	end
	if p.post_branch_pre then
		if not do_post_branch(p.post_branch_pre) then
			return postamble(false)
		end
	end
	if pushed then
		context:pop()
	end
	if p.post_branch then
		if not do_post_branch(p.post_branch) then
			return postamble(false)
		end
	end
	return postamble(true)
end

do_object = function(context, tree, keyed, patterns, obj, collection)
	tree:check_built()
	if M.debug then
		U.log("stack level: %d", #context.stack)
		U.log("object: %s", object_debug_info(obj))
	end
	local r
	local function do_list(list, keyed)
		if not list then
			return
		end
		for _, p in ipairs(list) do
			r = do_pattern(context, tree, p, obj, collection, keyed)
			if r ~= nil then
				return
			end
		end
	end

	local key_patterns = keyed and keyed[O.name_hash(obj)] or nil
	do_list(key_patterns, true)
	if r ~= nil then
		return r
	end
	do_list(patterns, false)
	if r ~= nil then
		return r
	end
	context:set_error(M.Error("no matching pattern for object: %s", object_debug_info(obj)), obj)
	return false
end

do_sub = function(context, tree, keyed, patterns, post, obj, iter_func)
	if keyed == nil and patterns == nil then
		-- filter rule was not a pattern list/tree
		return true
	else
		tree, keyed, patterns = into_tree(tree, keyed, patterns)
	end
	local collection = post and {} or nil
	for _, sub in iter_func(obj) do
		if not do_object(context, tree, keyed, patterns, sub, collection) then
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
		end
		if context.error ~= nil then
			return false
		end
	end
	return true
end

M.Context = U.class(M.Context)

function M.Context:__init()
	self.stack = {}
	self.trace_stack = {}
	self.error = nil
end

function M.Context:set_error(err, obj)
	self.error = err
	if not self.error.obj and obj then
		self.error:set_obj(obj)
	end
	if not self.error.path then
		local path = self:path()
		if path then
			self.error:set_path(path)
		end
	end
end

function M.Context:push(control, value, path)
	U.type_assert(control, M.Tree)
	U.assert(value ~= nil)
	U.type_assert(path, "string", true)
	path = path or self:path()
	table.insert(self.stack, {
		control,
		value,
		path,
	})
end

function M.Context:pop()
	U.assert(#self.stack > 0)
	table.remove(self.stack)
end

function M.Context:at(rel)
	rel = rel ~= nil and rel or 0
	return #self.stack > 0 and self.stack[rel < 0 and -rel or (#self.stack - rel)] or nil
end

function M.Context:control(rel)
	local l = self:at(rel)
	return l and l[1] or nil
end

function M.Context:value(rel)
	local l = self:at(rel)
	return l and l[2] or nil
end

function M.Context:path(rel)
	local l = self:at(rel)
	return l and l[3] or nil
end

function M.Context:trace_push(pattern)
	U.type_assert(pattern, M.Pattern)
	table.insert(self.trace_stack, pattern)
end

function M.Context:trace_pop()
	U.assert(#self.trace_stack > 0)
	table.remove(self.trace_stack)
end

function M.Context:trace_tostring()
	local str = string.format("pattern trace (%d):", #self.trace_stack)
	for i = #self.trace_stack, 1, -1 do
		local p = self.trace_stack[i]
		if p.definition_location ~= nil then
			str = str .. string.format("\n    %2d %s", i, p.definition_location)
		end
	end
	return str
end

function M.Context:consume(tree, obj, root, path)
	if root ~= nil then
		self:push(tree, root, path)
	end
	local r = do_object(self, tree, tree.keyed, tree.positional, obj, nil)
	if root ~= nil then
		self:pop()
	end
	return r
end

function M.Context:consume_sub(tree, obj, root, path)
	if root ~= nil then
		self:push(tree, root, path)
	end
	local r = do_sub(self, tree, tree.keyed, tree.positional, nil, obj, O.children)
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
	self.obj = nil
	self.source_line = 0
	self.path = nil
end

function M.Error:set_obj(obj)
	U.type_assert(obj, "userdata")
	self.obj = obj
	self.source_line = O.source_line(self.obj)
end

function M.Error:set_path(path)
	U.type_assert(path, "string")
	self.path = path
end

function M.Error:to_string()
	local str =  self.location .. ": error: " .. self.msg
	if self.path then
		str = str .. string.format("\nin file: %s\n", self.path)
	end
	if self.obj then
		str = str ..
			string.format("\nat object (line %d): ```\n", self.source_line) ..
			O.write_text_string(self.obj, true) ..
			"\n```"
	end
	return str
end

function M.Error:print()
	print(self:to_string())
end

return M

)"__RAW_STRING__"