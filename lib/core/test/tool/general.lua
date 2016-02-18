
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
}

Tool.add_tools({
Tool("subtool", {}, {}, [=[
subtool
  sub-tool test
]=],
function(self, parent, params)
	U.print("%s -> %s: TEST", parent.name, self.name)
end),
})

function do_test(t)
	U.print("argv: %s", table.concat(t.argv, " ", 2))
	U.assert(t.success == Tool.main(t.argv))
	print()
end

function main(_)
	for _, t in pairs(tests) do
		do_test(t)
	end

	return 0
end

return main(...)
