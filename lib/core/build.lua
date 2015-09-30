
local S, G, R = precore.helpers()

precore.make_config("quanta.lib.core.luajit2.dep", nil, {
{project = function(p)
	configuration {}
		includedirs {
			G"${DEP_PATH}/luajit/include/",
		}

	if not precore.env_project()["NO_LINK"] then
		libdirs {
			G"${DEP_PATH}/luajit/lib/",
		}
		links {
			":libluajit-5.1.a",
		}
	end
end}})

precore.make_config("quanta.lib.core.dep", {
	reverse = true,
}, {
"quanta.base",
"togo.lib.core.dep",
"quanta.lib.core.luajit2.dep",
{project = function(p)
	quanta.library_config("core")
end}})

precore.append_config_scoped("quanta.projects", {
{global = function(_)
	quanta.make_library("core", {
		"quanta.lib.core.dep",
	})
end}})
