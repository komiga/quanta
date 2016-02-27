u8R""__RAW_STRING__(

local U = require "togo.utility"
local FS = require "togo.filesystem"
local IO = require "togo.io"
local T = require "Quanta.Time"
require "Quanta.Time.Gregorian"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
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

function M.data_chrono_path(...) return M.data_path("chrono", ...) end

local function first_line(s)
	local b, _ = string.find(s, "\n")
	return string.sub(s, 1, (b or 0) - 1)
end

function M.tracker_active_date()
	local date = T()
	local slug = IO.read_file(M.data_chrono_path("active"))
	if slug then
		slug = first_line(slug)
		local obj = O.create(string.gsub(slug, "/", "-"))
		if obj and O.is_time(obj) and O.has_date(obj) then
			T.set(date, O.time(obj))
			T.clear_clock(date)
		else
			T.G.set_date_now(date)
		end
	else
		T.G.set_date_now(date)
	end
	return date
end

function M.tracker_path(date)
	check_initialized()
	if date ~= nil then
		U.type_assert(date, "userdata")
		local y, m, d = T.G.date_utc(date)
		return M.data_chrono_path(string.format("%04d/%02d/%02d.q", y, m, d))
	else
		local slug = IO.read_file(M.data_chrono_path("active"))
		U.assert(slug ~= nil, "failed to read active tracker date")
		return M.data_chrono_path(first_line(slug) .. ".q")
	end
end

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
	U.assert(FS.is_file(M.sys_path("config.lua")))
	M.reload_config()
end

function M.setup_config(f)
	check_initialized()

	setfenv(f, M.config)
	return f()
end

function M.reload_config()
	check_initialized()
	M.config = nil

	local path = M.sys_path("config.lua")
	local source = IO.read_file(path)
	if source == nil then
		error("failed to read config: %s", path)
	end
	local chunk, err = load(source, "@" .. path, "t")
	if chunk == nil then
		error("failed to parse config: %s:", path)
	end

	M.config = {}
	FS.working_dir_scope(M.sys_path(), function()
		chunk()
	end)

	U.type_assert(M.config, "table")
	U.type_assert(M.config.director, require("Quanta.Director"))
end

function M.new_match_context(implicit_scope)
	U.type_assert(implicit_scope, "userdata", true)
	check_initialized()

	local context = Match.Context()
	context.user = context.user or {}
	context.user.director = M.config.director
	context.user.implicit_scope = implicit_scope and T(implicit_scope) or nil
	context.user.scope_save = {}
	context.user.scope = {}
	return context
end

return M

)"__RAW_STRING__"