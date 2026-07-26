local M = {}

local native_print = print
local entries = {}
local message_names = {}
local event_names = {}

_G.print = function(...)
    native_print(...)
    local values = { ... }
    local line = ""
    for index = 1, #values do
        if index > 1 then
            line = line .. " "
        end
        line = line .. tostring(values[index])
    end
    entries[#entries + 1] = line
end

local function collect_constant(name, value)
    if name:match("^MSG_") then
        message_names[value] = name
    elseif name:match("^EVENT_") or name:match("^TRACKING_STATUS_") then
        event_names[value] = name
    end
end

function M.print_all_sdk_entities()
    print("---- LevelPlay Lua API ----")
    local names = {}
    for name in pairs(_G.levelplay) do
        names[#names + 1] = name
    end
    table.sort(names)

    for _, name in ipairs(names) do
        local value = _G.levelplay[name]
        if type(value) == "function" then
            print(name .. "()")
        else
            collect_constant(name, value)
            print(name .. "=" .. tostring(value))
        end
    end
    print("---------------------------")
end

function M.message_name(value)
    return message_names[value]
end

function M.event_name(value)
    return event_names[value]
end

function M.table_to_string(value, ignored)
    if not value then
        return nil
    end

    local keys = {}
    for key in pairs(value) do
        if not ignored or not ignored[key] then
            keys[#keys + 1] = key
        end
    end
    table.sort(keys, function(a, b)
        return tostring(a) < tostring(b)
    end)

    local parts = {}
    for _, key in ipairs(keys) do
        parts[#parts + 1] = tostring(key) .. "=" .. tostring(value[key])
    end
    return #parts > 0 and table.concat(parts, ", ") or nil
end

function M.update(node)
    local font_name = gui.get_font(node)
    local font = gui.get_font_resource(font_name)
    local size = gui.get_size(node)
    local options = { width = size.x, line_break = true }
    local text = table.concat(entries, "\n")
    local metrics = resource.get_text_metrics(font, text, options)

    while #entries > 1 and metrics.height > size.y do
        table.remove(entries, 1)
        text = table.concat(entries, "\n")
        metrics = resource.get_text_metrics(font, text, options)
    end
    gui.set_text(node, text)
end

return M
