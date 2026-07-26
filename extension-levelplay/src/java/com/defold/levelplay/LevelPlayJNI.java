package com.defold.levelplay;

import android.app.Activity;
import android.os.Build;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.widget.FrameLayout;

import com.unity3d.mediation.LevelPlay;
import com.unity3d.mediation.LevelPlayAdError;
import com.unity3d.mediation.LevelPlayAdInfo;
import com.unity3d.mediation.LevelPlayAdSize;
import com.unity3d.mediation.LevelPlayConfiguration;
import com.unity3d.mediation.LevelPlayInitError;
import com.unity3d.mediation.LevelPlayInitListener;
import com.unity3d.mediation.LevelPlayInitRequest;
import com.unity3d.mediation.LevelPlayPrivacySettings;
import com.unity3d.mediation.banner.LevelPlayBannerAdView;
import com.unity3d.mediation.banner.LevelPlayBannerAdViewListener;
import com.unity3d.mediation.interstitial.LevelPlayInterstitialAd;
import com.unity3d.mediation.interstitial.LevelPlayInterstitialAdListener;
import com.unity3d.mediation.rewarded.LevelPlayReward;
import com.unity3d.mediation.rewarded.LevelPlayRewardedAd;
import com.unity3d.mediation.rewarded.LevelPlayRewardedAdListener;

import org.json.JSONException;
import org.json.JSONObject;

import java.lang.reflect.Method;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Java bridge for the LevelPlay 9 object-based ad unit APIs.
 *
 * Each interstitial, rewarded, and banner instance has its own integer handle.
 * SDK callbacks can therefore always be routed back to the Lua object that
 * initiated the operation.
 */
public final class LevelPlayJNI {
    private static final String TAG = "DefoldLevelPlay";

    // These values are mirrored by the native extension.
    private static final int MSG_INIT = 1;
    private static final int MSG_INTERSTITIAL = 2;
    private static final int MSG_REWARDED = 3;
    private static final int MSG_BANNER = 4;

    private static final int EVENT_INIT_SUCCEEDED = 1;
    private static final int EVENT_INIT_FAILED = 2;
    private static final int EVENT_AD_LOADED = 3;
    private static final int EVENT_AD_LOAD_FAILED = 4;
    private static final int EVENT_AD_INFO_CHANGED = 5;
    private static final int EVENT_AD_DISPLAYED = 6;
    private static final int EVENT_AD_DISPLAY_FAILED = 7;
    private static final int EVENT_AD_CLICKED = 8;
    private static final int EVENT_AD_CLOSED = 9;
    private static final int EVENT_AD_REWARDED = 10;
    private static final int EVENT_AD_EXPANDED = 11;
    private static final int EVENT_AD_COLLAPSED = 12;
    private static final int EVENT_AD_LEFT_APPLICATION = 13;
    private static final int EVENT_JSON_ERROR = 100;

    private static final int ERROR_NOT_INITIALIZED = -1;
    private static final int ERROR_INVALID_ARGUMENT = -2;
    private static final int ERROR_INVALID_HANDLE = -3;
    private static final int ERROR_CREATE_FAILED = -4;
    private static final int ERROR_NETWORK_CONFIGURATION = -5;

    private static final int BANNER_SIZE_BANNER = 1;
    private static final int BANNER_SIZE_LARGE = 2;
    private static final int BANNER_SIZE_MEDIUM_RECTANGLE = 3;
    private static final int BANNER_SIZE_LEADERBOARD = 4;
    private static final int BANNER_SIZE_ADAPTIVE = 5;

    private static final int BANNER_POSITION_TOP = 1;
    private static final int BANNER_POSITION_BOTTOM = 2;

    private static native void callback(int messageId, int event, int handle, String json);

    private final Activity activity;
    private final boolean apsEnabled;
    private final String apsAppId;
    private final AtomicInteger nextHandle = new AtomicInteger(1);
    private final ConcurrentHashMap<Integer, InterstitialRecord> interstitials =
            new ConcurrentHashMap<Integer, InterstitialRecord>();
    private final ConcurrentHashMap<Integer, RewardedRecord> rewardedAds =
            new ConcurrentHashMap<Integer, RewardedRecord>();
    private final ConcurrentHashMap<Integer, BannerRecord> banners =
            new ConcurrentHashMap<Integer, BannerRecord>();

    private volatile boolean initializing;
    private volatile boolean initialized;
    private volatile boolean destroyed;
    private volatile boolean apsConfigured;

    public LevelPlayJNI(
            Activity activity,
            String engineVersion,
            String extensionVersion,
            boolean apsEnabled,
            String apsAppId) {
        this.activity = activity;
        this.apsEnabled = apsEnabled;
        this.apsAppId = apsAppId;
        Log.i(TAG, "Engine version: " + engineVersion + ", extension version: " + extensionVersion);
    }

    // ---------------------------------------------------------------------
    // SDK initialization and settings

    public synchronized void init(String appKey, String userId) {
        if (destroyed) {
            return;
        }
        if (appKey == null || appKey.trim().length() == 0) {
            sendBridgeError(MSG_INIT, EVENT_INIT_FAILED, 0, ERROR_INVALID_ARGUMENT,
                    "The LevelPlay app key must not be empty.", "init");
            return;
        }
        if (initializing || initialized) {
            sendBridgeError(MSG_INIT, EVENT_INIT_FAILED, 0, ERROR_INVALID_ARGUMENT,
                    "LevelPlay initialization has already started.", "init");
            return;
        }

        if (!configureAps()) {
            return;
        }

        try {
            initializing = true;
            LevelPlayInitRequest.Builder builder = new LevelPlayInitRequest.Builder(appKey);
            if (userId != null && userId.length() > 0) {
                builder.withUserId(userId);
            }

            LevelPlay.init(
                    activity.getApplicationContext(),
                    builder.build(),
                    new LevelPlayInitListener() {
                        @Override
                        public void onInitSuccess(LevelPlayConfiguration configuration) {
                            if (destroyed) {
                                return;
                            }
                            initializing = false;
                            initialized = true;
                            try {
                                JSONObject json = new JSONObject();
                                if (configuration != null) {
                                    json.put(
                                            "ad_quality_enabled",
                                            configuration.isAdQualityEnabled());
                                    putNullable(json, "ab", configuration.getAb());
                                }
                                send(MSG_INIT, EVENT_INIT_SUCCEEDED, 0, json);
                            } catch (JSONException exception) {
                                sendJsonError(MSG_INIT, 0, exception);
                            }
                        }

                        @Override
                        public void onInitFailed(LevelPlayInitError error) {
                            if (destroyed) {
                                return;
                            }
                            initializing = false;
                            initialized = false;
                            send(MSG_INIT, EVENT_INIT_FAILED, 0, errorToJson(error));
                        }
                    });
        } catch (Throwable throwable) {
            initializing = false;
            initialized = false;
            sendBridgeError(
                    MSG_INIT,
                    EVENT_INIT_FAILED,
                    0,
                    ERROR_CREATE_FAILED,
                    throwableMessage(throwable),
                    "init");
        }
    }

    public void setGDPRConsent(boolean consent) {
        LevelPlayPrivacySettings.setGDPRConsent(consent);
    }

    public void setCCPA(boolean doNotSell) {
        LevelPlayPrivacySettings.setCCPA(doNotSell);
    }

    public void setCOPPA(boolean childDirected) {
        LevelPlayPrivacySettings.setCOPPA(childDirected);
    }

    public void setMetaData(String key, String value) {
        LevelPlay.setMetaData(key, value);
    }

    public synchronized boolean setMetaLimitedDataUse(
            boolean enabled, int country, int state) {
        if (initializing || initialized || destroyed) {
            return false;
        }
        try {
            Class<?> settings = Class.forName("com.facebook.ads.AdSettings");
            String[] options = enabled ? new String[] {"LDU"} : new String[0];
            if (enabled) {
                Method setter = settings.getMethod(
                        "setDataProcessingOptions",
                        String[].class,
                        int.class,
                        int.class);
                setter.invoke(null, options, country, state);
            } else {
                Method setter = settings.getMethod(
                        "setDataProcessingOptions", String[].class);
                setter.invoke(null, (Object) options);
            }
            return true;
        } catch (Throwable throwable) {
            Log.e(TAG, "Unable to configure Meta Limited Data Use.", throwable);
            return false;
        }
    }

    public boolean setMetaAdvertiserTracking(boolean enabled) {
        return false;
    }

    public boolean setDynamicUserId(String userId) {
        return LevelPlay.setDynamicUserId(userId);
    }

    public void setAdaptersDebug(boolean enabled) {
        LevelPlay.setAdaptersDebug(enabled);
    }

    public void validateIntegration() {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                LevelPlay.validateIntegration(activity);
            }
        });
    }

    public void launchTestSuite() {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                LevelPlay.launchTestSuite(activity.getApplicationContext());
            }
        });
    }

    public String getSdkVersion() {
        return LevelPlay.getSdkVersion();
    }

    public void destroyAll() {
        destroyed = true;
        initialized = false;
        initializing = false;
        final InterstitialRecord[] interstitialRecords =
                interstitials.values().toArray(new InterstitialRecord[0]);
        final RewardedRecord[] rewardedRecords =
                rewardedAds.values().toArray(new RewardedRecord[0]);
        final BannerRecord[] bannerRecords =
                banners.values().toArray(new BannerRecord[0]);
        interstitials.clear();
        rewardedAds.clear();
        banners.clear();
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (InterstitialRecord record : interstitialRecords) {
                    if (record != null && record.ad != null) {
                        record.ad.setListener(null);
                        record.ad = null;
                    }
                }
                for (RewardedRecord record : rewardedRecords) {
                    if (record != null && record.ad != null) {
                        record.ad.setListener(null);
                        record.ad = null;
                    }
                }
                for (BannerRecord record : bannerRecords) {
                    if (record == null || record.adView == null) {
                        continue;
                    }
                    record.adView.setBannerListener(null);
                    record.adView.destroy();
                    ViewGroup parent = (ViewGroup) record.adView.getParent();
                    if (parent != null) {
                        parent.removeView(record.adView);
                    }
                    record.adView = null;
                }
            }
        });
    }

    // ---------------------------------------------------------------------
    // Interstitial ad units

    public int createInterstitial(final String adUnitId, final double bidFloor) {
        if (!canCreateAd(MSG_INTERSTITIAL, adUnitId, "create_interstitial")) {
            return 0;
        }

        final int handle = allocateHandle();
        final InterstitialRecord record = new InterstitialRecord();
        final Throwable[] failure = new Throwable[1];
        boolean completed = runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    LevelPlayInterstitialAd ad;
                    if (bidFloor >= 0.0) {
                        LevelPlayInterstitialAd.Config config =
                                new LevelPlayInterstitialAd.Config.Builder()
                                        .setBidFloor(bidFloor)
                                        .build();
                        ad = new LevelPlayInterstitialAd(adUnitId, config);
                    } else {
                        ad = new LevelPlayInterstitialAd(adUnitId);
                    }
                    ad.setListener(new InterstitialListener(handle));
                    record.ad = ad;
                    interstitials.put(handle, record);
                } catch (Throwable throwable) {
                    failure[0] = throwable;
                }
            }
        });
        if (!completed || failure[0] != null) {
            interstitials.remove(handle);
            sendBridgeError(
                    MSG_INTERSTITIAL,
                    EVENT_AD_LOAD_FAILED,
                    0,
                    ERROR_CREATE_FAILED,
                    completed
                            ? throwableMessage(failure[0])
                            : "Interrupted while creating the interstitial.",
                    "create_interstitial");
            return 0;
        }
        return handle;
    }

    public void destroyInterstitial(int handle) {
        final InterstitialRecord record = interstitials.remove(handle);
        if (record == null) {
            return;
        }
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (record.ad != null) {
                    record.ad.setListener(null);
                    record.ad = null;
                }
            }
        });
    }

    public void loadInterstitial(final int handle) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                InterstitialRecord record = interstitials.get(handle);
                if (record == null || record.ad == null) {
                    sendInvalidHandle(MSG_INTERSTITIAL, EVENT_AD_LOAD_FAILED,
                            handle, "load_interstitial");
                    return;
                }
                record.ad.loadAd();
            }
        });
    }

    public boolean isInterstitialReady(int handle) {
        InterstitialRecord record = interstitials.get(handle);
        return record != null && record.ad != null && record.ad.isAdReady();
    }

    public void showInterstitial(final int handle, final String placementName) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                InterstitialRecord record = interstitials.get(handle);
                if (record == null || record.ad == null) {
                    sendInvalidHandle(MSG_INTERSTITIAL, EVENT_AD_DISPLAY_FAILED,
                            handle, "show_interstitial");
                    return;
                }
                if (placementName == null || placementName.length() == 0) {
                    record.ad.showAd(activity);
                } else {
                    record.ad.showAd(activity, placementName);
                }
            }
        });
    }

    public boolean isInterstitialPlacementCapped(String placementName) {
        return initialized && placementName != null
                && LevelPlayInterstitialAd.isPlacementCapped(placementName);
    }

    // ---------------------------------------------------------------------
    // Rewarded ad units

    public int createRewarded(final String adUnitId, final double bidFloor) {
        if (!canCreateAd(MSG_REWARDED, adUnitId, "create_rewarded")) {
            return 0;
        }

        final int handle = allocateHandle();
        final RewardedRecord record = new RewardedRecord();
        final Throwable[] failure = new Throwable[1];
        boolean completed = runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    LevelPlayRewardedAd ad;
                    if (bidFloor >= 0.0) {
                        LevelPlayRewardedAd.Config config =
                                new LevelPlayRewardedAd.Config.Builder()
                                        .setBidFloor(bidFloor)
                                        .build();
                        ad = new LevelPlayRewardedAd(adUnitId, config);
                    } else {
                        ad = new LevelPlayRewardedAd(adUnitId);
                    }
                    ad.setListener(new RewardedListener(handle));
                    record.ad = ad;
                    rewardedAds.put(handle, record);
                } catch (Throwable throwable) {
                    failure[0] = throwable;
                }
            }
        });
        if (!completed || failure[0] != null) {
            rewardedAds.remove(handle);
            sendBridgeError(
                    MSG_REWARDED,
                    EVENT_AD_LOAD_FAILED,
                    0,
                    ERROR_CREATE_FAILED,
                    completed
                            ? throwableMessage(failure[0])
                            : "Interrupted while creating the rewarded ad.",
                    "create_rewarded");
            return 0;
        }
        return handle;
    }

    public void destroyRewarded(int handle) {
        final RewardedRecord record = rewardedAds.remove(handle);
        if (record == null) {
            return;
        }
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (record.ad != null) {
                    record.ad.setListener(null);
                    record.ad = null;
                }
            }
        });
    }

    public void loadRewarded(final int handle) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                RewardedRecord record = rewardedAds.get(handle);
                if (record == null || record.ad == null) {
                    sendInvalidHandle(MSG_REWARDED, EVENT_AD_LOAD_FAILED,
                            handle, "load_rewarded");
                    return;
                }
                record.ad.loadAd();
            }
        });
    }

    public boolean isRewardedReady(int handle) {
        RewardedRecord record = rewardedAds.get(handle);
        return record != null && record.ad != null && record.ad.isAdReady();
    }

    public void showRewarded(final int handle, final String placementName) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                RewardedRecord record = rewardedAds.get(handle);
                if (record == null || record.ad == null) {
                    sendInvalidHandle(MSG_REWARDED, EVENT_AD_DISPLAY_FAILED,
                            handle, "show_rewarded");
                    return;
                }
                if (placementName == null || placementName.length() == 0) {
                    record.ad.showAd(activity);
                } else {
                    record.ad.showAd(activity, placementName);
                }
            }
        });
    }

    public boolean isRewardedPlacementCapped(String placementName) {
        return initialized && placementName != null
                && LevelPlayRewardedAd.isPlacementCapped(placementName);
    }

    public String getReward(int handle, String placementName) {
        RewardedRecord record = rewardedAds.get(handle);
        if (record == null || record.ad == null) {
            return null;
        }
        LevelPlayReward reward =
                placementName == null || placementName.length() == 0
                        ? record.ad.getReward()
                        : record.ad.getReward(placementName);
        return reward == null ? null : rewardToJson(reward).toString();
    }

    // ---------------------------------------------------------------------
    // Banner ad units

    public int createBanner(
            final String adUnitId,
            final int size,
            final int position,
            final String placementName,
            final double bidFloor,
            final boolean respectSafeArea) {
        if (!canCreateAd(MSG_BANNER, adUnitId, "create_banner")) {
            return 0;
        }
        if (position != BANNER_POSITION_TOP && position != BANNER_POSITION_BOTTOM) {
            sendBridgeError(MSG_BANNER, EVENT_AD_LOAD_FAILED, 0, ERROR_INVALID_ARGUMENT,
                    "Banner position must be top (1) or bottom (2).", "create_banner");
            return 0;
        }

        final int handle = allocateHandle();
        final BannerRecord record = new BannerRecord(position, respectSafeArea);
        final Throwable[] failure = new Throwable[1];
        boolean completed = runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    LevelPlayAdSize adSize = resolveBannerSize(size);
                    LevelPlayBannerAdView.Config.Builder builder =
                            new LevelPlayBannerAdView.Config.Builder().setAdSize(adSize);
                    if (placementName != null && placementName.length() > 0) {
                        builder.setPlacementName(placementName);
                    }
                    if (bidFloor >= 0.0) {
                        builder.setBidFloor(bidFloor);
                    }

                    LevelPlayBannerAdView adView =
                            new LevelPlayBannerAdView(activity, adUnitId, builder.build());
                    adView.setBannerListener(new BannerListener(handle));
                    // Native LevelPlay banner loading is also the display
                    // path. Keep the attached view visible by default; callers
                    // can hide it after loading and show it again later.
                    adView.setVisibility(View.VISIBLE);
                    record.adView = adView;
                    banners.put(handle, record);
                    attachBanner(record);
                } catch (Throwable throwable) {
                    banners.remove(handle);
                    if (record.adView != null) {
                        record.adView.setBannerListener(null);
                        record.adView.destroy();
                        ViewGroup parent = (ViewGroup) record.adView.getParent();
                        if (parent != null) {
                            parent.removeView(record.adView);
                        }
                        record.adView = null;
                    }
                    failure[0] = throwable;
                }
            }
        });
        if (!completed || failure[0] != null) {
            banners.remove(handle);
            sendBridgeError(
                    MSG_BANNER,
                    EVENT_AD_LOAD_FAILED,
                    0,
                    ERROR_CREATE_FAILED,
                    completed
                            ? throwableMessage(failure[0])
                            : "Interrupted while creating the banner.",
                    "create_banner");
            return 0;
        }
        return handle;
    }

    public void loadBanner(final int handle) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                BannerRecord record = banners.get(handle);
                if (record == null || record.adView == null) {
                    sendInvalidHandle(MSG_BANNER, EVENT_AD_LOAD_FAILED,
                            handle, "load_banner");
                    return;
                }
                record.adView.loadAd();
            }
        });
    }

    public void showBanner(final int handle) {
        setBannerVisibility(handle, View.VISIBLE, "show_banner");
    }

    public void hideBanner(final int handle) {
        setBannerVisibility(handle, View.GONE, "hide_banner");
    }

    public void pauseBanner(final int handle) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                BannerRecord record = banners.get(handle);
                if (record == null || record.adView == null) {
                    sendInvalidHandle(MSG_BANNER, EVENT_AD_DISPLAY_FAILED,
                            handle, "pause_banner");
                    return;
                }
                record.adView.pauseAutoRefresh();
            }
        });
    }

    public void resumeBanner(final int handle) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                BannerRecord record = banners.get(handle);
                if (record == null || record.adView == null) {
                    sendInvalidHandle(MSG_BANNER, EVENT_AD_DISPLAY_FAILED,
                            handle, "resume_banner");
                    return;
                }
                record.adView.resumeAutoRefresh();
            }
        });
    }

    public void destroyBanner(final int handle) {
        final BannerRecord record = banners.remove(handle);
        if (record == null) {
            return;
        }
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (record.adView == null) {
                    return;
                }
                record.adView.setBannerListener(null);
                record.adView.destroy();
                ViewGroup parent = (ViewGroup) record.adView.getParent();
                if (parent != null) {
                    parent.removeView(record.adView);
                }
                record.adView = null;
            }
        });
    }

    // ---------------------------------------------------------------------
    // SDK listeners

    private final class InterstitialListener implements LevelPlayInterstitialAdListener {
        private final int handle;

        InterstitialListener(int handle) {
            this.handle = handle;
        }

        private boolean active() {
            return interstitials.containsKey(handle);
        }

        @Override
        public void onAdLoaded(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_INTERSTITIAL, EVENT_AD_LOADED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdLoadFailed(LevelPlayAdError error) {
            if (active()) {
                send(MSG_INTERSTITIAL, EVENT_AD_LOAD_FAILED, handle, errorToJson(error));
            }
        }

        @Override
        public void onAdDisplayed(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_INTERSTITIAL, EVENT_AD_DISPLAYED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdDisplayFailed(LevelPlayAdError error, LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_INTERSTITIAL, EVENT_AD_DISPLAY_FAILED, handle,
                        errorAndAdInfoToJson(error, adInfo));
            }
        }

        @Override
        public void onAdClicked(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_INTERSTITIAL, EVENT_AD_CLICKED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdClosed(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_INTERSTITIAL, EVENT_AD_CLOSED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdInfoChanged(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_INTERSTITIAL, EVENT_AD_INFO_CHANGED, handle, adInfoToJson(adInfo));
            }
        }
    }

    private final class RewardedListener implements LevelPlayRewardedAdListener {
        private final int handle;

        RewardedListener(int handle) {
            this.handle = handle;
        }

        private boolean active() {
            return rewardedAds.containsKey(handle);
        }

        @Override
        public void onAdLoaded(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_REWARDED, EVENT_AD_LOADED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdLoadFailed(LevelPlayAdError error) {
            if (active()) {
                send(MSG_REWARDED, EVENT_AD_LOAD_FAILED, handle, errorToJson(error));
            }
        }

        @Override
        public void onAdDisplayed(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_REWARDED, EVENT_AD_DISPLAYED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdDisplayFailed(LevelPlayAdError error, LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_REWARDED, EVENT_AD_DISPLAY_FAILED, handle,
                        errorAndAdInfoToJson(error, adInfo));
            }
        }

        @Override
        public void onAdClicked(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_REWARDED, EVENT_AD_CLICKED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdClosed(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_REWARDED, EVENT_AD_CLOSED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdInfoChanged(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_REWARDED, EVENT_AD_INFO_CHANGED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdRewarded(LevelPlayReward reward, LevelPlayAdInfo adInfo) {
            if (!active()) {
                return;
            }
            JSONObject json = adInfoToJson(adInfo);
            try {
                putNullable(json, "reward_name", reward == null ? null : reward.getName());
                json.put("reward_amount", reward == null ? 0 : reward.getAmount());
                send(MSG_REWARDED, EVENT_AD_REWARDED, handle, json);
            } catch (JSONException exception) {
                sendJsonError(MSG_REWARDED, handle, exception);
            }
        }
    }

    private final class BannerListener implements LevelPlayBannerAdViewListener {
        private final int handle;

        BannerListener(int handle) {
            this.handle = handle;
        }

        private boolean active() {
            return banners.containsKey(handle);
        }

        @Override
        public void onAdLoaded(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_LOADED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdLoadFailed(LevelPlayAdError error) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_LOAD_FAILED, handle, errorToJson(error));
            }
        }

        @Override
        public void onAdDisplayed(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_DISPLAYED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdDisplayFailed(LevelPlayAdInfo adInfo, LevelPlayAdError error) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_DISPLAY_FAILED, handle,
                        errorAndAdInfoToJson(error, adInfo));
            }
        }

        @Override
        public void onAdClicked(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_CLICKED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdExpanded(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_EXPANDED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdCollapsed(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_COLLAPSED, handle, adInfoToJson(adInfo));
            }
        }

        @Override
        public void onAdLeftApplication(LevelPlayAdInfo adInfo) {
            if (active()) {
                send(MSG_BANNER, EVENT_AD_LEFT_APPLICATION, handle, adInfoToJson(adInfo));
            }
        }
    }

    // ---------------------------------------------------------------------
    // Helpers

    private boolean configureAps() {
        if (!apsEnabled || apsConfigured) {
            return true;
        }
        if (apsAppId == null || apsAppId.trim().length() == 0) {
            sendBridgeError(
                    MSG_INIT,
                    EVENT_INIT_FAILED,
                    0,
                    ERROR_NETWORK_CONFIGURATION,
                    "aps_android_app_id is required when aps_android is enabled.",
                    "configure_aps");
            return false;
        }

        final Throwable[] failure = new Throwable[1];
        boolean completed = runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                try {
                    Class<?> registration =
                            Class.forName("com.amazon.device.ads.AdRegistration");
                    Method getInstance = registration.getMethod(
                            "getInstance", String.class, android.content.Context.class);
                    getInstance.invoke(
                            null, apsAppId, activity.getApplicationContext());
                } catch (Throwable throwable) {
                    failure[0] = throwable;
                }
            }
        });
        if (!completed || failure[0] != null) {
            sendBridgeError(
                    MSG_INIT,
                    EVENT_INIT_FAILED,
                    0,
                    ERROR_NETWORK_CONFIGURATION,
                    completed
                            ? throwableMessage(failure[0])
                            : "Interrupted while configuring Amazon Publisher Services.",
                    "configure_aps");
            return false;
        }
        apsConfigured = true;
        return true;
    }

    private boolean runOnUiThreadBlocking(final Runnable runnable) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            runnable.run();
            return true;
        }

        final CountDownLatch completed = new CountDownLatch(1);
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    runnable.run();
                } finally {
                    completed.countDown();
                }
            }
        });
        try {
            completed.await();
            return true;
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            return false;
        }
    }

    private boolean canCreateAd(int messageId, String adUnitId, String operation) {
        if (destroyed) {
            return false;
        }
        if (!initialized) {
            sendBridgeError(messageId, EVENT_AD_LOAD_FAILED, 0, ERROR_NOT_INITIALIZED,
                    "Create ad units only after EVENT_INIT_SUCCEEDED.", operation);
            return false;
        }
        if (adUnitId == null || adUnitId.trim().length() == 0) {
            sendBridgeError(messageId, EVENT_AD_LOAD_FAILED, 0, ERROR_INVALID_ARGUMENT,
                    "The LevelPlay ad unit ID must not be empty.", operation);
            return false;
        }
        return true;
    }

    private int allocateHandle() {
        while (true) {
            int handle = nextHandle.getAndIncrement();
            if (handle > 0
                    && !interstitials.containsKey(handle)
                    && !rewardedAds.containsKey(handle)
                    && !banners.containsKey(handle)) {
                return handle;
            }
            if (handle <= 0) {
                nextHandle.compareAndSet(handle + 1, 1);
            }
        }
    }

    private LevelPlayAdSize resolveBannerSize(int size) {
        switch (size) {
            case BANNER_SIZE_LARGE:
                return LevelPlayAdSize.LARGE;
            case BANNER_SIZE_MEDIUM_RECTANGLE:
                return LevelPlayAdSize.MEDIUM_RECTANGLE;
            case BANNER_SIZE_LEADERBOARD:
                return LevelPlayAdSize.LEADERBOARD;
            case BANNER_SIZE_ADAPTIVE:
                LevelPlayAdSize adaptive = LevelPlayAdSize.createAdaptiveAdSize(activity);
                return adaptive == null ? LevelPlayAdSize.BANNER : adaptive;
            case BANNER_SIZE_BANNER:
            default:
                return LevelPlayAdSize.BANNER;
        }
    }

    private void attachBanner(final BannerRecord record) {
        final LevelPlayBannerAdView adView = record.adView;
        if (adView == null) {
            return;
        }

        int gravity =
                Gravity.CENTER_HORIZONTAL
                        | (record.position == BANNER_POSITION_TOP ? Gravity.TOP : Gravity.BOTTOM);
        FrameLayout.LayoutParams layoutParams =
                new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        gravity);
        activity.addContentView(adView, layoutParams);

        if (record.respectSafeArea && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            adView.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
                @Override
                public WindowInsets onApplyWindowInsets(View view, WindowInsets insets) {
                    ViewGroup.LayoutParams rawLayoutParams = view.getLayoutParams();
                    if (rawLayoutParams instanceof FrameLayout.LayoutParams) {
                        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) rawLayoutParams;
                        params.leftMargin = insets.getSystemWindowInsetLeft();
                        params.topMargin = insets.getSystemWindowInsetTop();
                        params.rightMargin = insets.getSystemWindowInsetRight();
                        params.bottomMargin = insets.getSystemWindowInsetBottom();
                        view.setLayoutParams(params);
                    }
                    return insets;
                }
            });
            adView.requestApplyInsets();
        }
    }

    private void setBannerVisibility(final int handle, final int visibility, final String operation) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                BannerRecord record = banners.get(handle);
                if (record == null || record.adView == null) {
                    sendInvalidHandle(MSG_BANNER, EVENT_AD_DISPLAY_FAILED,
                            handle, operation);
                    return;
                }
                record.adView.setVisibility(visibility);
            }
        });
    }

    private void sendInvalidHandle(
            int messageId, int event, int handle, String operation) {
        if (destroyed) {
            return;
        }
        sendBridgeError(messageId, event, handle, ERROR_INVALID_HANDLE,
                "Unknown or destroyed LevelPlay ad handle.", operation);
    }

    private static JSONObject adInfoToJson(LevelPlayAdInfo adInfo) {
        JSONObject json = new JSONObject();
        if (adInfo == null) {
            return json;
        }
        try {
            putNullable(json, "ad_id", adInfo.getAdId());
            putNullable(json, "ad_unit_id", adInfo.getAdUnitId());
            putNullable(json, "ad_unit_name", adInfo.getAdUnitName());
            putNullable(json, "ad_format", adInfo.getAdFormat());
            putNullable(json, "placement_name", adInfo.getPlacementName());
            putNullable(json, "auction_id", adInfo.getAuctionId());
            putNullable(json, "country", adInfo.getCountry());
            putNullable(json, "ab", adInfo.getAb());
            putNullable(json, "segment_name", adInfo.getSegmentName());
            putNullable(json, "ad_network", adInfo.getAdNetwork());
            putNullable(json, "instance_name", adInfo.getInstanceName());
            putNullable(json, "instance_id", adInfo.getInstanceId());
            json.put("revenue", adInfo.getRevenue());
            putNullable(json, "precision", adInfo.getPrecision());
            putNullable(json, "encrypted_cpm", adInfo.getEncryptedCPM());
            putNullable(json, "creative_id", adInfo.getCreativeId());

            LevelPlayAdSize adSize = adInfo.getAdSize();
            if (adSize != null) {
                json.put("ad_width", adSize.getWidth());
                json.put("ad_height", adSize.getHeight());
                putNullable(json, "ad_size_description", adSize.getDescription());
                json.put("ad_size_is_adaptive", adSize.isAdaptive());
            }
        } catch (JSONException exception) {
            return jsonError(exception);
        }
        return json;
    }

    private static JSONObject errorToJson(LevelPlayAdError error) {
        JSONObject json = new JSONObject();
        if (error == null) {
            return json;
        }
        try {
            json.put("error_code", error.getErrorCode());
            putNullable(json, "error_message", error.getErrorMessage());
            putNullable(json, "ad_id", error.getAdId());
            putNullable(json, "ad_unit_id", error.getAdUnitId());
        } catch (JSONException exception) {
            return jsonError(exception);
        }
        return json;
    }

    private static JSONObject errorToJson(LevelPlayInitError error) {
        JSONObject json = new JSONObject();
        if (error == null) {
            return json;
        }
        try {
            json.put("error_code", error.getErrorCode());
            putNullable(json, "error_message", error.getErrorMessage());
        } catch (JSONException exception) {
            return jsonError(exception);
        }
        return json;
    }

    private static JSONObject errorAndAdInfoToJson(
            LevelPlayAdError error, LevelPlayAdInfo adInfo) {
        JSONObject json = adInfoToJson(adInfo);
        JSONObject errorJson = errorToJson(error);
        try {
            json.put("error_code", errorJson.optInt("error_code"));
            putNullable(json, "error_message", errorJson.optString("error_message", null));
            if (!json.has("ad_id")) {
                putNullable(json, "ad_id", errorJson.optString("ad_id", null));
            }
            if (!json.has("ad_unit_id")) {
                putNullable(json, "ad_unit_id", errorJson.optString("ad_unit_id", null));
            }
        } catch (JSONException exception) {
            return jsonError(exception);
        }
        return json;
    }

    private static JSONObject rewardToJson(LevelPlayReward reward) {
        JSONObject json = new JSONObject();
        if (reward == null) {
            return json;
        }
        try {
            putNullable(json, "name", reward.getName());
            json.put("amount", reward.getAmount());
        } catch (JSONException exception) {
            return jsonError(exception);
        }
        return json;
    }

    private static JSONObject jsonError(Throwable throwable) {
        JSONObject json = new JSONObject();
        try {
            json.put("_json_error", true);
            json.put("error", throwableMessage(throwable));
        } catch (JSONException ignored) {
            // JSONObject accepts strings, so this is only a defensive fallback.
        }
        return json;
    }

    private static void putNullable(JSONObject json, String key, Object value)
            throws JSONException {
        json.put(key, value == null ? JSONObject.NULL : value);
    }

    private static String throwableMessage(Throwable throwable) {
        if (throwable == null) {
            return "Unknown LevelPlay bridge error.";
        }
        String message = throwable.getLocalizedMessage();
        return message == null || message.length() == 0
                ? throwable.getClass().getSimpleName()
                : message;
    }

    private static void send(
            int messageId, int event, int handle, JSONObject json) {
        if (json != null && json.optBoolean("_json_error", false)) {
            json.remove("_json_error");
            event = EVENT_JSON_ERROR;
        }
        callback(messageId, event, handle, json == null ? "{}" : json.toString());
    }

    private static void sendJsonError(int messageId, int handle, Throwable throwable) {
        send(messageId, EVENT_JSON_ERROR, handle, jsonError(throwable));
    }

    private static void sendBridgeError(
            int messageId,
            int event,
            int handle,
            int errorCode,
            String errorMessage,
            String operation) {
        try {
            JSONObject json = new JSONObject();
            json.put("error_code", errorCode);
            putNullable(json, "error_message", errorMessage);
            putNullable(json, "operation", operation);
            send(messageId, event, handle, json);
        } catch (JSONException exception) {
            sendJsonError(messageId, handle, exception);
        }
    }

    private static final class InterstitialRecord {
        volatile LevelPlayInterstitialAd ad;
    }

    private static final class RewardedRecord {
        volatile LevelPlayRewardedAd ad;
    }

    private static final class BannerRecord {
        final int position;
        final boolean respectSafeArea;
        volatile LevelPlayBannerAdView adView;

        BannerRecord(int position, boolean respectSafeArea) {
            this.position = position;
            this.respectSafeArea = respectSafeArea;
        }
    }
}
