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

#include <android/native_window_jni.h>
#include <unistd.h>
}

#define LOG_TAG "pf"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C"
{
/**
 * 生成yuv文件
 */
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

/**
 * 原生播放
 */
JNIEXPORT void JNICALL
Java_com_pf_ffmpegdemo_playnativevideo_VideoView_render(JNIEnv *env, jobject instance,
                                                        jstring input_, jobject surface) {
    const char *input = env->GetStringUTFChars(input_, false);

    //注册组件
    av_register_all();
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (avformat_open_input(&pFormatContext, input, NULL, NULL) < 0) {
        LOGE("打开输入视频文件失败");
        return;
    }
    //获取视频信息
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        LOGE("获取视频信息失败");
        return;
    }
    //找到视频流下标
    int vedio_stream_idx = -1;
    int i;
    for (i = 0; i < pFormatContext->nb_streams; ++i) {
        if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vedio_stream_idx = i;
            LOGE("找到视频id:%d", pFormatContext->streams[i]->codec->codec_type);
            break;
        }
    }
    //获取视频编解码器上下文
    AVCodecContext *pCodecContext = pFormatContext->streams[i]->codec;
    //获取视频编码
    AVCodec *pCodec = avcodec_find_decoder(pCodecContext->codec_id);
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        LOGE("解码失败");
        return;
    }
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //av_init_packet(packet);
    //像素数据
    AVFrame *frame = av_frame_alloc();
    //RGB
    AVFrame *rgb_frame = av_frame_alloc();
    //给缓冲区分配内存
    //只有指定了AVFrame的像素格式、画面大小才能真正的分配内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_RGBA, pCodecContext->width,
                               pCodecContext->height));
    LOGE("宽:%d,  高:%d  ", pCodecContext->width, pCodecContext->height);
    //设置yuvFrame的缓冲区，像素格式
    int re = avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA,
                            pCodecContext->width, pCodecContext->height);
    LOGE("申请内存:%d", re);
    //输出需要改变
    int length = 0;
    int got_frame;
    //输出文件
    int frameCount = 0;
    SwsContext *swsContext = sws_getContext(pCodecContext->width, pCodecContext->height,
                                            pCodecContext->pix_fmt,
                                            pCodecContext->width, pCodecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL,
                                            NULL, NULL);
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    //视频缓冲区
    ANativeWindow_Buffer outBuffer;
    while (av_read_frame(pFormatContext, packet) >= 0) {
        //视频流
        if (packet->stream_index == vedio_stream_idx) {
            length = avcodec_decode_video2(pCodecContext, frame, &got_frame, packet);
            LOGE("获得长度:%d", length);
            //非零,正在解码
            if (got_frame) {
                //绘制之前，配置一些信息，比如宽高，格式
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodecContext->width,
                                                 pCodecContext->height, WINDOW_FORMAT_RGBA_8888);
                //绘制,先锁定画布
                ANativeWindow_lock(nativeWindow, &outBuffer, NULL);
                LOGE("解码第%d帧", frameCount++);
                //转为指定的yuv420P
                sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                          pCodecContext->height, rgb_frame->data, rgb_frame->linesize);
                //rgb_frame是有画面数据
                uint8_t *dst = (uint8_t *) outBuffer.bits;
                //拿到一行有多少个字节,*4是因为rgba四个数据
                int destStride = outBuffer.width * 4;
                //像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                //实际内存一行数量
                int srcStride = rgb_frame->linesize[0];
                for (i = 0; i < pCodecContext->height; i++) {
                    //memcpy(void *dest, const void *src, size_t n)
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                //这里是16毫秒，因为单位是微秒
                usleep(1000 * 16);
            }
        }
        av_free_packet(packet);
    }
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(pCodecContext);
    avformat_free_context(pFormatContext);
    env->ReleaseStringUTFChars(input_, input);
}
}