
local S, G, R = precore.helpers()

local configs = {
	"quanta.lib.core.dep",
}

togo.make_tests("object", {
	["general"] = {nil, configs},
	["io_text"] = {nil, configs},
})

togo.make_tests("string", {
	["general"] = {nil, configs},
})

togo.make_tests("chrono", {
	["time"] = {nil, configs},
})
