
Quanta = Quanta or {}
Quanta.Util = Quanta.Util or {}
local M = Quanta.Util

M.debug = false

function M.ternary(cond, x, y)
	return (cond)
		and x
		or  y
end

function M.optional(value, default)
	return (value ~= nil)
		and value
		or default
end

function M.optional_in(table, name, default)
	M.type_assert(table, "table")
	M.type_assert(name, "string")
	table[name] = M.optional(table[name], default)
	return table[name]
end

function M.type_class(x)
	if type(x) == "table" and x.__index then
		return x.__index
	end
	return type(x)
end

function M.is_type(x, tc, basic)
	return basic and type(x) == tc or M.type_class(x) == tc
end

function M.is_type_any(x, types, opt)
	M.type_assert(types, "table")

	if opt and x == nil then
		return true
	end
	local xt = M.type_class(x)
	for _, t in pairs(types) do
		if xt == t then
			return true
		end
	end
	return false
end

function M.get_trace(level)
	local info = debug.getinfo(level + 2, "Sl")
	-- return M.pad_left(info.short_src, 28) .. " @ " .. M.pad_right(tostring(info.currentline), 4)
	return info.short_src .. " @ " .. M.pad_right(tostring(info.currentline), 4)
end

function M.assertl(level, e, msg, ...)
	if not e then
		error(M.get_trace(level + 1) .. ": assertion failed: " .. string.format(msg or "<expression>", ...), 0)
	end
end

function M.assert(e, msg, ...)
	M.assertl(1, e, msg, ...)
end

function M.type_assert(x, tc, opt, level)
	level = M.optional(level, 0)
	M.assertl(
		level + 1,
		(opt and x == nil) or M.is_type(x, tc),
		"Util.type_assert: '%s' (a %s) is not of type %s",
		tostring(x), tostring(type(x)), tostring(tc)
	)
	return x
end

function M.type_assert_any(x, types, opt, level)
	M.type_assert(types, "table")
	level = M.optional(level, 0)

	if opt and x == nil then
		return x
	end
	local xt = M.type_class(x)
	for _, t in pairs(types) do
		if xt == t then
			return x
		end
	end
	M.assertl(
		level + 1, false,
		"Util.type_assert_any: '%s' (a %s) is not of any types specified",
		tostring(x), tostring(type(x))
	)
	return x
end

function M.table_last(table)
	M.assert(#table > 0)
	return table[#table]
end

function M.table_add(a, b)
	M.type_assert(a, "table")
	M.type_assert(b, "table")

	for k, v in pairs(b) do
		a[k] = v
	end
end

function M.table_ijoined(...)
	local r = {}
	for _, t in ipairs({...}) do
		M.type_assert(t, "table")
		for _, v in ipairs(t) do
			table.insert(r, v)
		end
	end
	return r
end

function M.table_inverse(t, value)
	local it = {}
	for k, v in pairs(t) do
		it[v] = value or k
	end
	return it
end

function M.table_keys(t)
	local kt = {}
	for k, _ in pairs(t) do
		table.insert(kt, k)
	end
	return kt
end

function M.pad_left(str, length)
	if #str < length then
		while #str < length do
			str = str .. ' '
		end
	end
	return str
end

function M.pad_right(str, length)
	if #str < length then
		while #str < length do
			str = ' ' .. str
		end
	end
	return str
end

function M.trace()
	print(M.get_trace(1) .. ": TRACE")
end

function M.print(msg, ...)
	M.type_assert(msg, "string")
	print(string.format(msg, ...))
end

function M.log(msg, ...)
	M.type_assert(msg, "string")
	print(M.get_trace(1) .. ": " .. string.format(msg, ...))
end

function M.log_debug(msg, ...)
	M.type_assert(msg, "string")
	if M.debug then
		print(M.get_trace(1) .. ": debug: " .. string.format(msg, ...))
	end
end

function M.min(x, y)
	return x < y and x or y
end

function M.max(x, y)
	return x > y and x or y
end

function M.clamp(x, min, max)
	return x < min and min or x > max and max or x
end

local BYTE_SLASH = string.byte('/', 1)
local BYTE_DOT = string.byte('.', 1)

-- a/b/c -> a
-- a -> nil
function M.rootname(s)
	M.type_assert(s, "string")
	for i = 1, #s do
		if string.byte(s, i) == BYTE_SLASH then
			return string.sub(s, 1, i - 1)
		end
	end
	return nil
end

-- a/b/c -> a/b
-- c -> c
function M.dirname(s)
	M.type_assert(s, "string")
	for i = #s, 1, -1 do
		if string.byte(s, i) == BYTE_SLASH then
			return string.sub(s, 1, i - 1)
		end
	end
	return s
end

-- a/b/c -> c
-- c -> c
function M.basename(s)
	M.type_assert(s, "string")
	local d = 0
	for i = #s, 1, -1 do
		if string.byte(s, i) == BYTE_DOT then
			d = i
			break
		end
	end
	for i = #s, 1, -1 do
		if string.byte(s, i) == BYTE_SLASH and i ~= #s then
			if i < d then
				return string.sub(s, i + 1, d - 1)
			else
				return string.sub(s, i + 1, #s)
			end
		end
	end
	return s
end

function M.strip_extension(s)
	M.type_assert(s, "string")
	local b = nil
	for i = #s, 1, -1 do
		b = string.byte(s, i)
		if b == BYTE_SLASH then
			break
		elseif b == BYTE_DOT then
			return string.sub(s, 1, i - 1)
		end
	end
	return s
end

function M.file_extension(s)
	M.type_assert(s, "string")
	local b
	for i = #s, 1, -1 do
		b = string.byte(s, i)
		if b == BYTE_SLASH then
			break
		elseif b == BYTE_DOT then
			return string.sub(s, i + 1, #s)
		end
	end
	return nil
end

function M.join_paths(...)
	local parts = {...}
	local path = ""
	for i, p in ipairs(parts) do
		M.type_assert(p, "string")
		path = path .. p
		if i ~= #parts then
			path = path .. "/"
		end
	end
	return path
end

function M.set_functable(t, func)
	if not t.__class_static then
		t.__class_static = {}
		t.__class_static.__index = t.__class_static
		setmetatable(t, t.__class_static)
	end
	t.__class_static.__call = func
end

function M.class(c)
	if c == nil then
		c = {}
	end
	c.__index = c
	M.set_functable(c, function(c, ...)
		local obj = {}
		setmetatable(obj, c)
		obj:__init(...)
		return obj
	end)
	return c
end

function M.module(name)
	M.type_assert(name, "string")

	local m = _G
	for p in string.gmatch(name, "([^.]+)%.?") do
		if not m[p] then
			m[p] = {}
		end
		m = m[p]
	end
	m._NAME = name
	m._M = m
	return m
end

return M
