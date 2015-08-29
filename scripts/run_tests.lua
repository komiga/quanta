#!/usr/bin/env lua5.1

-- Weee
local SCRIPTS_PATH = string.sub(arg[0], 1, -15)

dofile(SCRIPTS_PATH .. "/common.lua")
require("lfs")

local prev_wd = lfs.currentdir()
lfs.chdir(SCRIPTS_PATH .. "/..")
local ROOT = lfs.currentdir()

local group_data = {}

group_data["core"] = {
	excluded = {},
	args = {},
}

local group_names = quanta_libs()
local groups = {}

for _, group_name in pairs(group_names) do
	local root = "lib/" .. group_name .. "/test"
	local group = {
		name = group_name,
		root = ROOT .. "/" .. root,
		data = group_data[group_name],
		tests = {},
	}
	for file in iterate_dir(root, "file") do
		local name, ext = split_path(file)
		if ext == "elf" then
			if group.data.excluded[name] then
				printf("EXCLUDED: %s / %s", group_name, name)
			else
				table.insert(group.tests, file)
			end
		end
	end
	table.insert(groups, group)
end

function run()
	for _, group in pairs(groups) do
		printf("\nGROUP: %s", group.name)
		lfs.chdir(group.root)
		for _, path in pairs(group.tests) do
			local cmd = "./" .. path
			local args = group.data.args[path]
			if args then
				cmd = cmd .. " " .. args
			end
			printf("\nRUNNING: %s", cmd)
			local exit_code = os.execute(cmd)
			if exit_code ~= 0 then
				printf("ERROR: '%s' failed with exit code %d", path, exit_code)
				return -1
			end
		end
	end
	return 0
end

local ec = run()
lfs.chdir(prev_wd)
os.exit(ec)
