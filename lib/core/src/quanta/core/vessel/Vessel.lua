u8R""__RAW_STRING__(

local U = require "togo.utility"
local FS = require "togo.filesystem"
local IO = require "togo.io"
local T = require "Quanta.Time"
require "Quanta.Time.Gregorian"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local M = U.module(...)

M.exports = {}
M.group = {
	core = {
		path = nil,
	},
	user = {
		path = nil,
	},
	vessel = {
		path = nil,
	},
}
M.active_group = M.group.user
do
	table.insert(M.group, M.group.core)
	table.insert(M.group, M.group.user)
	table.insert(M.group, M.group.vessel)
end

local function check_initialized()
	U.assertl(1, M.initialized, "Quanta.Vessel has not been initialized")
end

function M.innermost_group()
	return (
		M.group.vessel.path and M.group.vessel or
		M.group.user.path and M.group.user or
		M.group.core.path and M.group.core or
		nil
	)
end

function M.set_group(g)
	U.type_assert(g, "table")
	U.type_assert(g.path, "string")
	M.active_group = g
	U.assert(FS.is_directory(M.active_group.path))
end

function M.set_work_local(work_local)
	check_initialized()
	U.type_assert(work_local, "boolean")
	M.work_local = work_local
end

function M.active_bucket()
	if M.active_group == M.group.vessel then
		return M.work_local and "local" or "vessel"
	end
	return ""
end

function M.path(...)
	check_initialized()
	return U.join_paths(M.active_group.path, ...)
end

function M.log_path(...) return M.path(M.active_bucket(), "log", ...) end
function M.sys_path(...) return M.path(M.active_bucket(), "sys", ...) end
function M.data_path(...) return M.path(M.active_bucket(), "data", ...) end
function M.script_path(...) return M.path(M.active_bucket(), "scripts", ...) end
function M.sublime_path(...) return M.path(M.active_bucket(), "sublime", ...) end

function M.data_chrono_path(...) return M.data_path("chrono", ...) end

function M.init(vessel_path, work_local)
	local core_path = os.getenv("QUANTA_CORE")
	local user_path = os.getenv("QUANTA_USER")
	U.assert(core_path, "core not found ($QUANTA_CORE)")

	local active_env = nil
	if not vessel_path then
		active_env = os.getenv("QUANTA_ACTIVE")
		vessel_path = os.getenv("QUANTA_ROOT")
		U.assert(vessel_path, "vessel path not provided and not found ($QUANTA_ROOT)")
	end

	M.initialized = true
	M.group.core.path = core_path
	M.group.user.path = user_path
	M.group.vessel.path = vessel_path
	M.active_env = active_env
	M.set_work_local(U.optional(work_local, true))
	M.set_group(M.innermost_group())

	M.reload_config()
end

local BYTE_SLASH = string.byte('/')

local function add_export(p)
	if not M.exports[p] then
		M.exports[p] = true
		package.path = package.path .. ';' .. p
	end
end

function M.export(...)
	local paths = {...}
	local wd = FS.working_dir()
	add_export(U.join_paths(wd, "?.lua"))
	for _, p in ipairs(paths) do
		if string.byte(p, 1) ~= BYTE_SLASH then
			add_export(U.join_paths(wd, p))
		else
			add_export(p)
		end
	end
end

function M.with_group(g, f, ...)
	U.type_assert(g, "table")
	U.type_assert(g.path, "string")
	local prev_active_group = M.active_group
	M.active_group = g
	f(...)
	M.active_group = prev_active_group
end

function M.setup_config(f)
	check_initialized()
	return f(M.config)
end

function M.reload_config()
	check_initialized()

	M.config = {}
	for _, g in ipairs(M.group) do
		M.with_group(g, function()
			local path = M.sys_path("config.lua")
			if not FS.is_file(path) then
				return
			end

			local source = IO.read_file(path)
			if source == nil then
				error("failed to read config: %s", path)
			end
			local chunk, err = load(source, "@" .. path, "t")
			if chunk == nil then
				error("failed to parse config: %s:", path)
			end

			FS.working_dir_scope(M.sys_path(), function()
				chunk()
			end)
		end)
	end
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

return M

)"__RAW_STRING__"