
local S, G, R = precore.helpers()

precore.make_config("quanta.app.tool.dep", {
	reverse = true,
}, {
"quanta.base",
"quanta.lib.core.dep",
{project = function(p)
	quanta.app_config("tool")
end}})

precore.append_config_scoped("quanta.projects", {
{global = function(_)
	quanta.make_app("tool", {
		"quanta.app.tool.dep",
	})
end}})
