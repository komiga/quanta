u8R""__RAW_STRING__(

local U = require "togo.utility"
local FS = require "togo.filesystem"
local M = U.module(...)

local function check_initialized()
	U.assertl(1, M.initialized, "Quanta.Vessel has not been initialized")
end

function M.active_bucket()
	return M.work_local and "local" or "vessel"
end

function M.path(...)
	check_initialized()
	return U.join_paths(M.root, ...)
end

function M.log_path(...) return M.path(M.active_bucket(), "log", ...) end
function M.sys_path(...) return M.path(M.active_bucket(), "sys", ...) end
function M.data_path(...) return M.path(M.active_bucket(), "data", ...) end
function M.script_path(...) return M.path(M.active_bucket(), "scripts", ...) end
function M.sublime_path(...) return M.path(M.active_bucket(), "sublime", ...) end

function M.set_work_local(work_local)
	check_initialized()
	U.type_assert(work_local, "boolean")
	M.work_local = work_local
end

function M.init(root_path, work_local)
	local root = U.type_assert(root_path, "string", true) or os.getenv("QUANTA_ROOT")
	U.assert(root, "vessel root not provided and not found ($QUANTA_ROOT)")

	M.initialized = true
	M.root = root
	M.set_work_local(U.optional(work_local, true))
	U.assert(FS.is_directory(M.root))
end

return M

)"__RAW_STRING__"