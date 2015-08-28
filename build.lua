
dofile("scripts/common.lua")

local S, G, R = precore.helpers()

quanta = {}

precore.make_config_scoped("quanta.env", {
	once = true,
}, {
{global = function()
	precore.define_group("QUANTA", os.getcwd())
end}})

precore.apply_global({
	"precore.env-common",
	"quanta.env",
})
precore.import(G"${DEP_PATH}/togo")

function quanta.library_config(name)
	configuration {}
		includedirs {
			G"${QUANTA_ROOT}/lib/" .. name .. "/src/",
		}
	quanta.link_library(name)
end

function quanta.app_config(name)
	configuration {}
		includedirs {
			G"${QUANTA_ROOT}/app/" .. name .. "/src/",
		}
	quanta.link_library("app_" .. name)
end

function quanta.link_library(name)
	name = "quanta_" .. name
	if not precore.env_project()["NO_LINK"] then
		configuration {"release"}
			links {name}

		configuration {"debug"}
			links {name .. "_d"}
	end
end

function quanta.make_library(name, configs, env)
	configs = configs or {}
	table.insert(configs, 1, "quanta.strict")
	table.insert(configs, 2, "quanta.base")

	env = precore.internal.env_set({
		QUANTA_LIBRARY = true,
		NO_LINK = true,
	}, env or {})

	local p = precore.make_project(
		"lib_" .. name,
		"C++", "StaticLib",
		"${QUANTA_BUILD}/lib/",
		"${QUANTA_BUILD}/out/${NAME}/",
		env, configs
	)

	configuration {"debug"}
		targetsuffix("_d")

	configuration {}
		targetname("quanta_" .. name)
		files {
			"src/**.cpp",
		}

	if os.isfile("test/build.lua") then
		precore.push_wd("test")
		local prev_solution = solution()
		precore.make_solution(
			"lib_" .. name .. "_test",
			{"debug", "release"},
			{"x64", "x32"},
			nil,
			{
				"precore.generic",
			}
		)
		precore.import(".")
		precore.pop_wd()
		solution(prev_solution.name)
	end
end

function quanta.make_app(name, configs, env)
	configs = configs or {}
	table.insert(configs, 1, "quanta.strict")
	table.insert(configs, 2, "quanta.base")

	env = env or {}
	local lib_env = precore.internal.env_set({
		QUANTA_LIB = true,
		NO_LINK = true,
	}, env)
	local lib_proj = precore.make_project(
		"app_" .. name .. "_lib",
		"C++", "StaticLib",
		"${QUANTA_BUILD}/lib/",
		"${QUANTA_BUILD}/out/${NAME}/",
		lib_env, configs
	)

	configuration {"debug"}
		targetsuffix("_d")

	configuration {}
		targetname("quanta_app_" .. name)
		files {
			"src/**.cpp",
		}
		excludes {
			"src/**/main.cpp"
		}

	local app_env = precore.internal.env_set({
		QUANTA_TOOL = true,
	}, env)
	precore.make_project(
		"app_" .. name,
		"C++", "ConsoleApp",
		"${QUANTA_BUILD}/bin/",
		"${QUANTA_BUILD}/out/${NAME}/",
		app_env, configs
	)

	configuration {"linux"}
		targetsuffix(".elf")

	configuration {}
		targetname(name)
		files {
			"src/**/main.cpp",
		}
end

function quanta.make_test(group, name, srcglob, configs)
	configs = configs or {}
	table.insert(configs, 1, "quanta.strict")
	table.insert(configs, 2, "quanta.base")

	local env = {
		QUANTA_TEST = true,
	}
	precore.make_project(
		group .. "_" .. name,
		"C++", "ConsoleApp",
		"./",
		"../build/${NAME}/",
		env, configs
	)
	if not srcglob then
		srcglob = name .. ".cpp"
	end

	configuration {"linux"}
		targetsuffix(".elf")

	configuration {}
		targetname(name)
		files {
			srcglob
		}
end

function quanta.make_tests(group, tests)
	precore.push_wd(group)
	for name, test in pairs(tests) do
		quanta.make_test(group, name, test[1], test[2])
	end
	precore.pop_wd()
end

precore.make_config("quanta.strict", nil, {
{project = function()
	-- NB: -Werror is a pita for GCC. Good for testing, though,
	-- since its error checking is better.
	configuration {"clang"}
		flags {
			"FatalWarnings",
		}
		buildoptions {
			"-Wno-extra-semi",
		}

	configuration {"linux"}
		buildoptions {
			"-pedantic-errors",
			"-Wextra",

			"-Wuninitialized",
			"-Winit-self",

			"-Wmissing-field-initializers",
			"-Wredundant-decls",

			"-Wfloat-equal",
			"-Wold-style-cast",

			"-Wnon-virtual-dtor",
			"-Woverloaded-virtual",

			"-Wunused",
			"-Wundef",
		}
end}})

precore.make_config("quanta.base", nil, {
"togo.base",
{project = function(p)
	configuration {"linux"}
		buildoptions {
			"-pthread",
		}

	if not precore.env_project()["NO_LINK"] then
		links {
			"m",
			"dl",
			"pthread",
		}
	end

	configuration {"debug"}
		defines {
			"QUANTA_DEBUG",
		}
end}})

precore.make_config_scoped("quanta.projects", {
	once = true,
}, {
{global = function()
	precore.make_solution(
		"quanta",
		{"debug", "release"},
		{"x64", "x32"},
		nil,
		{
			"precore.generic",
		}
	)

	local env = {
		NO_LINK = true,
	}
	local configs = {
		"quanta.strict",
	}
	for _, name in pairs(quanta.libs) do
		table.insert(configs, 1, "quanta.lib." .. name .. ".dep")
	end
	for _, name in pairs(quanta.apps) do
		table.insert(configs, 1, "quanta.app." .. name .. ".dep")
	end

	precore.make_project(
		"igen",
		"C++", "StaticLib",
		"build/igen/", "build/igen/",
		env, configs
	)

	configuration {"gmake"}
		prebuildcommands {
			"$(SILENT) mkdir -p ./tmp",
			"$(SILENT) ./scripts/run_igen.py -- $(ALL_CXXFLAGS)",
			"$(SILENT) exit 0",
		}
end}})

quanta.libs = quanta_libs()
quanta.apps = quanta_apps()

for _, name in pairs(quanta.libs) do
	precore.import(G"${QUANTA_ROOT}/lib/" .. name)
end
for _, name in pairs(quanta.apps) do
	precore.import(G"${QUANTA_ROOT}/app/" .. name)
end
