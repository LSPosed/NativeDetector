#include <cmath>
#include "bitmap.h"
#include "enc_str.h"
#include "logging.h"

jobject asBitmap(JNIEnv *env, float width, std::string_view str) {
    jclass classTextPaint = env->FindClass("android/text/TextPaint"_ienc .c_str());
    jmethodID initTextPaint = env->GetMethodID(classTextPaint, "<init>"_ienc .c_str(), "()V"_ienc .c_str());
    jmethodID setColor = env->GetMethodID(classTextPaint, "setColor"_ienc .c_str(), "(I)V"_ienc .c_str());
    jmethodID setAlpha = env->GetMethodID(classTextPaint, "setAlpha"_ienc .c_str(), "(I)V"_ienc .c_str());
    jmethodID setTextSize = env->GetMethodID(classTextPaint, "setTextSize"_ienc .c_str(), "(F)V"_ienc .c_str());
    jmethodID setAntiAlias = env->GetMethodID(classTextPaint, "setAntiAlias"_ienc .c_str(), "(Z)V"_ienc .c_str());
    jclass classBitmap = env->FindClass("android/graphics/Bitmap"_ienc .c_str());
    jmethodID createBitmap = env->GetStaticMethodID(classBitmap, "createBitmap"_ienc .c_str(), "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;"_ienc .c_str());
    jclass classBitmap$Config = env->FindClass("android/graphics/Bitmap$Config"_ienc .c_str());
    jfieldID field = env->GetStaticFieldID(classBitmap$Config, "ARGB_8888"_ienc .c_str(), "Landroid/graphics/Bitmap$Config;"_ienc .c_str());
    jobject config = env->GetStaticObjectField(classBitmap$Config, field);
    jclass classCanvas = env->FindClass("android/graphics/Canvas"_ienc .c_str());
    jmethodID initCanvas = env->GetMethodID(classCanvas, "<init>"_ienc .c_str(), "(Landroid/graphics/Bitmap;)V"_ienc .c_str());
    jmethodID drawARGB = env->GetMethodID(classCanvas, "drawARGB"_ienc .c_str(), "(IIII)V"_ienc .c_str());
    jclass classLayoutBuilder = env->FindClass("android/text/StaticLayout$Builder"_ienc .c_str());
    jmethodID buildLayout = env->GetMethodID(classLayoutBuilder, "build"_ienc .c_str(), "()Landroid/text/StaticLayout;"_ienc .c_str());
    jclass classLayout = env->FindClass("android/text/StaticLayout"_ienc .c_str());
    jmethodID getHeight = env->GetMethodID(classLayout, "getHeight"_ienc .c_str(), "()I"_ienc .c_str());
    jmethodID draw = env->GetMethodID(classLayout, "draw"_ienc .c_str(), "(Landroid/graphics/Canvas;)V"_ienc .c_str());
    jmethodID obtainLayout = env->GetStaticMethodID(classLayoutBuilder, "obtain"_ienc. c_str(), "(Ljava/lang/CharSequence;IILandroid/text/TextPaint;I)Landroid/text/StaticLayout$Builder;"_ienc .c_str());

    float textSize = 42.0f;
    jobject textPaint = env->NewObject(classTextPaint, initTextPaint);
    env->CallVoidMethod(textPaint, setColor, 0xFFFFFFFF);
    env->CallVoidMethod(textPaint, setAntiAlias, true);
    env->CallVoidMethod(textPaint, setAlpha, 222);
    env->CallVoidMethod(textPaint, setTextSize, textSize);
    jobject builder = env->CallStaticObjectMethod(classLayoutBuilder, obtainLayout, env->NewStringUTF(str.data()), 0, str.size(), textPaint, (jint)width);
    jobject layout = env->CallObjectMethod(builder, buildLayout);
    jint height = env->CallIntMethod(layout, getHeight);

    jobject bitmap = env->CallStaticObjectMethod(classBitmap, createBitmap, (jint)width, height, config);
    jobject canvas = env->NewObject(classCanvas, initCanvas, bitmap);

    env->CallVoidMethod(canvas, drawARGB, 0xff, 0x30, 0x30, 0x30);
    env->CallVoidMethod(layout, draw, canvas);
    env->DeleteLocalRef(canvas);
    env->DeleteLocalRef(classCanvas);
    env->DeleteLocalRef(config);
    env->DeleteLocalRef(classBitmap$Config);
    env->DeleteLocalRef(classBitmap);
    env->DeleteLocalRef(textPaint);
    env->DeleteLocalRef(classTextPaint);

    return bitmap;
}
