local M = {}

local ZERO_V = vmath.vector3(0)

local function init(tree, actions, name, el_data, parent)
    if name == "type" then
        return
    end

    local el = {
        name = name,
        parent = parent,
        enabled = true,
    }
    tree[name] = el

    if M.is_button(el, actions) then
        el.type = "button"
        el.node = gui.get_node(name .. "/larrybutton")
    else
        el.type = el_data.type
        el.node = gui.get_node(name)
    end

    if el.type == "page" then
        M.enable(el, false)
        gui.set_position(el.node, ZERO_V)
    end
end

function M.is_button(el, actions)
    return (actions and actions[el.name] ~= nil) or el.type == "button"
end

function M.enable(el, enabled)
    el.enabled = enabled
    gui.set_enabled(el.node, enabled)
end

function M.disable_all(tree, element_type)
    for _, el in pairs(tree) do
        if el.type == element_type then
            M.enable(el, false)
        end
    end
end

function M.is_enabled(el)
    if el.parent then
        return el.enabled and M.is_enabled(el.parent)
    end
    return el.enabled
end

function M.fill_tree(data, actions)
    local tree = {}
    for parent_name, parent_data in pairs(data) do
        init(tree, actions, parent_name, parent_data)
        for child_name, child_data in pairs(parent_data) do
            init(tree, actions, child_name, child_data, tree[parent_name])
        end
    end
    return tree
end

return M
