#if defined(DM_PLATFORM_ANDROID)

#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_defold_levelplay_LevelPlayJNI_callback(
    JNIEnv* env,
    jclass cls,
    jint message,
    jint event,
    jint handle,
    jstring json);

#ifdef __cplusplus
}
#endif

#endif
