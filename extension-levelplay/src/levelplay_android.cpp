#if defined(DM_PLATFORM_ANDROID)

#include <dmsdk/dlib/android.h>

#include "com_defold_levelplay_LevelPlayJNI.h"
#include "levelplay_callback_private.h"
#include "levelplay_private.h"

#include <string>

JNIEXPORT void JNICALL Java_com_defold_levelplay_LevelPlayJNI_callback(
    JNIEnv* env,
    jclass,
    jint message,
    jint event,
    jint handle,
    jstring json)
{
    const char* payload = 0;
    if (json)
    {
        payload = env->GetStringUTFChars(json, 0);
    }
    dmLevelPlay::AddToQueueCallback(message, event, handle, payload);
    if (payload)
    {
        env->ReleaseStringUTFChars(json, payload);
    }
}

namespace dmLevelPlay {

struct AndroidState
{
    jobject instance;

    jmethodID destroyAll;
    jmethodID init;
    jmethodID getSdkVersion;
    jmethodID validateIntegration;
    jmethodID launchTestSuite;
    jmethodID setGDPRConsent;
    jmethodID setCCPA;
    jmethodID setCOPPA;
    jmethodID setMetaData;
    jmethodID setMetaLimitedDataUse;
    jmethodID setMetaAdvertiserTracking;
    jmethodID setDynamicUserId;
    jmethodID setAdaptersDebug;

    jmethodID createInterstitial;
    jmethodID destroyInterstitial;
    jmethodID loadInterstitial;
    jmethodID isInterstitialReady;
    jmethodID showInterstitial;
    jmethodID isInterstitialPlacementCapped;

    jmethodID createRewarded;
    jmethodID destroyRewarded;
    jmethodID loadRewarded;
    jmethodID isRewardedReady;
    jmethodID showRewarded;
    jmethodID isRewardedPlacementCapped;
    jmethodID getReward;

    jmethodID createBanner;
    jmethodID loadBanner;
    jmethodID showBanner;
    jmethodID hideBanner;
    jmethodID pauseBannerAutoRefresh;
    jmethodID resumeBannerAutoRefresh;
    jmethodID destroyBanner;

    std::string returnValue;
};

static AndroidState g_State = {};

static bool ClearException(JNIEnv* env, const char* operation)
{
    if (!env->ExceptionCheck())
    {
        return false;
    }
    dmLogError("LevelPlay Java exception during %s.", operation);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
}

static jmethodID GetMethod(JNIEnv* env, jclass cls, const char* name, const char* signature)
{
    jmethodID method = env->GetMethodID(cls, name, signature);
    if (!method)
    {
        ClearException(env, name);
        dmLogError("LevelPlayJNI method not found: %s %s.", name, signature);
    }
    return method;
}

static jstring NewOptionalString(JNIEnv* env, const char* value)
{
    return value ? env->NewStringUTF(value) : 0;
}

static void CallVoid(jmethodID method, const char* operation)
{
    if (!g_State.instance || !method)
    {
        return;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    env->CallVoidMethod(g_State.instance, method);
    ClearException(env, operation);
}

static void CallVoidBool(jmethodID method, bool value, const char* operation)
{
    if (!g_State.instance || !method)
    {
        return;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    env->CallVoidMethod(g_State.instance, method, (jboolean)value);
    ClearException(env, operation);
}

static void CallVoidInt(jmethodID method, int value, const char* operation)
{
    if (!g_State.instance || !method)
    {
        return;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    env->CallVoidMethod(g_State.instance, method, (jint)value);
    ClearException(env, operation);
}

static void CallVoidIntString(jmethodID method,
                              int value,
                              const char* text,
                              const char* operation)
{
    if (!g_State.instance || !method)
    {
        return;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring string = NewOptionalString(env, text);
    env->CallVoidMethod(g_State.instance, method, (jint)value, string);
    ClearException(env, operation);
    if (string)
    {
        env->DeleteLocalRef(string);
    }
}

static bool CallBoolInt(jmethodID method, int value, const char* operation)
{
    if (!g_State.instance || !method)
    {
        return false;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jboolean result = env->CallBooleanMethod(g_State.instance, method, (jint)value);
    return !ClearException(env, operation) && result == JNI_TRUE;
}

static bool CallBoolString(jmethodID method, const char* value, const char* operation)
{
    if (!g_State.instance || !method)
    {
        return false;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring string = NewOptionalString(env, value);
    jboolean result = env->CallBooleanMethod(g_State.instance, method, string);
    bool failed = ClearException(env, operation);
    if (string)
    {
        env->DeleteLocalRef(string);
    }
    return !failed && result == JNI_TRUE;
}

static const char* CopyStringResult(JNIEnv* env, jstring value, const char* operation)
{
    g_State.returnValue.clear();
    if (ClearException(env, operation))
    {
        if (value)
        {
            env->DeleteLocalRef(value);
        }
        return 0;
    }
    if (!value)
    {
        return 0;
    }
    const char* chars = env->GetStringUTFChars(value, 0);
    bool copied = chars != 0;
    if (chars)
    {
        g_State.returnValue.assign(chars);
        env->ReleaseStringUTFChars(value, chars);
    }
    env->DeleteLocalRef(value);
    return copied ? g_State.returnValue.c_str() : 0;
}

static void BindMethods(JNIEnv* env, jclass cls)
{
    g_State.destroyAll = GetMethod(env, cls, "destroyAll", "()V");
    g_State.init = GetMethod(env, cls, "init", "(Ljava/lang/String;Ljava/lang/String;)V");
    g_State.getSdkVersion =
        GetMethod(env, cls, "getSdkVersion", "()Ljava/lang/String;");
    g_State.validateIntegration = GetMethod(env, cls, "validateIntegration", "()V");
    g_State.launchTestSuite = GetMethod(env, cls, "launchTestSuite", "()V");
    g_State.setGDPRConsent = GetMethod(env, cls, "setGDPRConsent", "(Z)V");
    g_State.setCCPA = GetMethod(env, cls, "setCCPA", "(Z)V");
    g_State.setCOPPA = GetMethod(env, cls, "setCOPPA", "(Z)V");
    g_State.setMetaData =
        GetMethod(env, cls, "setMetaData", "(Ljava/lang/String;Ljava/lang/String;)V");
    g_State.setMetaLimitedDataUse =
        GetMethod(env, cls, "setMetaLimitedDataUse", "(ZII)Z");
    g_State.setMetaAdvertiserTracking =
        GetMethod(env, cls, "setMetaAdvertiserTracking", "(Z)Z");
    g_State.setDynamicUserId =
        GetMethod(env, cls, "setDynamicUserId", "(Ljava/lang/String;)Z");
    g_State.setAdaptersDebug = GetMethod(env, cls, "setAdaptersDebug", "(Z)V");

    g_State.createInterstitial =
        GetMethod(env, cls, "createInterstitial", "(Ljava/lang/String;D)I");
    g_State.destroyInterstitial = GetMethod(env, cls, "destroyInterstitial", "(I)V");
    g_State.loadInterstitial = GetMethod(env, cls, "loadInterstitial", "(I)V");
    g_State.isInterstitialReady = GetMethod(env, cls, "isInterstitialReady", "(I)Z");
    g_State.showInterstitial =
        GetMethod(env, cls, "showInterstitial", "(ILjava/lang/String;)V");
    g_State.isInterstitialPlacementCapped =
        GetMethod(env, cls, "isInterstitialPlacementCapped", "(Ljava/lang/String;)Z");

    g_State.createRewarded = GetMethod(env, cls, "createRewarded", "(Ljava/lang/String;D)I");
    g_State.destroyRewarded = GetMethod(env, cls, "destroyRewarded", "(I)V");
    g_State.loadRewarded = GetMethod(env, cls, "loadRewarded", "(I)V");
    g_State.isRewardedReady = GetMethod(env, cls, "isRewardedReady", "(I)Z");
    g_State.showRewarded = GetMethod(env, cls, "showRewarded", "(ILjava/lang/String;)V");
    g_State.isRewardedPlacementCapped =
        GetMethod(env, cls, "isRewardedPlacementCapped", "(Ljava/lang/String;)Z");
    g_State.getReward =
        GetMethod(env, cls, "getReward", "(ILjava/lang/String;)Ljava/lang/String;");

    g_State.createBanner =
        GetMethod(env, cls, "createBanner", "(Ljava/lang/String;IILjava/lang/String;DZ)I");
    g_State.loadBanner = GetMethod(env, cls, "loadBanner", "(I)V");
    g_State.showBanner = GetMethod(env, cls, "showBanner", "(I)V");
    g_State.hideBanner = GetMethod(env, cls, "hideBanner", "(I)V");
    g_State.pauseBannerAutoRefresh =
        GetMethod(env, cls, "pauseBanner", "(I)V");
    g_State.resumeBannerAutoRefresh =
        GetMethod(env, cls, "resumeBanner", "(I)V");
    g_State.destroyBanner = GetMethod(env, cls, "destroyBanner", "(I)V");
}

void Initialize_Ext(const char* engineVersion,
                    const char* extensionVersion,
                    bool apsEnabled,
                    const char* apsAppId)
{
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jclass cls = dmAndroid::LoadClass(env, "com.defold.levelplay.LevelPlayJNI");
    bool classLoadFailed = ClearException(env, "load LevelPlayJNI");
    if (!cls || classLoadFailed)
    {
        dmLogError("Unable to load com.defold.levelplay.LevelPlayJNI.");
        if (cls)
        {
            env->DeleteLocalRef(cls);
        }
        return;
    }

    BindMethods(env, cls);
    jmethodID constructor = GetMethod(
        env,
        cls,
        "<init>",
        "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;ZLjava/lang/String;)V");
    if (!constructor)
    {
        env->DeleteLocalRef(cls);
        return;
    }

    jstring engine = env->NewStringUTF(engineVersion);
    jstring extension = env->NewStringUTF(extensionVersion);
    jstring apsAppIdString = NewOptionalString(env, apsAppId);
    jobject local = env->NewObject(
        cls,
        constructor,
        attacher.GetActivity()->clazz,
        engine,
        extension,
        (jboolean)apsEnabled,
        apsAppIdString);
    if (!ClearException(env, "construct LevelPlayJNI") && local)
    {
        g_State.instance = env->NewGlobalRef(local);
        env->DeleteLocalRef(local);
    }
    env->DeleteLocalRef(engine);
    env->DeleteLocalRef(extension);
    if (apsAppIdString)
    {
        env->DeleteLocalRef(apsAppIdString);
    }
    env->DeleteLocalRef(cls);
}

void Finalize_Ext()
{
    if (!g_State.instance)
    {
        return;
    }
    CallVoid(g_State.destroyAll, "destroyAll");
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    env->DeleteGlobalRef(g_State.instance);
    g_State.instance = 0;
    g_State.returnValue.clear();
}

void Init(const char* appKey, const char* userId)
{
    if (!g_State.instance || !g_State.init)
    {
        return;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring appKeyString = env->NewStringUTF(appKey);
    jstring userIdString = NewOptionalString(env, userId);
    env->CallVoidMethod(g_State.instance, g_State.init, appKeyString, userIdString);
    ClearException(env, "init");
    env->DeleteLocalRef(appKeyString);
    if (userIdString)
    {
        env->DeleteLocalRef(userIdString);
    }
}

const char* GetSdkVersion()
{
    if (!g_State.instance || !g_State.getSdkVersion)
    {
        return 0;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    return CopyStringResult(
        env,
        (jstring)env->CallObjectMethod(g_State.instance, g_State.getSdkVersion),
        "getSdkVersion");
}

void ValidateIntegration()
{
    CallVoid(g_State.validateIntegration, "validateIntegration");
}

void LaunchTestSuite()
{
    CallVoid(g_State.launchTestSuite, "launchTestSuite");
}

void SetGDPRConsent(bool consent)
{
    CallVoidBool(g_State.setGDPRConsent, consent, "setGDPRConsent");
}

void SetCCPA(bool optedOut)
{
    CallVoidBool(g_State.setCCPA, optedOut, "setCCPA");
}

void SetCOPPA(bool childDirected)
{
    CallVoidBool(g_State.setCOPPA, childDirected, "setCOPPA");
}

void SetMetaData(const char* key, const char* value)
{
    if (!g_State.instance || !g_State.setMetaData)
    {
        return;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring keyString = env->NewStringUTF(key);
    jstring valueString = env->NewStringUTF(value);
    env->CallVoidMethod(g_State.instance, g_State.setMetaData, keyString, valueString);
    ClearException(env, "setMetaData");
    env->DeleteLocalRef(keyString);
    env->DeleteLocalRef(valueString);
}

bool SetMetaLimitedDataUse(bool enabled, int country, int state)
{
    if (!g_State.instance || !g_State.setMetaLimitedDataUse)
    {
        return false;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jboolean result = env->CallBooleanMethod(g_State.instance,
                                             g_State.setMetaLimitedDataUse,
                                             (jboolean)enabled,
                                             (jint)country,
                                             (jint)state);
    return !ClearException(env, "setMetaLimitedDataUse") && result == JNI_TRUE;
}

bool SetMetaAdvertiserTracking(bool enabled)
{
    if (!g_State.instance || !g_State.setMetaAdvertiserTracking)
    {
        return false;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jboolean result = env->CallBooleanMethod(g_State.instance,
                                             g_State.setMetaAdvertiserTracking,
                                             (jboolean)enabled);
    return !ClearException(env, "setMetaAdvertiserTracking") &&
           result == JNI_TRUE;
}

bool SetDynamicUserId(const char* userId)
{
    return CallBoolString(g_State.setDynamicUserId, userId, "setDynamicUserId");
}

void SetAdaptersDebug(bool enabled)
{
    CallVoidBool(g_State.setAdaptersDebug, enabled, "setAdaptersDebug");
}

bool IsTrackingAuthorizationSupported()
{
    return false;
}

void RequestTrackingAuthorization()
{
}

int GetTrackingAuthorizationStatus()
{
    return -1;
}

int CreateInterstitialAd(const char* adUnitId, double bidFloor)
{
    if (!g_State.instance || !g_State.createInterstitial)
    {
        return 0;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring id = env->NewStringUTF(adUnitId);
    jint result =
        env->CallIntMethod(g_State.instance, g_State.createInterstitial, id, (jdouble)bidFloor);
    bool failed = ClearException(env, "createInterstitial");
    env->DeleteLocalRef(id);
    return failed ? 0 : (int)result;
}

void DestroyInterstitialAd(int handle)
{
    CallVoidInt(g_State.destroyInterstitial, handle, "destroyInterstitial");
}

void LoadInterstitialAd(int handle)
{
    CallVoidInt(g_State.loadInterstitial, handle, "loadInterstitial");
}

bool IsInterstitialAdReady(int handle)
{
    return CallBoolInt(g_State.isInterstitialReady, handle, "isInterstitialReady");
}

void ShowInterstitialAd(int handle, const char* placementName)
{
    CallVoidIntString(g_State.showInterstitial, handle, placementName, "showInterstitial");
}

bool IsInterstitialPlacementCapped(const char* placementName)
{
    return CallBoolString(
        g_State.isInterstitialPlacementCapped,
        placementName,
        "isInterstitialPlacementCapped");
}

int CreateRewardedAd(const char* adUnitId, double bidFloor)
{
    if (!g_State.instance || !g_State.createRewarded)
    {
        return 0;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring id = env->NewStringUTF(adUnitId);
    jint result = env->CallIntMethod(g_State.instance, g_State.createRewarded, id, (jdouble)bidFloor);
    bool failed = ClearException(env, "createRewarded");
    env->DeleteLocalRef(id);
    return failed ? 0 : (int)result;
}

void DestroyRewardedAd(int handle)
{
    CallVoidInt(g_State.destroyRewarded, handle, "destroyRewarded");
}

void LoadRewardedAd(int handle)
{
    CallVoidInt(g_State.loadRewarded, handle, "loadRewarded");
}

bool IsRewardedAdReady(int handle)
{
    return CallBoolInt(g_State.isRewardedReady, handle, "isRewardedReady");
}

void ShowRewardedAd(int handle, const char* placementName)
{
    CallVoidIntString(g_State.showRewarded, handle, placementName, "showRewarded");
}

bool IsRewardedPlacementCapped(const char* placementName)
{
    return CallBoolString(
        g_State.isRewardedPlacementCapped,
        placementName,
        "isRewardedPlacementCapped");
}

const char* GetReward(int handle, const char* placementName)
{
    if (!g_State.instance || !g_State.getReward)
    {
        return 0;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring placement = NewOptionalString(env, placementName);
    jstring result = (jstring)env->CallObjectMethod(
        g_State.instance,
        g_State.getReward,
        (jint)handle,
        placement);
    if (placement)
    {
        env->DeleteLocalRef(placement);
    }
    return CopyStringResult(env, result, "getReward");
}

int CreateBannerAd(const char* adUnitId,
                   int size,
                   int position,
                   const char* placementName,
                   double bidFloor,
                   bool respectSafeArea)
{
    if (!g_State.instance || !g_State.createBanner)
    {
        return 0;
    }
    dmAndroid::ThreadAttacher attacher;
    JNIEnv* env = attacher.GetEnv();
    jstring id = env->NewStringUTF(adUnitId);
    jstring placement = NewOptionalString(env, placementName);
    jint result = env->CallIntMethod(
        g_State.instance,
        g_State.createBanner,
        id,
        (jint)size,
        (jint)position,
        placement,
        (jdouble)bidFloor,
        (jboolean)respectSafeArea);
    bool failed = ClearException(env, "createBanner");
    env->DeleteLocalRef(id);
    if (placement)
    {
        env->DeleteLocalRef(placement);
    }
    return failed ? 0 : (int)result;
}

void LoadBannerAd(int handle)
{
    CallVoidInt(g_State.loadBanner, handle, "loadBanner");
}

void ShowBannerAd(int handle)
{
    CallVoidInt(g_State.showBanner, handle, "showBanner");
}

void HideBannerAd(int handle)
{
    CallVoidInt(g_State.hideBanner, handle, "hideBanner");
}

void PauseBannerAutoRefresh(int handle)
{
    CallVoidInt(g_State.pauseBannerAutoRefresh, handle, "pauseBannerAutoRefresh");
}

void ResumeBannerAutoRefresh(int handle)
{
    CallVoidInt(g_State.resumeBannerAutoRefresh, handle, "resumeBannerAutoRefresh");
}

void DestroyBannerAd(int handle)
{
    CallVoidInt(g_State.destroyBanner, handle, "destroyBanner");
}

} // namespace dmLevelPlay

#endif
