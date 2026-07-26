#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

#pragma once

#include <dmsdk/sdk.h>

namespace dmLevelPlay {

// These values are mirrored by the native Android bridge.
enum MessageId
{
    MSG_INIT = 1,
    MSG_INTERSTITIAL = 2,
    MSG_REWARDED = 3,
    MSG_BANNER = 4,
    MSG_TRACKING = 5
};

enum MessageEvent
{
    EVENT_INIT_SUCCEEDED = 1,
    EVENT_INIT_FAILED = 2,
    EVENT_AD_LOADED = 3,
    EVENT_AD_LOAD_FAILED = 4,
    EVENT_AD_INFO_CHANGED = 5,
    EVENT_AD_DISPLAYED = 6,
    EVENT_AD_DISPLAY_FAILED = 7,
    EVENT_AD_CLICKED = 8,
    EVENT_AD_CLOSED = 9,
    EVENT_AD_REWARDED = 10,
    EVENT_AD_EXPANDED = 11,
    EVENT_AD_COLLAPSED = 12,
    EVENT_AD_LEFT_APPLICATION = 13,

    TRACKING_STATUS_AUTHORIZED = 20,
    TRACKING_STATUS_DENIED = 21,
    TRACKING_STATUS_NOT_DETERMINED = 22,
    TRACKING_STATUS_RESTRICTED = 23,

    EVENT_JSON_ERROR = 100
};

struct CallbackData
{
    MessageId msg;
    MessageEvent event;
    int handle;
    char* json;
};

void SetLuaCallback(lua_State* L, int pos);
void UpdateCallback();
void InitializeCallback();
void FinalizeCallback();

void AddToQueueCallback(int message, int event, int handle, const char* json);

} // namespace dmLevelPlay

#endif
