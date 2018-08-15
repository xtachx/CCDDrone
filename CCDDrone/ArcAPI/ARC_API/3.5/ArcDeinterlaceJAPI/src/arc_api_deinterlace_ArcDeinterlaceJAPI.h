/* DO NOT EDIT THIS FILE - it is machine generated */
#ifdef __APPLE__
	#include <JavaVM/jni.h>
#else
	#include <jni.h>
#endif

/* Header for class arc_api_deinterlace_ArcDeinterlaceJAPI */

#ifndef _Included_arc_api_deinterlace_ArcDeinterlaceJAPI
#define _Included_arc_api_deinterlace_ArcDeinterlaceJAPI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    RunAlg
 * Signature: (JIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_RunAlg__JIII
  (JNIEnv *, jclass, jlong, jint, jint, jint);

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    RunAlg
 * Signature: (JIIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_RunAlg__JIIII
  (JNIEnv *, jclass, jlong, jint, jint, jint, jint);

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    RunAlg
 * Signature: (JIIIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_RunAlg__JIIIII
  (JNIEnv *, jclass, jlong, jint, jint, jint, jint, jint);

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    GetCustomAlgorithms
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_GetCustomAlgorithms
  (JNIEnv *, jclass, jstring);

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    GetAPIConstants
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_GetAPIConstants
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif