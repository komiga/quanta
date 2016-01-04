
local S, G, R = precore.helpers()

precore.make_config("quanta.lib.core.dep", {
	reverse = true,
}, {
"quanta.base",
"togo.lib.core.dep",
{project = function(p)
	quanta.library_config("core")
end}})

precore.append_config_scoped("quanta.projects", {
{global = function(_)
	quanta.make_library("core", {
		"quanta.lib.core.dep",
	})
end}})
