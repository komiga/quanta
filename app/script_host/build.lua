
local S, G, R = precore.helpers()

precore.make_config("quanta.app.script_host.dep", {
	reverse = true,
}, {
"quanta.base",
"quanta.lib.core.dep",
{project = function(p)
	quanta.app_config("script_host")
end}})

precore.append_config_scoped("quanta.projects", {
{global = function(_)
	quanta.make_app("script_host", {
		"quanta.app.script_host.dep",
	})
end}})
