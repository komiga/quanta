u8R""__RAW_STRING__(

local U = require "togo.utility"
local M = U.module(...)

M.debug = false

M.SECS_PER_MINUTE = 60
M.SECS_PER_HOUR = 60 * 60
M.SECS_PER_DAY = 24 * 60 * 60
M.SECS_PER_WEEK = 7 * M.SECS_PER_DAY

function M.__module_init__()
	U.set_functable(M, function(_, ...)
		return M.__mm_ctor(...)
	end)
end

return M

)"__RAW_STRING__"