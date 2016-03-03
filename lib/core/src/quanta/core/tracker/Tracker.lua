u8R""__RAW_STRING__(

local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Vessel = require "Quanta.Vessel"
local Prop = require "Quanta.Prop"
local Instance = require "Quanta.Instance"
local M = U.module(...)

M.debug = false

U.class(M)

function M:__init()
	self.date = T()
	self.entries = {}
	self.entry_groups = {}
	self.entry_by_marker = {}
	self.attachments = {}
end

function M:from_object(obj)
	U.type_assert(obj, "userdata")

	T.clear(self.date)
	self.entries = {}
	self.entry_groups = {}
	self.entry_by_marker = {}
	self.attachments = {}

	local context = Vessel.new_match_context(self.date)
	context.user.tracker = self
	if not context:consume(M.t_head, obj, self) then
		return false, context.error:to_string(), context.error.source_line
	end
	return self:validate_and_fixup()
end

function M:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	else
		O.clear(obj)
	end

	O.set_identifier(obj, "Tracker")

	local date_obj = O.push_child(obj)
	O.set_name(date_obj, "date")
	O.set_time_date(date_obj, self.date)

	local entries_obj = O.push_child(obj)
	O.set_name(entries_obj, "entries")

	for _, attachment in ipairs(self.attachments) do
		attachment:to_object(O.push_child(entries_obj))
	end
	for _, entry in ipairs(self.entries) do
		entry:to_object(O.push_child(entries_obj), self.date)
	end

	return obj
end

local function entry_error(entry, msg, ...)
	msg = string.format(
		"%s\nat object (line %d): ```\n%s\n```",
		string.format(msg, ...),
		entry.source_line,
		O.write_text_string(entry.obj, true)
	)
	return false, msg, entry.source_line
end

local function fixup_spillover(entry_start, r_start, entry_end, r_end)
	if
		T.value(r_start.time) > T.value(r_end.time) and
		T.date_seconds_utc(r_start.time) >= T.date_seconds_utc(r_end.time)
	then
		if T.difference(r_start.time, r_end.time) <= (8 * T.SECS_PER_HOUR) then
			if entry_start == entry_end then
				return entry_error(entry_start, "negative entry range appears to be typoed (end is within 8h before start)")
			else
				return entry_error(entry_start, "subsequent entry is non-chronological (end is within 8h before start)")
			end
		end
		T.add(r_end.time, T.SECS_PER_DAY)
	end
	return true
end

local function find_by_ref(entries, start, n, ool)
	local direction = n < 0 and -1 or 1
	local target = direction < 0 and 1 or #entries
	for i = start + direction, target, direction do
		local entry = entries[i]
		if entry.ool == ool then
			n = n - direction
			if n == 0 then
				return entry, i
			end
		end
	end
	return nil
end

local function fixup_time_ref(self, i, entry, time, part, no_branch)
	local ref_entry, ref_i
	if time.type == M.EntryTime.Type.specified then
		return true
	elseif time.type == M.EntryTime.Type.placeholder then
		return entry_error(entry, "entry range %s is unspecified", part)
	elseif time.type == M.EntryTime.Type.marker then
		local ref = self.entry_by_marker[time.marker]
		if not ref then
			return entry_error(entry, "entry range %s referent '%s' (marker) does not exist", part, time.marker)
		end
		ref_entry = ref.entry
		ref_i = ref.index
		if ref_i < i then
			return entry_error(entry, "entry range %s referent '%s' (marker) precedes its entry", part, time.marker)
		elseif ref_entry == entry then
			return entry_error(entry, "entry range %s refers to itself by marker '%s'", part, time.marker)
		end
	else -- ref
		ref_entry, ref_i = find_by_ref(self.entries, i, time.index, time.ool)
		if not ref_entry then
			return entry_error(entry, "entry range %s reference index is out-of-bounds", part)
		end
	end

	local ref_time = ref_entry.r_start
	if ref_time.type ~= M.EntryTime.Type.specified then
		-- total unreliable kludge! go after that dependency graph solution
		if not no_branch then
			local success, msg, source_line = fixup_time_ref(self, ref_i, ref_entry, ref_entry.r_start, "start", true)
			if not success then return false, msg, source_line end
		end
		if ref_time.type ~= M.EntryTime.Type.specified then
			return entry_error(entry, "entry range %s referent has an unresolved start time", part)
		end
	end

	time.type = M.EntryTime.Type.specified
	T.set(time.time, T.value(ref_time.time))
	time.ool = false
	time.index = 0
	time.marker = nil
	if time.approximation == 0 and time.certain then
		time.approximation = ref_time.approximation
		time.certain = ref_time.certain
	end
	return true
end

function M:validate_and_fixup()
	if T.value(self.date) == 0 then
		return false, "date is unset", 0
	end
	local success, msg, source_line
	local prev_entry = nil
	for i, entry in ipairs(self.entries) do
		if entry.r_start.type == M.EntryTime.Type.specified then
			if prev_entry then
				success, msg, source_line = fixup_spillover(prev_entry, prev_entry.r_start, entry, entry.r_start)
				if not success then return false, msg, source_line end
			end
			prev_entry = entry
		end
	end

	for i, entry in ipairs(self.entries) do
		-- TODO: build and solve a dependency graph
		if
			entry.r_start.type == M.EntryTime.Type.placeholder and
			entry.r_end.type == M.EntryTime.Type.placeholder
		then
			return entry_error(entry, "entry range is incomplete or unspecified")
		end

		entry:fixup()
		success, msg, source_line = fixup_time_ref(self, i, entry, entry.r_start, "start")
		if not success then return false, msg, source_line end
		success, msg, source_line = fixup_time_ref(self, i, entry, entry.r_end, "end")
		if not success then return false, msg, source_line end

		success, msg, source_line = fixup_spillover(entry, entry.r_start, entry, entry.r_end)
		if not success then return false, msg, source_line end

		entry:recalculate()
		local duration = T.value(entry.duration)
		if duration < 0 then
			return entry_error(entry, "entry range is degenerate: duration is negative")
		elseif duration == 0 then
			return entry_error(entry, "entry range is degenerate: duration is zero")
		end

		if entry.continue_id then
			if T.compare_equal(entry.continue_scope, self.date) then
				local group = self.entry_groups[entry.continue_id]
				if not group then
					if #entry.actions == 0 then
						return entry_error(entry, "local continue group head entry carries no actions")
					end
					group = {}
					self.entry_groups[entry.continue_id] = group
				end
				table.insert(group, entry)
			end
		elseif #entry.actions == 0 then
			return entry_error(entry, "entry is empty: carries no actions and doesn't belong to a continue group")
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

function M.Action:to_object(obj, is_primary)
	U.type_assert(obj, "userdata", true)
	U.type_assert(is_primary, "boolean", true)
	if not obj then
		obj = O.create()
	else
		O.clear(obj)
	end

	O.set_identifier(obj, self.id or "UnknownAction")
	self.data:to_object(self, obj)

	if is_primary then
		O.set_name(O.push_tag(obj), "action_primary")
	end
	return obj
end

function M.Action.remove_internal_tags(obj)
	local function remove_tag(name)
		local tag = O.find_tag(obj, name)
		if tag then
			O.remove_tag(obj, tag)
		end
	end

	remove_tag("action_primary")
end

M.Action.t_ignore_internal_tags = Match.Tree({
Match.Pattern{name = "action_primary"},
})

local function add_action(list, obj)
	local action = M.Action()
	action.id = O.identifier(obj)
	action.id_hash = O.identifier_hash(obj)
	table.insert(list, action)

	return action
end

function M.Action.struct_list(list)
	U.type_assert(list, "table")
	return list
end

function M.Action.adapt_struct_list(serialized_name, property_name)
	local function element_pattern(name)
		return Match.Pattern{
			name = name,
			vtype = O.Type.identifier,
			tags = Match.Any,
			children = Match.Any,
			acceptor = function(context, parent, obj)
				local action = add_action(parent[property_name], obj)
				return parent:read_action(context, action, obj)
			end,
		}
	end

	local t_head = Match.Tree({
	-- = ...
	element_pattern(serialized_name),
	-- = ... + ...
	-- = {...}
	Match.Pattern{
		name = serialized_name,
		vtype = {O.Type.null, O.Type.expression},
		children = {
			element_pattern(nil),
		},
	},
	})
	t_head:build()

	return Prop.adapt_list_to_object(M.Action, serialized_name), t_head
end

M.Action.p_head = Match.Pattern{
	vtype = O.Type.identifier,
	tags = Match.Any,
	children = Match.Any,
	acceptor = function(context, entry, obj)
		local action = add_action(entry.actions, obj)
		return context.user.director:read_action(context, entry, action, obj)
	end,
	post_branch = function(context, entry, obj)
		local tag_action_primary = O.find_tag(obj, "action_primary")
		if tag_action_primary then
			if entry.primary_action then
				return Match.Error("entry already has a primary action")
			end
			entry.primary_action = #entry.actions
		end
	end,
}

M.PlaceholderAction = U.class(M.PlaceholderAction)

function M.PlaceholderAction:__init(description)
	self.description = description or ""
end

function M.PlaceholderAction:from_object(context, entry, action, obj)
	return context:consume(M.PlaceholderAction.t_head, obj, self)
end

function M.PlaceholderAction:to_object(action, obj)
	if self.description and #self.description > 0 then
		local d = O.push_child(obj)
		O.set_string(d, self.description)
	end
end

function M.PlaceholderAction:compare_equal(other)
	return self.description == other.description
end

M.PlaceholderAction.t_head = Match.Tree({
Match.Pattern{
	vtype = Match.Any,
	tags = Match.Any,
	children = {Match.Pattern{
		name = {"d", ""},
		vtype = O.Type.string,
		acceptor = function(context, self, obj)
			self.description = O.string(obj)
		end,
	}},
	acceptor = function(context, self, obj)
		if O.num_children(obj) > 1 then
			return Match.Error("placeholder action can only carry a single string")
		end
	end,
},
})

M.PlaceholderAction.t_head:build()

M.UnknownAction = U.class(M.UnknownAction)

function M.UnknownAction:__init()
	self.obj = O.create()
end

function M.UnknownAction:from_object(context, entry, action, obj)
	O.copy_children(self.obj, obj)
	O.copy_tags(self.obj, obj)
	M.Action.remove_internal_tags(self.obj)
end

function M.UnknownAction:to_object(action, obj)
	O.copy_children(obj, self.obj)
	O.copy_tags(obj, self.obj)
end

function M.UnknownAction:compare_equal(other)
	-- TODO?
	return true
end

M.EntryTime = U.class(M.EntryTime)

M.EntryTime.Type = {
	specified		= 1,
	placeholder		= 2, -- XXX
	ref				= 3, -- EPREV, ENEXT
	marker			= 4, -- identifier
}

function M.EntryTime:__init()
	self.type = M.EntryTime.Type.placeholder
	self.time = T()
	self.ool = false
	self.index = 0
	self.marker = nil
	self.approximation = 0
	self.certain = true
end

function M.EntryTime:to_object(obj, scope)
	U.type_assert(obj, "userdata", true)
	U.type_assert(scope, "userdata", true)
	if not obj then
		obj = O.create()
	else
		O.clear(obj)
	end

	if self.type == M.EntryTime.Type.specified then
		O.set_time(obj, self.time)
		if scope then
			O.reduce_time(obj, scope)
		end
	elseif self.type == M.EntryTime.Type.placeholder then
		O.set_identifier(obj, "XXX")
	elseif self.type == M.EntryTime.Type.ref then
		if self.index < 0 then
			O.set_identifier(obj, "EPREV")
		else
			O.set_identifier(obj, "ENEXT")
		end
		-- NB: index == 0 is actually an error
		local abs_index = math.abs(self.index)
		if abs_index > 1 then
			O.set_integer(O.make_quantity(obj), abs_index)
		end
	elseif self.type == M.EntryTime.Type.marker then
		O.set_identifier(obj, self.marker)
	end

	if self.ool then
		O.set_name(O.push_tag(obj), "ool")
	end
	O.set_value_certain(obj, self.certain)
	O.set_value_approximation(obj, self.approximation)
	return obj
end

local function entry_time_set_uncertainties(self, obj)
	self.approximation = O.value_approximation(obj)
	self.certain = not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))
end

local et_ref_type_by_name = {
	["EPREV"] = M.EntryTime.Type.ref,
	["ENEXT"] = M.EntryTime.Type.ref,
}

M.EntryTime.t_head = Match.Tree({
-- time
Match.Pattern{
	vtype = O.Type.time,
	acceptor = function(context, self, obj)
		if O.is_zoned(obj) then
			return Match.Error("entry range endpoint must not be zoned")
		elseif not O.has_clock(obj) then
			return Match.Error("entry range endpoint must have clock time")
		end

		self.type = M.EntryTime.Type.specified

		T.set(self.time, O.time_resolved(obj, context.user.tracker.date))
		entry_time_set_uncertainties(self, obj)
	end,
},
-- XXX
Match.Pattern{
	vtype = O.Type.identifier,
	value = "XXX",
	acceptor = function(context, self, obj)
		self.type = M.EntryTime.Type.placeholder
	end,
},
-- EPREV, ENEXT
Match.Pattern{
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
			self.index = self.index * O.integer(obj)
		end,
	}},
	acceptor = function(context, self, obj)
		self.type = M.EntryTime.Type.ref
		if O.identifier(obj) == "EPREV" then
			self.index = -1
		else
			self.index = 1
		end
		entry_time_set_uncertainties(self, obj)
	end,
},
-- identifier
Match.Pattern{
	vtype = O.Type.identifier,
	acceptor = function(context, self, obj)
		self.type = M.EntryTime.Type.marker
		self.marker = O.identifier(obj)
		entry_time_set_uncertainties(self, obj)
	end,
},
})

M.EntryTime.t_head:build()

M.Entry = U.class(M.Entry)

function M.Entry:__init()
	self.source_line = 0
	self.ool = false
	self.duration = T()
	self.r_start = M.EntryTime()
	self.r_end = M.EntryTime()
	self.tags = {}
	self.rel_id = {}
	self.continue_id = nil
	self.continue_scope = nil
	self.marker = nil
	self.actions = {}
	self.primary_action = nil
end

function M.Entry:fixup()
	if not self.primary_action and #self.actions > 0 then
		self.primary_action = 1
	end
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

function M.Entry:to_object(obj, scope)
	U.type_assert(obj, "userdata", true)
	U.type_assert(scope, "userdata", true)
	if not obj then
		obj = O.create()
	else
		O.clear(obj)
	end

	O.set_identifier(obj, "Entry")
	if self.ool then
		O.set_name(O.push_tag(obj), "ool")
	end

	local range_obj = O.push_child(obj)
	O.set_name(range_obj, "range")
	O.set_expression(range_obj)

	local r_start_obj = O.push_child(range_obj)
	local r_end_obj = O.push_child(range_obj)

	self.r_start:to_object(r_start_obj, scope)
	O.set_op(r_end_obj, O.Operator.sub)
	self.r_end:to_object(r_end_obj, scope)

	if #self.tags > 0 then
		local tags_obj = O.push_child(obj)
		O.set_name(tags_obj, "tags")
		for _, tag in ipairs(self.tags) do
			O.set_identifier(O.push_child(tags_obj), tag)
		end
	end

	if #self.rel_id > 0 then
		local rel_id_obj = O.push_child(obj)
		O.set_name(rel_id_obj, "rel_id")
		for _, rel_id in ipairs(self.rel_id) do
			O.set_identifier(O.push_child(rel_id_obj), rel_id)
		end
	end

	if self.continue_id ~= nil then
		local continue_id_obj = O.push_child(obj)
		O.set_name(continue_id_obj, "continue_id")
		if self.continue_scope and self.continue_scope ~= scope then
			local scope_obj = O.push_child(continue_id_obj)
			O.set_time_date(scope_obj, self.continue_scope)
			if scope then
				O.reduce_time(scope_obj, scope)
			end
			O.set_identifier(O.push_child(scope_obj), self.continue_id)
		else
			O.set_identifier(continue_id_obj, self.continue_id)
		end
	end

	if self.marker then
		local marker_obj = O.push_child(obj)
		O.set_name(marker_obj, "marker")
		O.set_identifier(marker_obj, self.marker)
	end

	if #self.actions > 0 then
		local actions_obj = O.push_child(obj)
		O.set_name(actions_obj, "tags")
		for i, action in ipairs(self.actions) do
			action:to_object(O.push_child(actions_obj), i ~= 1 and self.primary_action == i)
		end
	end
	return obj
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
		entry.source_line = O.source_line(obj)
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
M.Entry.t_body:add({
Match.Pattern{
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
},

-- tags = {...}
Match.Pattern{
	name = "tags",
	children = {Match.Pattern{
		vtype = {O.Type.identifier, O.Type.string},
		acceptor = function(context, self, obj)
			table.insert(self.tags, O.text(obj))
		end,
	}},
},

-- rel_id = ...
Match.Pattern{
	name = "rel_id",
	vtype = O.Type.identifier,
	acceptor = function(context, self, obj)
		table.insert(self.rel_id, O.identifier(obj))
	end,
},
-- rel_id = {...}
Match.Pattern{
	name = "rel_id",
	children = {Match.Pattern{
		vtype = O.Type.identifier,
		acceptor = function(context, self, obj)
			table.insert(self.rel_id, O.identifier(obj))
		end,
	}},
},

-- continue_id = identifier
Match.Pattern{
	name = "continue_id",
	vtype = O.Type.identifier,
	acceptor = function(context, self, obj)
		self.continue_id = O.identifier(obj)
		self.continue_scope = context.user.tracker.date
	end,
},
-- continue_id = null
Match.Pattern{
	name = "continue_id",
	vtype = O.Type.null,
},
-- continue_id = date{identifier}
Match.Pattern{
	name = "continue_id",
	vtype = O.Type.time,
	children = 1,
	acceptor = function(context, self, obj)
		if O.has_clock(obj) then
			return Match.Error("scope must not have clock time")
		end

		self.continue_id = O.identifier(O.child_at(obj, 1))
		self.continue_scope = T(O.time_resolved(obj, context.user.tracker.date))
	end,
},

-- marker = ...
Match.Pattern{
	name = "marker",
	vtype = O.Type.identifier,
	acceptor = function(context, self, obj)
		self.marker = O.identifier(obj)
		if context.user.tracker.entry_by_marker[self.marker] then
			return Match.Error("marker '%s' is not unique", self.marker)
		end
		context.user.tracker.entry_by_marker[self.marker] = {
			entry = self,
			index = #context.user.tracker.entries,
		}
	end,
},

-- actions = {...}
Match.Pattern{
	name = "actions",
	children = {M.Action.p_head},
	acceptor = function(context, self, obj)
		if not O.has_children(obj) then
			return Match.Error("entry actions must be non-empty when specified")
		end
	end,
},
})

M.Entry.t_body:build()
M.Entry.t_head_tags:build()

M.Attachment = U.class(M.Attachment)

function M.Attachment:__init()
	self.id = nil
	self.id_hash = O.NAME_NULL
	self.data = nil
end

function M.Attachment:to_object(obj)
	U.type_assert(obj, "userdata", true)
	if not obj then
		obj = O.create()
	else
		O.clear(obj)
	end

	O.set_identifier(obj, self.id)
	self.data:to_object(self, obj)
	return obj
end

-- attachment
M.Attachment.p_head = Match.Pattern{
	vtype = O.Type.identifier,
	tags = Match.Any,
	children = Match.Any,
	acceptor = function(context, tracker, obj)
		local attachment = M.Attachment()
		attachment.id = O.identifier(obj)
		attachment.id_hash = O.identifier_hash(obj)
		table.insert(tracker.attachments, attachment)

		return context.user.director:read_attachment(context, tracker, attachment, obj)
	end,
}

M.UnknownAttachment = U.class(M.UnknownAttachment)

function M.UnknownAttachment:__init()
	self.obj = O.create()
end

function M.UnknownAttachment:from_object(context, tracker, attachment, obj)
	O.copy_children(self.obj, obj)
	O.copy_tags(self.obj, obj)
end

function M.UnknownAttachment:to_object(attachment, obj)
	O.copy_children(obj, self.obj)
	O.copy_tags(obj, self.obj)
end

function M.UnknownAttachment:compare_equal(other)
	-- TODO?
	return true
end

M.t_body = Match.Tree()

M.t_head = Match.Tree({
-- Tracker{...}
Match.Pattern{
	vtype = O.Type.identifier,
	value = "Tracker",
	children = M.t_body,
},
})

M.t_body:add({
-- date = date{:full:zoned}
Match.Pattern{
	name = "date",
	vtype = O.Type.time,
	acceptor = function(context, self, obj)
		if O.is_date_contextual(obj) then
			return Match.Error("date must be full")
		elseif not O.is_zoned(obj) then
			return Match.Error("date must be zoned")
		elseif O.has_clock(obj) then
			return Match.Error("date must not have clock time")
		end

		T.set(self.date, O.time(obj))
	end,
},
-- entries = list{Entry}
Match.Pattern{
	name = "entries",
	children = {
		M.Entry.p_head,
		M.Attachment.p_head,
	},
	acceptor = function(context, self, obj)
		if T.value(self.date) == 0 then
			-- TODO: pre-match to avoid this
			return Match.Error("date must be set before entries")
		end
	end,
},
})

M.t_body:build()
M.t_head:build()

return M

)"__RAW_STRING__"