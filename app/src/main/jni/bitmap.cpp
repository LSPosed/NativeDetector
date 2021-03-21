#include <cmath>
#include "bitmap.h"

jobject asBitmap(JNIEnv *env, float width, jstring label) {
    jclass classTextPaint = env->FindClass("android/text/TextPaint");
    jmethodID initTextPaint = env->GetMethodID(classTextPaint, "<init>", "()V");
    jmethodID setColor = env->GetMethodID(classTextPaint, "setColor", "(I)V");
    jmethodID setAlpha = env->GetMethodID(classTextPaint, "setAlpha", "(I)V");
    jmethodID setTextSize = env->GetMethodID(classTextPaint, "setTextSize", "(F)V");
    jmethodID measureText = env->GetMethodID(classTextPaint, "measureText",
                                             "(Ljava/lang/String;)F");
    jmethodID setAntiAlias = env->GetMethodID(classTextPaint, "setAntiAlias", "(Z)V");
    jmethodID descentMethod = env->GetMethodID(classTextPaint, "descent", "()F");
    jmethodID ascentMethod = env->GetMethodID(classTextPaint, "ascent", "()F");

    float textSize = 42.0f;
    jobject textPaint = env->NewObject(classTextPaint, initTextPaint);
    env->CallVoidMethod(textPaint, setTextSize, textSize);
    float measureWidth = env->CallFloatMethod(textPaint, measureText, label);
    textSize *= (width / measureWidth);
    env->CallVoidMethod(textPaint, setTextSize, textSize);
    measureWidth = env->CallFloatMethod(textPaint, measureText, label);

    auto bitmapWidth = (size_t) ceilf(measureWidth);
    float baseLine = -env->CallFloatMethod(textPaint, ascentMethod);
    float descent = env->CallFloatMethod(textPaint, descentMethod);
    auto bitmapHeight = (size_t) ceilf(baseLine + descent);

    jclass classBitmap = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmap = env->GetStaticMethodID(classBitmap, "createBitmap",
                                                    "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

    jclass classBitmap$Config = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID field = env->GetStaticFieldID(classBitmap$Config, "ARGB_8888",
                                           "Landroid/graphics/Bitmap$Config;");
    jobject config = env->GetStaticObjectField(classBitmap$Config, field);
    jobject bitmap = env->CallStaticObjectMethod(classBitmap, createBitmap, bitmapWidth,
                                                 bitmapHeight, config);

    jclass classCanvas = env->FindClass("android/graphics/Canvas");
    jmethodID initCanvas = env->GetMethodID(classCanvas, "<init>", "(Landroid/graphics/Bitmap;)V");
    jmethodID drawPaint = env->GetMethodID(classCanvas, "drawPaint", "(Landroid/graphics/Paint;)V");
    jmethodID drawText = env->GetMethodID(classCanvas, "drawText",
                                          "(Ljava/lang/String;FFLandroid/graphics/Paint;)V");

    jobject canvas = env->NewObject(classCanvas, initCanvas, bitmap);

    env->CallVoidMethod(textPaint, setColor, 0xFF121212);
    env->CallVoidMethod(canvas, drawPaint, textPaint);

    env->CallVoidMethod(textPaint, setAntiAlias, true);
    env->CallVoidMethod(textPaint, setColor, 0xFFFFFFFF);
    env->CallVoidMethod(textPaint, setAlpha, 222);
    env->CallVoidMethod(canvas, drawText, label, 0.0F, baseLine, textPaint);

    env->DeleteLocalRef(canvas);
    env->DeleteLocalRef(classCanvas);
    env->DeleteLocalRef(config);
    env->DeleteLocalRef(classBitmap$Config);
    env->DeleteLocalRef(classBitmap);
    env->DeleteLocalRef(textPaint);
    env->DeleteLocalRef(classTextPaint);

    return bitmap;
}
