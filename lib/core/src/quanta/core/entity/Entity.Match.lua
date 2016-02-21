u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Prop = require "Quanta.Prop"
local Entity = require "Quanta.Entity"
local Vessel = require "Quanta.Vessel"
local M = U.module(...)

local function parent_source_value(name)
	return function(context)
		local parent_entity = context:value(1).parent
		return parent_entity and parent_entity.sources[0][name]
	end
end

M.t_universe = Match.Tree()
M.t_source = Match.Tree()

local _, t_source_description = Prop.Description.adapt_struct(
	"d", "description",
	parent_source_value("description")
)
local _, t_source_label = Prop.Description.adapt_struct(
	"label", "label",
	parent_source_value("label")
)

local source_model_tags = {
Match.Pattern{
	name = "id",
	children = {Match.Pattern{
		vtype = {O.Type.null, O.Type.string},
		acceptor = function(_, model, obj)
			if O.is_string(obj) then
				model.id = O.string(obj)
			end
		end
	}},
},
}

M.t_source:add({
Prop.Note.t_struct_head,
Prop.Author.t_struct_head,
t_source_description,
t_source_label,
Match.Pattern{
	name = "vendor",
	vtype = {O.Type.null, O.Type.string},
	tags = Prop.Author.t_head_tags,
	acceptor = function(_, p, obj)
		return p:set_vendor(0, Prop.Author(
			O.is_string(obj) and O.string(obj) or nil,
			O.value_certain(obj),
			nil, true
		))
	end
},
Match.Pattern{
	name = "vendors",
	children = {Match.Pattern{
		vtype = {O.Type.null, O.Type.string},
		tags = Prop.Author.t_head_tags,
		acceptor = function(_, p, obj)
			return p:add_vendor(Prop.Author(
				O.is_string(obj) and O.string(obj) or nil,
				O.value_certain(obj),
				nil, true
			))
		end
	}},
},
Match.Pattern{
	name = "base_model",
	vtype = {O.Type.null, O.Type.string},
	tags = source_model_tags,
	acceptor = function(_, p, obj)
		local base_model = Entity.Model()
		if O.is_string(obj) then
			base_model.name = O.string(obj)
		end
		p:set_base_model(base_model)
		return base_model
	end
},
Match.Pattern{
	name = "model",
	vtype = {O.Type.null, O.Type.string},
	tags = source_model_tags,
	acceptor = function(_, p, obj)
		local model = Entity.Model()
		if O.is_string(obj) then
			model.name = O.string(obj)
		end
		p:set_model(model)
		return model
	end
},

-- TODO
Match.Pattern{
	name = "composition",
	vtype = {O.Type.identifier, O.Type.string, O.Type.expression},
	tags = Match.Any,
},
Match.Pattern{
	name = "composition",
	vtype = {O.Type.null, O.Type.expression},
	children = true,
},
Match.Pattern{
	name = "nutrition",
	children = true,
},
-- state
Match.Pattern{
	name = "state",
	vtype = O.Type.string,
},
Match.Pattern{
	name = "state",
	children = {
		Match.Pattern{vtype = O.Type.string}
	},
},
-- TODO: device specialization
Match.Pattern{
	name = "config",
	children = true,
},
-- TODO
Match.Pattern{
	name = "properties",
	children = true,
},
})

M.t_shared_body = Match.Tree({
Match.Pattern{
	name = "aliases",
	children = {Match.Pattern{
		children = true,
		acceptor = function(_, cat, obj)
			if O.num_children(obj) < 2 then
				return Match.Error("alias definition must have at least one source")
			end
			-- TODO
		end,
	}},
},
Match.Pattern{
	name = "children",
	children = M.t_universe,
},
Match.Pattern{
	any_branch = M.t_source,
	acceptor = function(_, e, obj)
		return e.sources[0]
	end
},
})

M.t_entity_head = Match.Tree()

M.t_entity_body = Match.Tree({
Match.Pattern{
	name = "sources",
	children = {Match.Pattern{
		children = M.t_source,
		acceptor = function(_, e, obj)
			return e:add_source(Entity.Source())
		end
	}},
	acceptor = function(_, e, obj)
		if e:any_sources() then
			return Match.Error("entity source(s) already defined")
		end
	end
},
M.t_shared_body,
})

M.t_entity_head:add(Match.Pattern{
	name = true,
	vtype = O.Type.identifier,
	value = "Entity",
	children = M.t_entity_body,
	acceptor = function(_, parent, obj)
		return parent:add(Entity(O.name(obj)))
	end
})

M.t_category_head = Match.Tree()

M.t_category_body = Match.Tree({
Match.Pattern{
	name = "include",
	collect = {Match.Pattern{
		vtype = O.Type.string,
		acceptor = function(_, _, obj)
			return O.string(obj)
		end
	}},
	collect_post = function(context, cat, obj, collection)
		local sub = O.create()
		for _, path in ipairs(collection) do
			-- TODO: expand pattern-match path against filesystem
			path = Vessel.data_path("entity/" .. path)
			if not O.read_text_file(sub, path) then
				return Match.Error("failed to load include file: %s", path)
			end
			if not context:consume_sub(context:control(), sub, cat, path) then
				return false
			end
		end
		return true
	end
},
M.t_shared_body,
})

M.t_category_head:add(Match.Pattern{
	name = true,
	vtype = O.Type.identifier,
	value = "EntityCategory",
	children = M.t_category_body,
	acceptor = function(_, parent, obj)
		return parent:add(Entity.Category(O.name(obj)))
	end
})

M.t_universe:add({
	M.t_category_head,
	M.t_entity_head,
})

function M.read_universe(rp, name)
	U.type_assert(name, "string", true)

	local path, root
	if U.is_type(rp, "string") then
		path = rp
		root = O.create()
		if not O.read_text_file(root, path) then
			U.log("error: failed to read root")
			return nil
		end
	else
		U.type_assert(rp, "userdata")
		root = rp
	end

	local universe = Entity.Universe(name or "universe")
	local context = Match.Context()
	if context:consume_sub(M.t_universe, root, universe, path) then
		return universe
	else
		U.log("match error:\n%s", context.error:to_string())
	end
	return nil
end

return M

)"__RAW_STRING__"