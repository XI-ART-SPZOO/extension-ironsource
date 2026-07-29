#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_IOS)

#pragma once

namespace dmLevelPlay {

enum BannerSize
{
    BANNER_SIZE_BANNER = 1,
    BANNER_SIZE_LARGE = 2,
    BANNER_SIZE_MEDIUM_RECTANGLE = 3,
    BANNER_SIZE_LEADERBOARD = 4,
    BANNER_SIZE_ADAPTIVE = 5
};

enum BannerPosition
{
    BANNER_POSITION_TOP = 1,
    BANNER_POSITION_BOTTOM = 2
};

void Initialize_Ext(const char* engineVersion,
                    const char* extensionVersion,
                    bool apsEnabled,
                    const char* apsAppId);
void Finalize_Ext();

void Init(const char* appKey, const char* userId);
const char* GetSdkVersion();
void ValidateIntegration();
void LaunchTestSuite();
void SetGDPRConsent(bool consent);
void SetCCPA(bool optedOut);
void SetCOPPA(bool childDirected);
void SetMetaData(const char* key, const char* value);
bool SetMetaLimitedDataUse(bool enabled, int country, int state);
bool SetMetaAdvertiserTracking(bool enabled);
bool SetDynamicUserId(const char* userId);
void SetAdaptersDebug(bool enabled);

bool IsTrackingAuthorizationSupported();
void RequestTrackingAuthorization();
int GetTrackingAuthorizationStatus();

int CreateInterstitialAd(const char* adUnitId, double bidFloor);
void DestroyInterstitialAd(int handle);
void LoadInterstitialAd(int handle);
bool IsInterstitialAdReady(int handle);
void ShowInterstitialAd(int handle, const char* placementName);
bool IsInterstitialPlacementCapped(const char* placementName);

int CreateRewardedAd(const char* adUnitId, double bidFloor);
void DestroyRewardedAd(int handle);
void LoadRewardedAd(int handle);
bool IsRewardedAdReady(int handle);
void ShowRewardedAd(int handle, const char* placementName);
bool IsRewardedPlacementCapped(const char* placementName);
const char* GetReward(int handle, const char* placementName);

int CreateBannerAd(const char* adUnitId,
                   int size,
                   int position,
                   const char* placementName,
                   double bidFloor,
                   bool respectSafeArea);
void LoadBannerAd(int handle);
void ShowBannerAd(int handle);
void HideBannerAd(int handle);
void PauseBannerAutoRefresh(int handle);
void ResumeBannerAutoRefresh(int handle);
void DestroyBannerAd(int handle);

} // namespace dmLevelPlay

#endif
