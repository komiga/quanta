
local U = require "togo.utility"
local O = require "Quanta.Object"
local Time = require "Quanta.Time"
local Match = require "Quanta.Match"

-- Match.debug = true

local Entity = require "Quanta.Entity"
local Measurement = require "Quanta.Measurement"
local Unit = require "Quanta.Unit"
local Tracker = require "Quanta.Tracker"

function make_time(text)
	U.type_assert(text, "string")

	local t = Time()
	local obj = O.create(text)
	U.assert(obj)
	Time.set(t, O.time(obj))
	return t
end

function make_modifier(id, data)
	U.type_assert(id, "string")
	U.assert(U.is_type(data, "table") or U.is_instance(data))

	local m = Unit.Modifier()
	m.id = id
	m.id_hash = O.hash_name(id)
	m.data = data
	return m
end

function make_step(seq, implicit, measurements, modifiers, items)
	U.type_assert(seq, "number")
	U.type_assert(implicit, "boolean", true)

	local u = Unit.Step(seq, implicit)
	u.measurements = measurements
	u.modifiers = modifiers
	for _, i in ipairs(items) do
		u:add(i)
	end
	return u
end

function make_element(type, index, description, author, note, items)
	U.type_assert(type, "number")
	U.type_assert(index, "number")
	U.type_assert(description, "string")
	U.type_assert(author, "table")
	U.type_assert(note, "table")
	U.type_assert(items, "table")

	local u = Unit.Element(type, index)
	u.description = description
	u.author = author
	u.note = note
	for _, i in ipairs(items) do
		u:add(i)
	end
	return u
end

function make_element_generic(index, description, author, note, steps)
	return make_element(Unit.ElementType.generic, index, description, author, note, steps)
end

function make_element_primary(index, description, author, note, steps)
	return make_element(Unit.ElementType.primary, index, description, author, note, steps)
end

function make_unit_ref(
	name, id, thing, scope,
	source, sub_source,
	source_certain, sub_source_certain,
	variant_certain, presence_certain,
	measurements, modifiers, items
)
	U.type_assert(name, "string", true)
	U.type_assert(id, "string", true)
	U.type_assert(scope, "string", true)
	U.type_assert(source, "number")
	U.type_assert(sub_source, "number")
	U.type_assert(source_certain, "boolean")
	U.type_assert(sub_source_certain, "boolean")
	U.type_assert(variant_certain, "boolean")
	U.type_assert(presence_certain, "boolean")
	U.type_assert(measurements, "table")
	U.type_assert(modifiers, "table")
	U.type_assert(items, "table")

	local u = Unit.Reference()
	u:set_name(name)
	u:set_id(id)
	u.scope = scope and make_time(scope) or nil
	u.thing = thing
	u.source = source
	u.sub_source = sub_source
	u.source_certain = source_certain
	u.sub_source_certain = sub_source_certain
	u.variant_certain = variant_certain
	u.presence_certain = presence_certain
	u.measurements = measurements
	u.modifiers = modifiers
	for _, i in ipairs(items) do
		u:add(i)
	end
	return u
end

function make_unit_comp(name, measurements, modifiers, items)
	U.type_assert(name, "string", true)
	U.type_assert(measurements, "table")
	U.type_assert(modifiers, "table")
	U.type_assert(items, "table")

	local u = Unit.Composition()
	u:set_name(name)
	u.modifiers = modifiers
	u.measurements = measurements
	for _, i in ipairs(items) do
		u:add(i)
	end
	return u
end

function make_unit_def(sub_type, name, description, author, note, measurements, modifiers, items, _splerwuh)
	U.type_assert(sub_type, "number")
	U.type_assert(name, "string")
	U.type_assert(description, "string")
	U.type_assert(author, "table")
	U.type_assert(note, "table")
	U.type_assert(measurements, "table")
	U.type_assert(modifiers, "table")
	U.type_assert(items, "table")
	U.assert(_splerwuh == nil)

	local u = Unit.Definition()
	u.sub_type = sub_type
	u:set_name(name)
	u.description = description
	u.author = author
	u.note = note
	u.modifiers = modifiers
	u.measurements = measurements
	for _, i in ipairs(items) do
		u:add(i)
	end
	return u
end

function make_tracker_action(id, data)
	U.type_assert(id, "string")
	U.assert(U.is_type(data, "table") or U.is_instance(data))

	local a = Tracker.Action()
	a.id = id
	a.id_hash = O.hash_name(id)
	a.data = data
	return a
end

local entry_time_types = {
	["XXX"] = Tracker.EntryTime.Type.placeholder,
	["EPREV"] = Tracker.EntryTime.Type.ref_prev,
	["ENEXT"] = Tracker.EntryTime.Type.ref_next,
}

function make_tracker_entry_time(time, approximation, certain)
	local t = Tracker.EntryTime()
	t.type = entry_time_types[time] or Tracker.EntryTime.Type.specified
	if t.type == Tracker.EntryTime.Type.specified then
		U.type_assert(time, "string", true)
		t.time = make_time(time)
	else
		U.assert(time == nil)
		t.time = T()
	end
	t.approximation = approximation
	t.certain = certain
	return t
end

function make_tracker_entry(ool, r_start, r_end, tags, rel_id, continue_scope, continue_id, primary_action, actions)
	U.type_assert(ool, "boolean")
	U.type_assert(r_start, Tracker.EntryTime)
	U.type_assert(r_end, Tracker.EntryTime)
	U.type_assert(tags, "table")
	U.type_assert(rel_id, "table")
	U.type_assert(continue_scope, "string", true)
	U.type_assert(continue_id, "string", true)
	U.type_assert(marker, "string", true)
	U.type_assert(primary_action, "number", true)
	U.type_assert(actions, "table")

	local e = Tracker.Entry()
	e.ool = ool
	e.r_start = r_start
	e.r_end = r_end
	e.tags = tags
	e.rel_id = rel_id
	e.continue_scope = continue_scope and make_time(continue_scope) or nil
	e.continue_id = continue_id
	e.actions = actions
	e.primary_action = primary_action

	e:recalculate()
	return e
end

function make_tracker(date, entries)
	U.type_assert(date, "string")
	U.type_assert(entries, "table")

	local t = Tracker()
	t.date = make_time(date)
	t.entries = entries
	return t
end

function check_measurement_list_equal(x, y)
	U.assert(#x == #y)
	for i = 1, #x do
		U.assert(x[i] == y[i])
	end
end

function check_author_equal(x, y)
	U.assert(x.name == y.name)
	U.assert(x.certain == y.certain)
	U.assert(x.address == y.address)
	U.assert(x.address_certain == y.address_certain)
end

function check_note_equal(x, y)
	U.assert(x.text == y.text)
	U.assert((x.time == nil) == (y.time == nil))
	if x.time then
		U.assert(Time.compare_equal(x.time, y.time))
	end
end

function check_modifier_equal(x, y)
	U.assert(x.id == y.id)
	U.assert(x.id_hash == y.id_hash)

	local class = U.type_class(x.data)
	U.assert(class == U.type_class(y.data))
	if class.compare_equal then
		U.assert(x.data:compare_equal(y.data))
	end
end

function check_modifier_list_equal(x, y)
	U.assert(#x.modifiers == #y.modifiers)
	for i = 1, #x.modifiers do
		check_modifier_equal(x.modifiers[i], y.modifiers[i])
	end
end

function check_unit_equal(x, y)
	U.assert(x.type == y.type)
	U.assert(x.sub_type == y.sub_type)
	U.assert(x.name == y.name)
	U.assert(x.name_hash == y.name_hash)
	U.assert(x.id == y.id)
	U.assert(x.id_hash == y.id_hash)

	U.assert((x.scope == nil) == (y.scope == nil))
	if x.scope then
		U.assert(Time.compare_equal(x.scope, y.scope))
	end
	U.assert(x.thing == y.thing)

	U.assert(x.description == y.description)
	U.assert(#x.author == #y.author)
	for i = 1, #x.author do
		check_author_equal(x.author[i], y.author[i])
	end
	U.assert(#x.note == #y.note)
	for i = 1, #x.note do
		check_note_equal(x.note[i], y.note[i])
	end

	U.assert(x.source == y.source)
	U.assert(x.sub_source == y.sub_source)
	U.assert(x.source_certain == y.source_certain)
	U.assert(x.sub_source_certain == y.sub_source_certain)
	U.assert(x.variant_certain == y.variant_certain)
	U.assert(x.presence_certain == y.presence_certain)

	check_modifier_list_equal(x, y)
	check_measurement_list_equal(x, y)

	U.assert(#x.items == #y.items)
	for i = 1, #x.items do
		check_unit_equal(x.items[i], y.items[i])
	end

	check_modifier_list_equal(x, y)
	check_measurement_list_equal(x, y)
end

function check_tracker_action_equal(x, y)
	U.assert(x.id == y.id)
	U.assert(x.id_hash == y.id_hash)

	local class = U.type_class(x.data)
	U.assert(class == U.type_class(y.data))
	if class.compare_equal then
		U.assert(x.data:compare_equal(y.data))
	end
end

function check_tracker_entry_time_equal(x, y)
	U.assert(x.type == y.type)
	U.assert(Time.compare_equal(x.time, y.time))
	U.assert(x.ool == y.ool)
	U.assert(x.index == y.index)
	U.assert(x.approximation == y.approximation)
	U.assert(x.certain == y.certain)
end

function check_tracker_entry_equal(x, y)
	U.assert(x.ool == y.ool)
	U.assert(Time.compare_equal(x.duration, y.duration))
	check_tracker_entry_time_equal(x.r_start, y.r_start)
	check_tracker_entry_time_equal(x.r_end, y.r_end)

	U.assert(#x.tags == #y.tags)
	for i = 1, #x.tags do
		check_tracker_entry_equal(x.tags[i], y.tags[i])
	end

	U.assert(#x.rel_id == #y.rel_id)
	for i = 1, #x.rel_id do
		U.assert(x.rel_id[i] == y.rel_id[i])
	end

	U.assert(#x.tags == #y.tags)
	for i = 1, #x.tags do
		U.assert(x.tags[i] == y.tags[i])
	end

	U.assert(not x.continue_scope == not y.continue_scope)
	if x.continue_scope then
		U.assert(Time.compare_equal(x.continue_scope, y.continue_scope))
	end
	U.assert(x.continue_id == y.continue_id)

	U.assert(x.primary_action == y.primary_action)
	U.assert(#x.actions == #y.actions)
	for i = 1, #x.actions do
		check_tracker_action_equal(x.actions[i], y.actions[i])
	end
end

function check_tracker_attachment_equal(x, y)
	U.assert(x.id == y.id)
	U.assert(x.id_hash == y.id_hash)

	local class = U.type_class(x.data)
	U.assert(class == U.type_class(y.data))
	if class.compare_equal then
		U.assert(x.data:compare_equal(y.data))
	end
end

function check_tracker_equal(x, y)
	U.assert(Time.compare_equal(x.date, y.date))

	U.assert(#x.entries == #y.entries)
	for i = 1, #x.entries do
		check_tracker_entry_equal(x.entries[i], y.entries[i])
	end

	U.assert(#x.attachments == #y.attachments)
	for i = 1, #x.attachments do
		check_tracker_attachment_equal(x.attachments[i], y.attachments[i])
	end
end
