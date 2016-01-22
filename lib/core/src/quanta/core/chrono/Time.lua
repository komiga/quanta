u8R""__RAW_STRING__(

local U = require "togo.utility"
local M = U.module(...)

M.debug = false

function M.__module_init__()
	U.set_functable(M, M.__mm_ctor)
end

return M

)"__RAW_STRING__"