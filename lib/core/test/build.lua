
local S, G, R = precore.helpers()

local configs = {
	"quanta.lib.core.dep",
}

togo.make_tests("object", {
	["general"] = {nil, configs},
})

togo.make_tests("string", {
	["general"] = {nil, configs},
})
