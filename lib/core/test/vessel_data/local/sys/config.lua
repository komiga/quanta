
local Vessel = require "Quanta.Vessel"
local Director = require "Quanta.Director"

Director.debug = true

Vessel.setup_config(function(_ENV)
	director = Director()
end)
