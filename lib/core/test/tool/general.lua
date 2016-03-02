
local U = require "togo.utility"
local Tool = require "Quanta.Tool"

Tool.log_level = Tool.LogLevel.debug

function make_test(success, ...)
	return {
		success = success,
		argv = {"lib/core/test/tool/general.lua", ...},
	}
end

local tests = {
	make_test(false),
	make_test(false, "-x"),

	make_test(true, "help"),
	make_test(false, "help", "nonexistent_command"),
	make_test(true, "help", "subtool"),
	make_test(true, "subtool"),
	make_test(true, "subtool", "-x"),
	make_test(true, "subtool", "help", "-x"),

	make_test(true, "subtool", "command"),
	make_test(true, "subtool", "command", "-y"),
	make_test(true, "subtool", "help", "command"),
}

local subtool = Tool("subtool", {}, {}, [=[
subtool [command]
  sub-tool test
]=],
function(self, parent, options, params)
	U.print("%s -> %s: %d", parent.name, self.name, #params)
	if #params > 0 then
		return self:run_command(params)
	end
end)

subtool:add_options({
Tool.Option("-x", "boolean", [=[
-x
  sub-tool option test
]=],
function(_, value)
	U.print("-x: %s", value)
end),
})

local subtool_command = Tool("command", {}, {}, [=[
command [-y]
  sub-tool command test
]=],
function(self, parent, options, params)
	U.print("%s -> %s: %d", parent.name, self.name, #params)
end)

subtool_command:add_options({
Tool.Option("-y", "boolean", [=[
-y
  sub-tool command option test
]=],
function(_, value)
	U.print("-y: %s", value)
end),
})

subtool:add_commands({
	Tool.help_command,
	subtool_command,
})
Tool.add_tools(subtool)

function do_test(t)
	U.print("argv: %s", table.concat(t.argv, " ", 2))
	U.assert(t.success == Tool.main(t.argv))
	print()
end

function main(_)
	Tool.main_config = {
		vessel_root = "vessel_data",
		vessel_work_local = true,
	}

	for _, t in pairs(tests) do
		do_test(t)
	end

	return 0
end

return main(...)
