#pragma once

#include <jni.h>
#include <set>
#include <string_view>

jobject asBitmap(JNIEnv *env, float width, std::string_view str);
