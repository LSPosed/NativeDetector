#include <cmath>
#include "bitmap.h"
#include "enc_str.h"

jobject asBitmap(JNIEnv *env, float width, jstring label) {
    jclass classTextPaint = env->FindClass("android/text/TextPaint"_ienc .c_str());
    jmethodID initTextPaint = env->GetMethodID(classTextPaint, "<init>"_ienc .c_str(), "()V"_ienc .c_str());
    jmethodID setColor = env->GetMethodID(classTextPaint, "setColor"_ienc .c_str(), "(I)V"_ienc .c_str());
    jmethodID setAlpha = env->GetMethodID(classTextPaint, "setAlpha"_ienc .c_str(), "(I)V"_ienc .c_str());
    jmethodID setTextSize = env->GetMethodID(classTextPaint, "setTextSize"_ienc .c_str(), "(F)V"_ienc .c_str());
    jmethodID measureText = env->GetMethodID(classTextPaint, "measureText"_ienc .c_str(),
                                             "(Ljava/lang/String;)F"_ienc .c_str());
    jmethodID setAntiAlias = env->GetMethodID(classTextPaint, "setAntiAlias"_ienc .c_str(), "(Z)V"_ienc .c_str());
    jmethodID descentMethod = env->GetMethodID(classTextPaint, "descent"_ienc .c_str(), "()F"_ienc .c_str());
    jmethodID ascentMethod = env->GetMethodID(classTextPaint, "ascent"_ienc .c_str(), "()F"_ienc .c_str());

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

    jclass classBitmap = env->FindClass("android/graphics/Bitmap"_ienc .c_str());
    jmethodID createBitmap = env->GetStaticMethodID(classBitmap, "createBitmap"_ienc .c_str(),
                                                    "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;"_ienc .c_str());

    jclass classBitmap$Config = env->FindClass("android/graphics/Bitmap$Config"_ienc .c_str());
    jfieldID field = env->GetStaticFieldID(classBitmap$Config, "ARGB_8888"_ienc .c_str(),
                                           "Landroid/graphics/Bitmap$Config;"_ienc .c_str());
    jobject config = env->GetStaticObjectField(classBitmap$Config, field);
    jobject bitmap = env->CallStaticObjectMethod(classBitmap, createBitmap, bitmapWidth,
                                                 bitmapHeight, config);

    jclass classCanvas = env->FindClass("android/graphics/Canvas"_ienc .c_str());
    jmethodID initCanvas = env->GetMethodID(classCanvas, "<init>"_ienc .c_str(), "(Landroid/graphics/Bitmap;)V"_ienc .c_str());
    jmethodID drawPaint = env->GetMethodID(classCanvas, "drawPaint"_ienc .c_str(), "(Landroid/graphics/Paint;)V"_ienc .c_str());
    jmethodID drawText = env->GetMethodID(classCanvas, "drawText"_ienc .c_str(),
                                          "(Ljava/lang/String;FFLandroid/graphics/Paint;)V"_ienc .c_str());

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
