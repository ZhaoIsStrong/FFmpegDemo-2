#include <jni.h>
#include <string>

extern "C"
{
# include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
JNIEXPORT jstring JNICALL
Java_com_pf_ffmpegdemo_MainActivity_stringFromJNI(JNIEnv *env, jobject jobj) {
    std::string hello = "Hello from C++";
    avcodec_register_all();
    av_register_all();
    return env->NewStringUTF(hello.c_str());
}
}