
local U = require "togo.utility"
local O = require "Quanta.Object"
local Time = require "Quanta.Time"
local Entity = require "Quanta.Entity"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local Composition = require "Quanta.Composition"
local Unit = require "Quanta.Unit"
local Tracker = require "Quanta.Tracker"

--Match.debug = true

function make_time(text)
	local t = Time()
	local obj = O.create(text)
	U.assert(obj)
	Time.set(t, O.time(obj))
	return t
end

function make_modifier(id, data)
	local m = Instance.Modifier()
	m.id = id
	m.id_hash = O.hash_name(id)
	m.data = data
	return m
end

function make_instance(
	name, item, scope,
	source, sub_source,
	source_certain, sub_source_certain,
	variant_certain, presence_certain,
	measurements, selection, modifiers
)
	local i = Instance()
	i.name = name
	i.name_hash = O.hash_name(name)
	i.scope = scope and make_time(scope) or nil
	i.item = item
	i.source = source
	i.sub_source = sub_source
	i.source_certain = source_certain
	i.sub_source_certain = sub_source_certain
	i.variant_certain = variant_certain
	i.presence_certain = presence_certain
	i.measurements = measurements
	i.selection = selection
	i.modifiers = modifiers
	return i
end

function make_composition(measurement, items)
	local c = Composition()
	c.items = items
	c.measurement = measurement
	return c
end

function make_step(index, measurement, items)
	local s = Unit.Step()
	s.index = index
	s.composition = make_composition(measurement, items)
	return s
end

function make_element(type, index, description, author, note, steps)
	local e = Unit.Element()
	e.type = type
	e.index = index
	e.description = description
	e.author = author
	e.note = note
	e.steps = steps
	return e
end

function make_element_generic(index, description, author, note, steps)
	return make_element(Unit.Element.Type.generic, index, description, author, note, steps)
end

function make_element_primary(index, description, author, note, steps)
	return make_element(Unit.Element.Type.primary, index, description, author, note, steps)
end

function make_unit(type, name, description, author, note, elements_generic, elements_primary)
	local u = Unit()
	u.type = type
	u:set_name(name)
	u.description = description
	u.author = author
	u.note = note
	u.elements = {
		elements_generic,
		elements_primary,
	}
	return u
end

function make_tracker_action(id, data)
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
		t.time = make_time(time)
	else
		t.time = T()
	end
	t.approximation = approximation
	t.certain = certain
	return t
end

function make_tracker_entry(ool, r_start, r_end, tags, rel_id, continue_scope, continue_id, primary_action, actions)
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
	local t = Tracker()
	t.date = make_time(date)
	t.entries = entries
	return t
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

function check_instance_equal(x, y)
	U.assert(x.name == y.name)
	U.assert(x.name_hash == y.name_hash)
	U.assert((x.scope == nil) == (y.scope == nil))
	if x.scope then
		U.assert(Time.compare_equal(x.scope, y.scope))
	end
	U.assert(x.item == y.item)
	U.assert(x.source == y.source)
	U.assert(x.sub_source == y.sub_source)
	U.assert(x.source_certain == y.source_certain)
	U.assert(x.sub_source_certain == y.sub_source_certain)
	U.assert(x.variant_certain == y.variant_certain)
	U.assert(x.presence_certain == y.presence_certain)

	U.assert(#x.measurements == #y.measurements)
	for mi = 1, #x.measurements do
		U.assert(x.measurements[mi] == y.measurements[mi])
	end

	U.assert(#x.selection == #y.selection)
	for si = 1, #x.selection do
		check_instance_equal(x.selection[si], y.selection[si])
	end

	U.assert(#x.modifiers == #y.modifiers)
	for mi = 1, #x.modifiers do
		check_modifier_equal(x.modifiers[mi], y.modifiers[mi])
	end
end

function check_composition_equal(x, y)
	U.assert(#x.items == #y.items)
	for i = 1, #x.items do
		check_instance_equal(x.items[i], y.items[i])
	end
	U.assert(x.measurement == y.measurement)
end

function check_step_equal(x, y)
	U.assert(x.index == y.index)
	check_composition_equal(x.composition, y.composition)
end

function check_element_equal(x, y)
	U.assert(x.type == y.type)
	U.assert(x.index == y.index)
	U.assert(x.description == y.description)

	U.assert(#x.author == #y.author)
	for i = 1, #x.author do
		check_author_equal(x.author[i], y.author[i])
	end

	U.assert(#x.steps == #y.steps)
	for i = 1, #x.steps do
		check_step_equal(x.steps[i], y.steps[i])
	end
end

function check_unit_equal(x, y)
	U.assert(x.type == y.type)
	U.assert(x.name == y.name)
	U.assert(x.name_hash == y.name_hash)
	U.assert(x.description == y.description)

	U.assert(#x.author == #y.author)
	for i = 1, #x.author do
		check_author_equal(x.author[i], y.author[i])
	end

	U.assert(#x.note == #y.note)
	for i = 1, #x.note do
		check_note_equal(x.note[i], y.note[i])
	end

	U.assert(#x.elements == #y.elements)

	local a = x.elements[Unit.Element.Type.generic]
	local b = y.elements[Unit.Element.Type.generic]
	U.assert(#a == #b)
	for i = 1, #a do
		check_element_equal(a[i], b[i])
	end

	a = x.elements[Unit.Element.Type.primary]
	b = y.elements[Unit.Element.Type.primary]
	U.assert(#a == #b)
	for i = 1, #a do
		check_element_equal(a[i], b[i])
	end
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
