local log = require("example.log")

local M = {}
local event_handler

local function restore_audio_after_fullscreen_ad(message_id, event)
    local fullscreen = message_id == levelplay.MSG_INTERSTITIAL or message_id == levelplay.MSG_REWARDED
    local finished = event == levelplay.EVENT_AD_CLOSED or event == levelplay.EVENT_AD_DISPLAY_FAILED
    if fullscreen and finished then
        sound.set_group_gain("master", 1)
    end
end

local function levelplay_callback(self, message_id, message)
    print(("callback: %s %s"):format(
        log.message_name(message_id) or tostring(message_id),
        log.event_name(message.event) or tostring(message.event)
    ))

    local details = log.table_to_string(message, { event = true })
    if details then
        print("message: " .. details)
    end

    if message_id == levelplay.MSG_REWARDED and message.event == levelplay.EVENT_AD_REWARDED then
        print(("reward: %s x %s"):format(
            tostring(message.reward_name),
            tostring(message.reward_amount)
        ))
    end

    restore_audio_after_fullscreen_ad(message_id, message.event)

    if event_handler then
        event_handler(self, message_id, message)
    end
end

function M.set(handler)
    event_handler = handler
    levelplay.set_callback(levelplay_callback)
end

function M.clear()
    event_handler = nil
    levelplay.set_callback(nil)
end

return M
