u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local M = U.module(...)

M.LogLevel = {
	silent = 0,
	error = 1,
	info = 2,
	debug = 3,
}

M.log_level = M.LogLevel.info

local BYTE_LF = string.byte('\n', 1)

local function boundary(text, s, e, byte)
	local step = s > e and -1 or 1
	for i = s, e, step do
		if string.byte(text, i) ~= byte then
			return i
		end
	end
	return s
end

local function trim_text(text)
	return string.sub(
		text,
		boundary(text, 1, #text, BYTE_LF),
		boundary(text, #text, 1, BYTE_LF)
	)
end

local function indent_text(text, level)
	if level <= 0 then
		return text
	end
	local indent = string.rep("  ", level)
	text, _ = string.gsub(text, "\n", "\n" .. indent)
	return indent .. text
end

function M.log_error(msg, ...)
	U.type_assert(msg, "string")
	if M.log_level >= M.LogLevel.error then
		U.print("error: " .. msg, ...)
	end
	return false
end

function M.log(msg, ...)
	U.type_assert(msg, "string")
	if M.log_level >= M.LogLevel.info then
		U.print(msg, ...)
	end
end

function M.log_debug(msg, ...)
	U.type_assert(msg, "string")
	if M.log_level >= M.LogLevel.debug then
		U.print(U.get_trace(1) .. ": debug: " .. msg, ...)
	end
end

function M.error(msg, ...)
	error(U.get_trace(1) .. ": error: " .. string.format(msg, ...), 0)
end

U.class(M)

function M:__init(name, options, commands, help_text, func)
	U.type_assert(name, "string")
	U.type_assert(help_text, "string")
	U.type_assert(options, "table")
	U.type_assert(func, "function")

	self.name = name
	self.help_text = trim_text(help_text)
	self.func = func
	self.options = {}
	self.commands = {}

	self.auto_read_options = true

	self:add_options(options)
	self:add_commands(commands)
end

function M:add_options(options)
	if U.is_type(options, M.Option) then
		local option = options
		for _, name in ipairs(option.names) do
			U.assert(
				self.options[name] == nil,
				"tool %s: option must be unique: %s",
				self.name, name
			)
			self.options[name] = option
			table.insert(self.options, option)
		end
	else
		U.type_assert(options, "table")
		for _, option in pairs(options) do
			self:add_options(option)
		end
	end
end

function M:add_commands(commands)
	if U.is_type(commands, M) then
		local command = commands
		U.assert(command ~= self)
		U.assert(
			self.commands[command.name] == nil,
			"tool %s: command must be unique: %s",
			self.name, command.name
		)
		self.commands[command.name] = command
		table.insert(self.commands, command)
	else
		U.type_assert(commands, "table")
		for _, command in pairs(commands) do
			self:add_commands(command)
		end
	end
end

function M:print_help(level)
	level = U.type_assert(level, "number") or 0

	print(indent_text(self.help_text, level))
	for _, option in ipairs(self.options) do
		print()
		option:print_help(level + 1)
	end
	for _, command in ipairs(self.commands) do
		print()
		command:print_help(level + 1)
	end
end

function M:read_options(options)
	for _, option in ipairs(options) do
		local tool_option = self.options[option.name]
		if not tool_option then
			return M.log_error("%s: option not recognized: %s", self.name, option.name)
		end
		if not tool_option:consume(self, option.name, option.value) then
			return false
		end
	end
	return true
end

function M:run(parent, options, params)
	if self.auto_read_options then
		if not self:read_options(options) then
			return false
		end
	end
	local rv = self.func(self, parent, options, params)
	if rv == nil then
		return true
	end
	return U.type_assert(rv, "boolean")
end

function M:run_command(parent_params)
	local options = {}
	local params = {}

	if #parent_params == 0 then
		return M.log_error("%s: expected command", self.name)
	end
	local name = parent_params[1].value
	local i = 2
	while i <= #parent_params do
		local p = parent_params[i]
		if not p.name then
			break
		end
		table.insert(options, p)
		i = i + 1
	end
	while i <= #parent_params do
		table.insert(params, parent_params[i])
		i = i + 1
	end

	local command = self.commands[name]
	if not command then
		return M.log_error("%s: command not recognized: %s", self.name, name)
	end
	return command:run(self, options, params)
end

M.Option = U.class(M.Option)

function M.Option:__init(names, types, help_text, func)
	U.type_assert_any(names, {"string", "table"})
	U.type_assert_any(table, {"string", "table"}, true)
	U.type_assert(help_text, "string")
	U.type_assert(func, "function")

	self.names = U.is_type(names, "table") and names or {names}
	if types then
		self.types = U.is_type(types, "table") and types or {types}
	else
		self.types = nil
	end
	self.help_text = trim_text(help_text)
	self.func = func
end

function M.Option:print_help(level)
	level = U.type_assert(level, "number") or 0

	print(indent_text(self.help_text, level))
end

function M.Option:consume(tool, name, value)
	if self.types and not U.is_type_any(value, self.types) then
		return M.log_error("%s: value type is invalid", name)
	end
	local rv = self.func(tool, value)
	if rv == nil then
		return true
	end
	return U.type_assert(rv, "boolean")
end

M.help_command = M("help", {}, {}, [=[
help [command_name | option_name [...]]
  prints help for commands
]=],
function(self, parent, options, params)
	if #params == 0 then
		parent:print_help(-1)
		return true
	end

	local function do_bucket(kind, list, bucket, get_name)
		for _, p in ipairs(list) do
			local name = get_name(p)
			local thing = bucket[name]
			if not thing then
				return M.log_error(
					"%s %s: %s not recognized: %s",
					parent.name, self.name,
					kind, name
				)
			end
			thing:print_help(0)
		end
		return true
	end
	return (
		do_bucket("option", options, parent.options, function(p) return p.name end) and
		do_bucket("command", params, parent.commands, function(p) return p.value end)
	)
end)
M.help_command.auto_read_options = false

local main_options = {
M.Option("--log", nil, [=[
--log=error | info | debug
  set log level
  default: info
]=],
function(_, value)
	local given = value
	if U.is_type(value, "string") then
		value = M.LogLevel[value]
	end
	if U.is_type(value, "number") and value >= M.LogLevel.info and value <= M.LogLevel.debug then
		M.log_level = value
		return true
	end
	return M.log_error("--log is invalid: %s", tostring(given))
end),
}

local main_commands = {
M.help_command,
}

M.main_tool = M("main", main_options, main_commands, [=[
main [options] command [command_options] [command_params]
]=],
function(self, parent, options, params)
	return self:run_command(params)
end)

function M.add_tools(tools)
	M.main_tool:add_commands(tools)
end

local function concat_params(to, from)
	for _, p in ipairs(from) do
		table.insert(to, p)
	end
end

local function nilify_params(params)
	for _, p in ipairs(params) do
		if p.name == "" then
			p.name = nil
		end
		if p.value == "" then
			p.value = nil
		end
	end
end

function M.main(argv)
	local _, opts, cmd_opts, cmd_params = U.parse_args(argv)
	local params = {}
	opts.name = nil
	cmd_opts.name = nil

	concat_params(params, cmd_opts)
	if cmd_params.name ~= "" then
		table.insert(params, {value = cmd_params.name})
	end
	cmd_params.name = nil
	concat_params(params, cmd_params)

	nilify_params(opts)
	nilify_params(params)
	return M.main_tool:run(nil, opts, params)
end

return M

)"__RAW_STRING__"