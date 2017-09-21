#include <jni.h>
#include <string>
#include <android/log.h>

extern "C"
{
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
}

#define LOG_TAG "pf"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C"
{
JNIEXPORT void JNICALL
Java_com_pf_ffmpegdemo_MainActivity_openFile(JNIEnv *env, jobject instance, jstring inputFileName_,
                                             jstring outFileName_) {
    const char *inputFileName = env->GetStringUTFChars(inputFileName_, 0);
    const char *outFileName = env->GetStringUTFChars(outFileName_, 0);

    //注册各大组件
    av_register_all();
    //拿到上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();
    if (avformat_open_input(&avFormatContext, inputFileName, NULL, NULL) < 0) {
        LOGE("打开失败");
        return;
    }
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }
    //视频流索引
    int vedio_stream_idx = -1;
    int i;
    //这里的++i和i++没区别
    for (i = 0; i < avFormatContext->nb_streams; ++i) {
        LOGE("循环:%d", i);
        //codec:每一个流对应的解码器上下文
        //codec_type:流的类型,这里要找视频流
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vedio_stream_idx = i;
            break;
        }
    }
    //获取解码器上下文
    AVCodecContext *avCodecContext = avFormatContext->streams[i]->codec;
    //拿到解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        LOGE("解码失败");
        return;
    }
    //分配内存
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    //初始化结构体
    av_init_packet(avPacket);
    AVFrame *frame = av_frame_alloc();
    //生命一个yuvframe
    AVFrame *yuvFrame = av_frame_alloc();
    //给yuvframe的缓冲区初始化
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, avCodecContext->width, avCodecContext->height));
    int re = avpicture_fill((AVPicture *) yuvFrame, out_buffer, AV_PIX_FMT_YUV420P,
                            avCodecContext->width, avCodecContext->height);
    LOGE("宽:%d,高:%d", avCodecContext->width, avCodecContext->height);
    //mp4的上下文:avCodecContext->pix_fmt
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_YUV420P, SWS_BILINEAR,
                                            NULL, NULL, NULL);
    int frameCount = 0;
    FILE *fp_yuv = fopen(outFileName, "wb");
    //出参
    int got_frame;
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        //根据frame进行原生绘制
        avcodec_decode_video2(avCodecContext, frame, &got_frame, avPacket);
        //拿到yuv三个通道的数据
        LOGE("解码:%d", frameCount++);
        if (got_frame > 0) {
            sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                      frame->height, yuvFrame->data, yuvFrame->linesize);
            int y_size = avCodecContext->width * avCodecContext->height;
            //y:u:v = 4:1:1
            fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
            fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);
            fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);
        }
        av_free_packet(avPacket);
    }
    fclose(fp_yuv);
    //释放资源
    av_frame_free(&frame);
    av_frame_free(&yuvFrame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);
    env->ReleaseStringUTFChars(inputFileName_, inputFileName);
    env->ReleaseStringUTFChars(outFileName_, outFileName);
}
}