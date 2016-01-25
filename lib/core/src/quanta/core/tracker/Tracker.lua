u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local M = U.module(...)

M.debug = false

U.class(M)

function M:__init()
	self.date = T()
	self.entries = {}
end

function M:from_object(obj, director)
	U.type_assert(obj, "userdata")

	T.clear(self.date)
	self.entries = {}

	local context = Match.Context()
	context.user = {
		tracker = self,
		director = director,
		obj_tmp = O.create(),
	}
	if not context:consume(M.t_head, obj, self) then
		return false, context.error:to_string()
	end
	return self:validate_and_fixup()
end

-- TODO
--[[function M:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	else
		O.clear(obj)
	end

	
	return obj
end--]]

local function obj_error(obj, msg, ...)
	return false, string.format(
		"%s\nat object: ```\n%s\n```",
		string.format(msg, ...),
		O.write_text_string(obj, true)
	)
end

-- TODO: build and solve a dependency graph

local function find_by_ref(entries, start, direction, n, ool)
	for i = start + direction, direction < 0 and 1 or #entries, direction do
		local entry = entries[i]
		if entry.ool == ool then
			n = n - 1
		end
		if n == 0 then
			return entry
		end
	end
	return nil
end

local function fixup_time_ref(self, i, entry, time)
	if time.type < M.EntryTime.Type.ref_prev then
		return true
	end

	local direction = time.type == M.EntryTime.Type.ref_prev and -1 or 1
	local ref_entry = find_by_ref(self.entries, i, direction, entry.index, entry.ool)
	if not ref_entry then
		return obj_error(entry.obj, "entry reference index is out-of-bounds")
	end

	local ref_time = ref_entry.r_start
	if ref_time.type ~= M.EntryTime.Type.specified then
		return obj_error(entry.obj, "entry referent has an unresolved start time")
	end

	T.set(time.time, T.value(ref_time.time))
	return true
end

function M:validate_and_fixup()
	if T.value(self.date) == 0 then
		return false, "date is unset"
	end
	local success, msg
	for i, entry in ipairs(self.entries) do
		success, msg = fixup_time_ref(self, i, entry, entry.r_start)
		if not success then return false, msg end
		success, msg = fixup_time_ref(self, i, entry, entry.r_end)
		if not success then return false, msg end

		entry:recalculate()
		local duration = T.value(entry.duration)
		if duration < 0 then
			return obj_error(entry.obj, "entry range is degenerate: duration is negative")
		elseif duration == 0 then
			return obj_error(entry.obj, "entry range is degenerate: duration is zero")
		end
		entry.obj = nil
	end
	return true
end

M.Action = U.class(M.Action)

function M.Action:__init()
	self.id = nil
	self.id_hash = O.NAME_NULL
	self.data = nil
end

M.Action.t_head = Match.Tree()

M.Action.p_accum = Match.Pattern{
	any_branch = M.Action.t_head,
	acceptor = function(context, entry, obj)
		local action = M.Action()
		table.insert(entry.actions, action)
		return action
	end,
}

-- ETODO
M.Action.t_head:add(Match.Pattern{
	vtype = O.Type.identifier,
	value = "ETODO",
	children = {Match.Pattern{
		name = {"d", ""},
		vtype = O.Type.string,
		acceptor = function(context, self, obj)
			self.data.description = O.string(obj)
		end,
	}},
	acceptor = function(context, self, obj)
		if O.num_children(obj) > 1 then
			return Match.Error("ETODO can only carry a single string")
		end

		self.id = "ETODO"
		self.id_hash = O.hash_name(self.id)
		self.data = {
			description = "",
		}
	end,
})

-- action
M.Action.t_head:add(Match.Pattern{
	vtype = O.Type.identifier,
	acceptor = function(context, self, obj)
		return context.user.director:read_action(context, self, obj)
	end,
})

M.EntryTime = U.class(M.EntryTime)

M.EntryTime.Type = {
	specified		= 1,
	placeholder		= 2, -- XXX
	ref_prev		= 3, -- EPREV
	ref_next		= 4, -- ENEXT
}

function M.EntryTime:__init()
	self.type = M.EntryTime.Type.placeholder
	self.time = T()
	self.ool = false
	self.index = 0
	self.approximation = 0
	self.certain = true
end

M.EntryTime.t_head = Match.Tree()

local function entry_time_set_uncertainties(self, obj)
	self.approximation = O.value_approximation(obj)
	self.certain = not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))
end

-- time
M.EntryTime.t_head:add(Match.Pattern{
	vtype = O.Type.time,
	acceptor = function(context, self, obj)
		if O.is_zoned(obj) then
			return Match.Error("entry range endpoint must not be zoned")
		elseif not O.has_clock(obj) then
			return Match.Error("entry range endpoint must have clock time")
		end

		self.type = M.EntryTime.Type.specified

		O.copy(context.user.obj_tmp, obj, false)
		O.resolve_time(context.user.obj_tmp, context.user.tracker.date)
		T.set(self.time, O.time(context.user.obj_tmp))
		entry_time_set_uncertainties(self, obj)
	end,
})

-- XXX
M.EntryTime.t_head:add(Match.Pattern{
	vtype = O.Type.identifier,
	value = "XXX",
	acceptor = function(context, self, obj)
		self.type = M.EntryTime.Type.placeholder
	end,
})

local et_ref_type_by_name = {
	["EPREV"] = M.EntryTime.Type.ref_prev,
	["ENEXT"] = M.EntryTime.Type.ref_next,
}

-- EPREV, ENEXT
M.EntryTime.t_head:add(Match.Pattern{
	vtype = O.Type.identifier,
	value = {"EPREV", "ENEXT"},
	tags = {Match.Pattern{
		name = "ool",
		acceptor = function(context, self, obj)
			self.ool = true
		end,
	}},
	-- ...[>0]
	quantity = {Match.Pattern{
		vtype = O.Type.integer,
		value = function(_, _, obj, _)
			if O.integer(obj) <= 0 then
				return Match.Error("entry time ref index must be greater than zero")
			end
			return true
		end,
		acceptor = function(context, self, obj)
			self.index = O.integer(obj)
		end,
	}},
	acceptor = function(context, self, obj)
		self.type = et_ref_type_by_name[O.identifier(obj)]
		entry_time_set_uncertainties(self, obj)
		if not O.has_quantity(obj) then
			self.index = 1
		end
	end,
})

M.Entry = U.class(M.Entry)

function M.Entry:__init()
	self.ool = false
	self.duration = T()
	self.r_start = M.EntryTime()
	self.r_end = M.EntryTime()
	self.tags = {}
	self.rel_id = {}
	self.continue_scope = nil
	self.continue_id = nil
	self.actions = {}
end

function M.Entry:recalculate()
	if
		self.r_start.type == M.EntryTime.Type.specified and
		self.r_end.type == M.EntryTime.Type.specified
	then
		T.set(self.duration, T.difference(self.r_end.time, self.r_start.time))
	else
		T.clear(self.duration)
	end
end

M.Entry.t_head_tags = Match.Tree()
M.Entry.t_body = Match.Tree()

M.Entry.p_head = Match.Pattern{
	vtype = O.Type.identifier,
	value = "Entry",
	tags = M.Entry.t_head_tags,
	children = M.Entry.t_body,
	acceptor = function(context, tracker, obj)
		local entry = M.Entry()
		table.insert(tracker.entries, entry)
		entry.obj = obj
		return entry
	end,
}

-- Entry:ool
M.Entry.t_head_tags:add(Match.Pattern{
	name = "ool",
	acceptor = function(context, self, obj)
		self.ool = true
	end,
})

-- range = (EntryTime - EntryTime)
M.Entry.t_body:add(Match.Pattern{
	name = "range",
	vtype = O.Type.expression,
	acceptor = function(context, self, obj)
		if O.num_children(obj) ~= 2 then
			return Match.Error("range must have two elements")
		elseif O.op(O.child_at(obj, 2)) ~= O.Operator.sub then
			return Match.Error("range operator must be a subtraction")
		end

		if not context:consume(M.EntryTime.t_head, O.child_at(obj, 1), self.r_start) then
			return
		end
		if not context:consume(M.EntryTime.t_head, O.child_at(obj, 2), self.r_end) then
			return
		end
	end,
})

-- tags = {...}
M.Entry.t_body:add(Match.Pattern{
	name = "tags",
	children = {Match.Pattern{
		vtype = {O.Type.identifier, O.Type.string},
		acceptor = function(context, self, obj)
			table.insert(self.tags, O.text(obj))
		end,
	}},
})

-- rel_id = ...
M.Entry.t_body:add(Match.Pattern{
	name = "rel_id",
	children = {Match.Pattern{
		vtype = O.Type.identifier,
		acceptor = function(context, self, obj)
			table.insert(self.rel_id, O.identifier(obj))
		end,
	}},
})

-- rel_id = {...}
M.Entry.t_body:add(Match.Pattern{
	name = "rel_id",
	vtype = O.Type.identifier,
	acceptor = function(context, self, obj)
		table.insert(self.rel_id, O.identifier(obj))
	end,
})

-- continue_id = identifier
M.Entry.t_body:add(Match.Pattern{
	name = "continue_id",
	vtype = O.Type.identifier,
	acceptor = function(context, self, obj)
		self.continue_scope = nil
		self.continue_id = O.identifier(obj)
	end,
})

-- continue_id = date{identifier}
M.Entry.t_body:add(Match.Pattern{
	name = "continue_id",
	vtype = O.Type.time,
	children = 1,
	acceptor = function(context, self, obj)
		if O.has_clock(obj) then
			return Match.Error("scope must not have clock time")
		end

		O.copy(context.user.obj_tmp, obj, false)
		O.resolve_time(context.user.obj_tmp, context.user.tracker.date)
		self.continue_scope = T(O.time(context.user.obj_tmp))
		self.continue_id = O.identifier(O.child_at(obj, 1))
	end,
})

-- actions = {...}
M.Entry.t_body:add(Match.Pattern{
	name = "actions",
	children = {M.Action.p_accum},
	acceptor = function(context, self, obj)
		if not O.has_children(obj) then
			return Match.Error("entry actions must be non-empty when specified")
		end
	end,
})

M.t_head = Match.Tree()
M.t_body = Match.Tree()

-- Tracker{...}
M.t_head:add(Match.Pattern{
	vtype = O.Type.identifier,
	value = "Tracker",
	children = M.t_body,
})

-- date = date{:full:zoned}
M.t_body:add(Match.Pattern{
	name = "date",
	vtype = O.Type.time,
	acceptor = function(context, self, obj)
		if O.is_month_contextual(obj) then
			return Match.Error("date must be full")
		elseif not O.is_zoned(obj) then
			return Match.Error("date must be zoned")
		elseif O.has_clock(obj) then
			return Match.Error("date must not have clock time")
		end

		T.set(self.date, O.time(obj))
	end,
})

-- entries = list{Entry}
M.t_body:add(Match.Pattern{
	name = "entries",
	children = {
		-- M.SystemData.p_head,
		M.Entry.p_head,
	},
	acceptor = function(context, self, obj)
		if T.value(self.date) == 0 then
			-- TODO: pre-match to avoid this
			return Match.Error("date must be set before entries")
		end
	end,
})

return M

)"__RAW_STRING__"