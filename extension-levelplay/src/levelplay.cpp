#define EXTENSION_NAME LevelPlayExt
#define LIB_NAME "LevelPlay"
#define MODULE_NAME "levelplay"

#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

#include "levelplay_callback_private.h"
#include "levelplay_private.h"

#include <string.h>

namespace dmLevelPlay {

static const char* OptionalString(lua_State* L, int index)
{
    return lua_isnoneornil(L, index) ? 0 : luaL_checkstring(L, index);
}

static int CheckHandle(lua_State* L, int index)
{
    int handle = luaL_checkint(L, index);
    if (handle <= 0)
    {
        luaL_error(L, "Expected a positive LevelPlay ad handle, got %d.", handle);
    }
    return handle;
}

static void PushHandle(lua_State* L, int handle)
{
    if (handle > 0)
    {
        lua_pushnumber(L, (lua_Number)handle);
    }
    else
    {
        lua_pushnil(L);
    }
}

static bool CheckBoolean(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TBOOLEAN);
    return lua_toboolean(L, index) != 0;
}

static int Lua_Init(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* appKey = luaL_checkstring(L, 1);
    const char* userId = OptionalString(L, 2);
    Init(appKey, userId);
    return 0;
}

static int Lua_SetCallback(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetLuaCallback(L, 1);
    return 0;
}

static int Lua_GetSdkVersion(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* version = GetSdkVersion();
    if (version)
    {
        lua_pushstring(L, version);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static int Lua_ValidateIntegration(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ValidateIntegration();
    return 0;
}

static int Lua_LaunchTestSuite(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    LaunchTestSuite();
    return 0;
}

static int Lua_SetGDPRConsent(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetGDPRConsent(CheckBoolean(L, 1));
    return 0;
}

static int Lua_SetCCPA(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetCCPA(CheckBoolean(L, 1));
    return 0;
}

static int Lua_SetCOPPA(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetCOPPA(CheckBoolean(L, 1));
    return 0;
}

static int Lua_SetMetaData(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetMetaData(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
    return 0;
}

static int Lua_SetMetaLimitedDataUse(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    bool enabled = CheckBoolean(L, 1);
    int country = lua_isnoneornil(L, 2) ? 0 : luaL_checkint(L, 2);
    int state = lua_isnoneornil(L, 3) ? 0 : luaL_checkint(L, 3);
    if (country < 0 || state < 0)
    {
        return luaL_error(L, "Meta LDU country and state codes must be non-negative.");
    }
    lua_pushboolean(L, SetMetaLimitedDataUse(enabled, country, state));
    return 1;
}

static int Lua_SetMetaAdvertiserTracking(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, SetMetaAdvertiserTracking(CheckBoolean(L, 1)));
    return 1;
}

static int Lua_SetDynamicUserId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, SetDynamicUserId(luaL_checkstring(L, 1)));
    return 1;
}

static int Lua_SetAdaptersDebug(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    SetAdaptersDebug(CheckBoolean(L, 1));
    return 0;
}

static int Lua_RequestTrackingAuthorization(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    RequestTrackingAuthorization();
    return 0;
}

static int Lua_GetTrackingAuthorizationStatus(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    int status = GetTrackingAuthorizationStatus();
    if (status < 0)
    {
        lua_pushnil(L);
    }
    else
    {
        lua_pushnumber(L, (lua_Number)status);
    }
    return 1;
}

static int Lua_CreateInterstitialAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* adUnitId = luaL_checkstring(L, 1);
    double bidFloor = lua_isnoneornil(L, 2) ? -1.0 : luaL_checknumber(L, 2);
    PushHandle(L, CreateInterstitialAd(adUnitId, bidFloor));
    return 1;
}

static int Lua_DestroyInterstitialAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    DestroyInterstitialAd(CheckHandle(L, 1));
    return 0;
}

static int Lua_LoadInterstitialAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    LoadInterstitialAd(CheckHandle(L, 1));
    return 0;
}

static int Lua_IsInterstitialAdReady(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, IsInterstitialAdReady(CheckHandle(L, 1)));
    return 1;
}

static int Lua_ShowInterstitialAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int handle = CheckHandle(L, 1);
    ShowInterstitialAd(handle, OptionalString(L, 2));
    return 0;
}

static int Lua_IsInterstitialPlacementCapped(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, IsInterstitialPlacementCapped(luaL_checkstring(L, 1)));
    return 1;
}

static int Lua_CreateRewardedAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* adUnitId = luaL_checkstring(L, 1);
    double bidFloor = lua_isnoneornil(L, 2) ? -1.0 : luaL_checknumber(L, 2);
    PushHandle(L, CreateRewardedAd(adUnitId, bidFloor));
    return 1;
}

static int Lua_DestroyRewardedAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    DestroyRewardedAd(CheckHandle(L, 1));
    return 0;
}

static int Lua_LoadRewardedAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    LoadRewardedAd(CheckHandle(L, 1));
    return 0;
}

static int Lua_IsRewardedAdReady(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, IsRewardedAdReady(CheckHandle(L, 1)));
    return 1;
}

static int Lua_ShowRewardedAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int handle = CheckHandle(L, 1);
    ShowRewardedAd(handle, OptionalString(L, 2));
    return 0;
}

static int Lua_IsRewardedPlacementCapped(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, IsRewardedPlacementCapped(luaL_checkstring(L, 1)));
    return 1;
}

static int Lua_GetReward(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    int handle = CheckHandle(L, 1);
    const char* json = GetReward(handle, OptionalString(L, 2));
    if (!json)
    {
        lua_pushnil(L);
    }
    else
    {
        if (dmScript::JsonToLua(L, json, strlen(json)) != 1)
        {
            lua_pushnil(L);
        }
    }
    return 1;
}

static void ReadBannerOptions(lua_State* L,
                              int index,
                              int* size,
                              int* position,
                              const char** placementName,
                              double* bidFloor,
                              bool* respectSafeArea)
{
    *size = BANNER_SIZE_ADAPTIVE;
    *position = BANNER_POSITION_BOTTOM;
    *placementName = 0;
    *bidFloor = -1.0;
    *respectSafeArea = true;

    if (lua_isnoneornil(L, index))
    {
        return;
    }

    luaL_checktype(L, index, LUA_TTABLE);

    lua_getfield(L, index, "size");
    if (!lua_isnil(L, -1))
    {
        *size = luaL_checkint(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "position");
    if (!lua_isnil(L, -1))
    {
        *position = luaL_checkint(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "placement");
    if (!lua_isnil(L, -1))
    {
        *placementName = luaL_checkstring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "bid_floor");
    if (!lua_isnil(L, -1))
    {
        *bidFloor = luaL_checknumber(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "respect_safe_area");
    if (!lua_isnil(L, -1))
    {
        *respectSafeArea = CheckBoolean(L, -1);
    }
    lua_pop(L, 1);

    if (*size < BANNER_SIZE_BANNER || *size > BANNER_SIZE_ADAPTIVE)
    {
        luaL_error(L, "Unknown banner size constant %d.", *size);
    }
    if (*position != BANNER_POSITION_TOP && *position != BANNER_POSITION_BOTTOM)
    {
        luaL_error(L, "Unknown banner position constant %d.", *position);
    }
}

static int Lua_CreateBannerAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* adUnitId = luaL_checkstring(L, 1);
    int size;
    int position;
    const char* placementName;
    double bidFloor;
    bool respectSafeArea;
    ReadBannerOptions(L, 2, &size, &position, &placementName, &bidFloor, &respectSafeArea);
    PushHandle(L, CreateBannerAd(adUnitId,
                                 size,
                                 position,
                                 placementName,
                                 bidFloor,
                                 respectSafeArea));
    return 1;
}

static int Lua_LoadBannerAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    LoadBannerAd(CheckHandle(L, 1));
    return 0;
}

static int Lua_ShowBannerAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ShowBannerAd(CheckHandle(L, 1));
    return 0;
}

static int Lua_HideBannerAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    HideBannerAd(CheckHandle(L, 1));
    return 0;
}

static int Lua_PauseBannerAutoRefresh(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    PauseBannerAutoRefresh(CheckHandle(L, 1));
    return 0;
}

static int Lua_ResumeBannerAutoRefresh(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    ResumeBannerAutoRefresh(CheckHandle(L, 1));
    return 0;
}

static int Lua_DestroyBannerAd(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    DestroyBannerAd(CheckHandle(L, 1));
    return 0;
}

static const luaL_reg ModuleMethods[] =
{
    {"set_callback", Lua_SetCallback},
    {"init", Lua_Init},
    {"get_sdk_version", Lua_GetSdkVersion},
    {"validate_integration", Lua_ValidateIntegration},
    {"launch_test_suite", Lua_LaunchTestSuite},
    {"set_gdpr_consent", Lua_SetGDPRConsent},
    {"set_ccpa", Lua_SetCCPA},
    {"set_coppa", Lua_SetCOPPA},
    {"set_metadata", Lua_SetMetaData},
    {"set_meta_limited_data_use", Lua_SetMetaLimitedDataUse},
    {"set_meta_advertiser_tracking", Lua_SetMetaAdvertiserTracking},
    {"set_dynamic_user_id", Lua_SetDynamicUserId},
    {"set_adapters_debug", Lua_SetAdaptersDebug},
    {"request_tracking_authorization", Lua_RequestTrackingAuthorization},
    {"get_tracking_authorization_status", Lua_GetTrackingAuthorizationStatus},

    {"create_interstitial_ad", Lua_CreateInterstitialAd},
    {"destroy_interstitial_ad", Lua_DestroyInterstitialAd},
    {"load_interstitial_ad", Lua_LoadInterstitialAd},
    {"is_interstitial_ad_ready", Lua_IsInterstitialAdReady},
    {"show_interstitial_ad", Lua_ShowInterstitialAd},
    {"is_interstitial_placement_capped", Lua_IsInterstitialPlacementCapped},

    {"create_rewarded_ad", Lua_CreateRewardedAd},
    {"destroy_rewarded_ad", Lua_DestroyRewardedAd},
    {"load_rewarded_ad", Lua_LoadRewardedAd},
    {"is_rewarded_ad_ready", Lua_IsRewardedAdReady},
    {"show_rewarded_ad", Lua_ShowRewardedAd},
    {"is_rewarded_placement_capped", Lua_IsRewardedPlacementCapped},
    {"get_reward", Lua_GetReward},

    {"create_banner_ad", Lua_CreateBannerAd},
    {"load_banner_ad", Lua_LoadBannerAd},
    {"show_banner_ad", Lua_ShowBannerAd},
    {"hide_banner_ad", Lua_HideBannerAd},
    {"pause_banner_auto_refresh", Lua_PauseBannerAutoRefresh},
    {"resume_banner_auto_refresh", Lua_ResumeBannerAutoRefresh},
    {"destroy_banner_ad", Lua_DestroyBannerAd},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    luaL_register(L, MODULE_NAME, ModuleMethods);

#define SET_CONSTANT(name)             \
    lua_pushnumber(L, (lua_Number)name); \
    lua_setfield(L, -2, #name)

    SET_CONSTANT(MSG_INIT);
    SET_CONSTANT(MSG_INTERSTITIAL);
    SET_CONSTANT(MSG_REWARDED);
    SET_CONSTANT(MSG_BANNER);
    SET_CONSTANT(MSG_TRACKING);

    SET_CONSTANT(EVENT_INIT_SUCCEEDED);
    SET_CONSTANT(EVENT_INIT_FAILED);
    SET_CONSTANT(EVENT_AD_LOADED);
    SET_CONSTANT(EVENT_AD_LOAD_FAILED);
    SET_CONSTANT(EVENT_AD_INFO_CHANGED);
    SET_CONSTANT(EVENT_AD_DISPLAYED);
    SET_CONSTANT(EVENT_AD_DISPLAY_FAILED);
    SET_CONSTANT(EVENT_AD_CLICKED);
    SET_CONSTANT(EVENT_AD_CLOSED);
    SET_CONSTANT(EVENT_AD_REWARDED);
    SET_CONSTANT(EVENT_AD_EXPANDED);
    SET_CONSTANT(EVENT_AD_COLLAPSED);
    SET_CONSTANT(EVENT_AD_LEFT_APPLICATION);
    SET_CONSTANT(EVENT_JSON_ERROR);

    SET_CONSTANT(TRACKING_STATUS_AUTHORIZED);
    SET_CONSTANT(TRACKING_STATUS_DENIED);
    SET_CONSTANT(TRACKING_STATUS_NOT_DETERMINED);
    SET_CONSTANT(TRACKING_STATUS_RESTRICTED);

    SET_CONSTANT(BANNER_SIZE_BANNER);
    SET_CONSTANT(BANNER_SIZE_LARGE);
    SET_CONSTANT(BANNER_SIZE_MEDIUM_RECTANGLE);
    SET_CONSTANT(BANNER_SIZE_LEADERBOARD);
    SET_CONSTANT(BANNER_SIZE_ADAPTIVE);
    SET_CONSTANT(BANNER_POSITION_TOP);
    SET_CONSTANT(BANNER_POSITION_BOTTOM);

#undef SET_CONSTANT

    lua_pop(L, 1);
}

static dmExtension::Result AppInitialize(dmExtension::AppParams*)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result Initialize(dmExtension::Params* params)
{
    InitializeCallback();
    LuaInit(params->m_L);

    lua_getglobal(params->m_L, "sys");
    lua_getfield(params->m_L, -1, "get_engine_info");
    lua_call(params->m_L, 0, 1);
    lua_getfield(params->m_L, -1, "version");
    const char* engineVersion = lua_tostring(params->m_L, -1);
    const char* extensionVersion =
        dmConfigFile::GetString(params->m_ConfigFile, "levelplay.version", "0.0.0");
#if defined(DM_PLATFORM_ANDROID)
    const char* apsEnabledKey = "levelplay.aps_android";
    const char* apsAppIdKey = "levelplay.aps_android_app_id";
#else
    const char* apsEnabledKey = "levelplay.aps_ios";
    const char* apsAppIdKey = "levelplay.aps_ios_app_id";
#endif
    bool apsEnabled = dmConfigFile::GetInt(params->m_ConfigFile, apsEnabledKey, 0) != 0;
    const char* apsAppId =
        dmConfigFile::GetString(params->m_ConfigFile, apsAppIdKey, "");
    Initialize_Ext(engineVersion ? engineVersion : "unknown",
                   extensionVersion,
                   apsEnabled,
                   apsAppId);
    lua_pop(params->m_L, 3);

    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalize(dmExtension::AppParams*)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result Finalize(dmExtension::Params*)
{
    Finalize_Ext();
    FinalizeCallback();
    return dmExtension::RESULT_OK;
}

static dmExtension::Result Update(dmExtension::Params*)
{
    UpdateCallback();
    return dmExtension::RESULT_OK;
}

} // namespace dmLevelPlay

DM_DECLARE_EXTENSION(EXTENSION_NAME,
                     LIB_NAME,
                     dmLevelPlay::AppInitialize,
                     dmLevelPlay::AppFinalize,
                     dmLevelPlay::Initialize,
                     dmLevelPlay::Update,
                     0,
                     dmLevelPlay::Finalize)

#else

static dmExtension::Result InitializeLevelPlay(dmExtension::Params*)
{
    dmLogInfo("Registered LevelPlay extension (mobile platforms only)");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeLevelPlay(dmExtension::Params*)
{
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME,
                     LIB_NAME,
                     0,
                     0,
                     InitializeLevelPlay,
                     0,
                     0,
                     FinalizeLevelPlay)

#endif
