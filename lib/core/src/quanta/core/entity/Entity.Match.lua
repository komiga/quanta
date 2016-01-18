u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local Entity = require "Quanta.Entity"
local Vessel = require "Quanta.Vessel"
local M = U.module(...)

local function compose_string(context, name, obj)
	local value = not O.value_certain(obj) and "?" or ""
	if O.is_string(obj) then
		value = value .. O.string(obj)
	elseif O.has_children(obj) then
		local parent = context:value(1).parent:source(0)[name]
		parent = parent or ""
		for _, sub in O.children(obj) do
			if O.is_identifier(sub) and O.identifier(sub) == "_" then
				value = value .. parent
			elseif O.is_string(sub) then
				value = value .. O.string(sub)
			end
		end
	end
	return value
end

local function first_string(obj)
	local prefix = not O.value_certain(obj) and "?" or ""
	if O.is_string(obj) then
		return prefix .. O.string(obj)
	elseif O.has_children(obj) then
		for _, sub in O.children(obj) do
			if O.is_string(sub) then
				return prefix .. O.string(sub)
			end
		end
	end
	return prefix
end

M.source = Match.Tree()

M.source:add({
-- TODO
Match.Pattern{
	name = "note",
	vtype = O.Type.string,
},
Match.Pattern{
	name = "note",
	children = {
		Match.Pattern{vtype = O.Type.string}
	},
},
Match.Pattern{
	name = "d",
	vtype = {O.Type.null, O.Type.string, O.Type.expression},
	children = function(_, _, obj, _)
		return O.has_children(obj) == O.is_expression(obj)
	end,
	acceptor = function(context, p, obj)
		p:set_description(compose_string(context, "description", obj))
	end
},
Match.Pattern{
	name = "label",
	vtype = O.Type.string,
	acceptor = function(_, p, obj)
		p:set_label(O.string(obj))
	end
},
Match.Pattern{
	name = "composition",
	vtype = {O.Type.identifier, O.Type.string},
	tags = Match.Any,
	acceptor = function(_, p, obj)
		-- TODO: decompose (Quanta.Entity.Instance?)
	end
},
Match.Pattern{
	name = "composition",
	vtype = {O.Type.null, O.Type.expression},
	children = true,
	acceptor = function(_, p, obj)
		-- TODO: decompose (Quanta.Entity.Instance?)
	end
},
})

local source_model_tags = {
Match.Pattern{
	name = "id",
	children = {Match.Pattern{
		vtype = {O.Type.null, O.Type.string},
		acceptor = function(_, model, obj)
			if O.is_string(obj) then
				model:set_id(O.string(obj))
			end
		end
	}},
},
}

M.source:add({
Match.Pattern{
	name = "base_model",
	vtype = {O.Type.null, O.Type.string},
	tags = source_model_tags,
	acceptor = function(_, p, obj)
		local base_model = Entity.Model()
		if O.is_string(obj) then
			base_model:set_name(O.string(obj))
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
			model:set_name(O.string(obj))
		end
		p:set_model(model)
		return model
	end
},
})

local source_place_tags = {
Match.Pattern{
	name = {"address", "addr"},
	children = {Match.Pattern{
		vtype = {O.Type.null, O.Type.string},
		acceptor = function(_, place, obj)
			if O.is_string(obj) then
				place:set_address(O.string(obj))
			end
		end
	}},
},
}

M.source:add({
Match.Pattern{
	name = "author",
	vtype = {O.Type.null, O.Type.string, O.Type.expression},
	tags = source_place_tags,
	acceptor = function(_, p, obj)
		local author = Entity.Place()
		author:set_name(first_string(obj))
		p:set_author(author)
		return author
	end
},
Match.Pattern{
	name = "vendor",
	vtype = {O.Type.null, O.Type.string, O.Type.expression},
	children = function(_, _, obj, _)
		return O.has_children(obj) == O.is_expression(obj)
	end,
	tags = source_place_tags,
	acceptor = function(_, p, obj)
		local vendor = Entity.Place()
		vendor:set_name(first_string(obj))
		p:set_vendor(0, vendor)
		return vendor
	end
},
Match.Pattern{
	name = "vendors",
	children = {Match.Pattern{
		vtype = {O.Type.null, O.Type.string},
		tags = source_place_tags,
		acceptor = function(_, p, obj)
			local vendor = Entity.Place()
			if O.is_string(obj) then
				vendor:set_name(O.string(obj))
			end
			p:add_vendor(vendor)
			return vendor
		end
	}},
},
})

M.source:add({
Match.Pattern{
	name = "composition",
	vtype = {O.Type.identifier, O.Type.string, O.Type.expression},
	tags = true,
	acceptor = function(_, p, obj)
		-- TODO: decompose (Quanta.Entity.Instance?)
	end
},
Match.Pattern{
	name = "composition",
	children = true,
	acceptor = function(_, p, obj)
		-- TODO: decompose (Quanta.Entity.Instance?)
	end
},
Match.Pattern{
	name = "nutrition",
	children = true,
	acceptor = function(_, p, obj)
		-- TODO: decompose (Quanta.Entity.Instance?)
	end
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
-- TODO: computer compositor
Match.Pattern{
	name = "config",
	children = true,
},
-- TODO
Match.Pattern{
	name = "properties",
	children = true,
},
--[[Match.Pattern{
	any = true,
},--]]
})

M.entity = Match.Tree()

local entity_patterns = {
Match.Pattern{
	name = "sources",
	children = {Match.Pattern{
		children = M.source,
		acceptor = function(_, e, obj)
			return e:add_source(Entity.Source())
		end
	}},
	acceptor = function(_, e, obj)
		if e:has_source() then
			return Match.Error("entity source(s) already defined")
		end
	end
},
Match.Pattern{
	name = "specializations",
	children = M.entity,
},
Match.Pattern{
	any_branch = M.source,
	acceptor = function(_, e, obj)
		return e:source()
	end
},
Match.Pattern{
	func = function(_, e, _, _)
		return e.compositor ~= nil
	end,
	acceptor = function(context, e, obj)
		if e.compositor then
			context:consume_sub(e.compositor, obj)
		end
	end
},
--[[Match.Pattern{
	any = true,
},--]]
}

M.entity:add(Match.Pattern{
	name = true,
	vtype = O.Type.identifier,
	value = "Entity",
	children = entity_patterns,
	acceptor = function(_, parent, obj)
		return parent:add(Entity(O.name(obj)))
	end
})

M.universe = Match.Tree()

local category_patterns = {
Match.Pattern{
	name = "d",
	vtype = {O.Type.null, O.Type.string, O.Type.expression},
	children = function(_, _, obj, _)
		return O.has_children(obj) == O.is_expression(obj)
	end,
	acceptor = function(context, cat, obj)
		Entity.Category.set_description(cat, compose_string(context, "description", obj))
	end
},
Match.Pattern{
	name = "compositor",
	vtype = O.Type.string,
	acceptor = function(context, cat, obj)
		local path = Vessel.data_path("entity/" .. O.string(obj))
		-- TODO: error reporting
		local chunk, err = loadfile(path)
		if err then
			return Match.Error("failed to load compositor from '%s': %s", path, err)
		end
		cat:set_compositor(chunk())
	end
},
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
			if not O.read_text_file(sub, Vessel.data_path("entity/" .. path)) then
				return Match.Error("failed to load include file: %s", path)
			end
			if not context:consume_sub(context:control(), sub) then
				return false
			end
		end
		O.destroy(sub)
		return true
	end
},
Match.Pattern{
	name = "aliases",
	children = {Match.Pattern{
		children = true,
		acceptor = function(_, cat, obj)
			if O.num_children(obj) < 2 then
				return Match.Error("alias definition must have at least one target")
			end
			-- TODO
		end,
	}},
},
}

M.universe:add(Match.Pattern{
	name = true,
	vtype = O.Type.identifier,
	value = "EntityCategory",
	children = category_patterns,
	acceptor = function(_, parent, obj)
		return parent:add(Entity.Category(O.name(obj)))
	end
})

table.insert(category_patterns, Match.Pattern{
	name = "children",
	children = U.table_ijoined(M.universe.patterns, M.entity.patterns),
})

function M.read_universe(root_path, name)
	U.type_assert(name, "string", true)

	local universe = nil
	local root = O.create()
	if O.read_text_file(root, root_path) then
		universe = Entity.Universe(name or "universe")
		local context = Match.Context()
		if not context:consume_sub(M.universe, root, universe) then
			U.log("match error:\n%s", context.error:to_string(context.error_obj))
			universe = nil
		end
	else
		U.log("error: failed to read root")
	end
	O.destroy(root)
	return universe
end

return M

)"__RAW_STRING__"