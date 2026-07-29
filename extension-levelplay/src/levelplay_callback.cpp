#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

#include "levelplay_callback_private.h"

#include <stdlib.h>
#include <string.h>

namespace dmLevelPlay {

static dmScript::LuaCallbackInfo* g_LuaCallback = 0;
static dmArray<CallbackData> g_CallbackQueue;
static dmMutex::HMutex g_CallbackMutex = 0;
static bool g_CallbackActive = false;
static uint32_t g_CallbackInvocationDepth = 0;
static dmArray<dmScript::LuaCallbackInfo*> g_DeferredCallbacks;

// A Lua callback may replace or remove itself. Keep every retired callback
// rooted until the outermost invocation has restored the script context.
static void FlushDeferredCallbacks()
{
    if (g_CallbackInvocationDepth != 0)
    {
        return;
    }
    for (uint32_t i = 0; i < g_DeferredCallbacks.Size(); ++i)
    {
        dmScript::DestroyCallback(g_DeferredCallbacks[i]);
    }
    g_DeferredCallbacks.SetSize(0);
}

static void RetireCallback(dmScript::LuaCallbackInfo* callback)
{
    if (!callback)
    {
        return;
    }
    if (g_CallbackInvocationDepth == 0)
    {
        dmScript::DestroyCallback(callback);
        return;
    }
    if (g_DeferredCallbacks.Full())
    {
        g_DeferredCallbacks.OffsetCapacity(4);
    }
    g_DeferredCallbacks.Push(callback);
}

static void DestroyCallback()
{
    dmScript::LuaCallbackInfo* callback = g_LuaCallback;
    g_LuaCallback = 0;
    RetireCallback(callback);
}

static void CopyJsonFields(lua_State* L, const char* json, int destination)
{
    if (!json || json[0] == '\0')
    {
        return;
    }

    if (dmScript::JsonToLua(L, json, strlen(json)) != 1)
    {
        return;
    }
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return;
    }

    int source = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, source) != 0)
    {
        lua_pushvalue(L, -2);
        lua_pushvalue(L, -2);
        lua_settable(L, destination);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

static void InvokeCallback(const CallbackData& data)
{
    dmScript::LuaCallbackInfo* callback = g_LuaCallback;
    if (!dmScript::IsCallbackValid(callback))
    {
        dmLogError("LevelPlay callback is invalid. Set it with levelplay.set_callback().");
        return;
    }

    lua_State* L = dmScript::GetCallbackLuaContext(callback);
    int top = lua_gettop(L);
    if (!dmScript::SetupCallback(callback))
    {
        return;
    }

    lua_pushnumber(L, (lua_Number)data.msg);
    lua_newtable(L);
    int message = lua_gettop(L);

    lua_pushnumber(L, (lua_Number)data.event);
    lua_setfield(L, message, "event");
    if (data.handle > 0)
    {
        lua_pushnumber(L, (lua_Number)data.handle);
        lua_setfield(L, message, "handle");
    }
    CopyJsonFields(L, data.json, message);

    ++g_CallbackInvocationDepth;
    dmScript::PCall(L, 3, 0);
    dmScript::TeardownCallback(callback);
    assert(top == lua_gettop(L));
    assert(g_CallbackInvocationDepth > 0);
    --g_CallbackInvocationDepth;
    FlushDeferredCallbacks();
}

void InitializeCallback()
{
    if (!g_CallbackMutex)
    {
        g_CallbackMutex = dmMutex::New();
    }
    DM_MUTEX_SCOPED_LOCK(g_CallbackMutex);
    g_CallbackActive = true;
}

void FinalizeCallback()
{
    if (!g_CallbackMutex)
    {
        DestroyCallback();
        FlushDeferredCallbacks();
        return;
    }
    {
        DM_MUTEX_SCOPED_LOCK(g_CallbackMutex);
        g_CallbackActive = false;
        for (uint32_t i = 0; i < g_CallbackQueue.Size(); ++i)
        {
            free(g_CallbackQueue[i].json);
        }
        g_CallbackQueue.SetSize(0);
    }
    // Keep the mutex alive for the process lifetime. A native SDK callback
    // already in flight may still reach AddToQueueCallback after finalization;
    // deleting the mutex here would turn that harmless late callback into a
    // use-after-free.
    DestroyCallback();
    FlushDeferredCallbacks();
}

void SetLuaCallback(lua_State* L, int pos)
{
    dmScript::LuaCallbackInfo* replacement = 0;
    if (!lua_isnoneornil(L, pos))
    {
        luaL_checktype(L, pos, LUA_TFUNCTION);
        replacement = dmScript::CreateCallback(L, pos);
    }

    dmScript::LuaCallbackInfo* previous = g_LuaCallback;
    g_LuaCallback = replacement;
    RetireCallback(previous);
}

void AddToQueueCallback(int message, int event, int handle, const char* json)
{
    CallbackData data;
    data.msg = (MessageId)message;
    data.event = (MessageEvent)event;
    data.handle = handle;
    data.json = json ? strdup(json) : 0;

    if (!g_CallbackMutex)
    {
        free(data.json);
        return;
    }
    DM_MUTEX_SCOPED_LOCK(g_CallbackMutex);
    if (!g_CallbackActive)
    {
        free(data.json);
        return;
    }
    if (g_CallbackQueue.Full())
    {
        g_CallbackQueue.OffsetCapacity(8);
    }
    g_CallbackQueue.Push(data);
}

void UpdateCallback()
{
    if (!g_CallbackMutex)
    {
        return;
    }

    dmArray<CallbackData> pending;
    {
        DM_MUTEX_SCOPED_LOCK(g_CallbackMutex);
        if (!g_CallbackActive || g_CallbackQueue.Empty())
        {
            return;
        }
        pending.Swap(g_CallbackQueue);
    }

    for (uint32_t i = 0; i < pending.Size(); ++i)
    {
        InvokeCallback(pending[i]);
        free(pending[i].json);
    }
}

} // namespace dmLevelPlay

#endif
