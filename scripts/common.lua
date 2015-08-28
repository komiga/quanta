
-- FIXME: u g h, why
if precore then
	dofile("../dep/togo/scripts/common.lua")
else
	dofile("dep/togo/scripts/common.lua")
end

function quanta_libs()
	return {
		"core",
	}
end

function quanta_apps()
	return {
	}
end
