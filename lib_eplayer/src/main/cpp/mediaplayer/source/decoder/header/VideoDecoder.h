
#ifndef EPLAYER_VIDEODECODER_H
#define EPLAYER_VIDEODECODER_H


#include "MediaDecoder.h"
#include "PlayerState.h"
#include "MediaClock.h"

class VideoDecoder : public MediaDecoder {
public:
    VideoDecoder(AVFormatContext *pFormatCtx, AVCodecContext *avctx,
                 AVStream *stream, int streamIndex, PlayerState *playerState);

    virtual ~VideoDecoder();

    void setMasterClock(MediaClock *masterClock);

    //override保留字表示当前函数重写了基类的虚函数
    void start() override;

    void stop() override;

    void flush() override;

    int getFrameSize();

    int getRotate();

    FrameQueue *getFrameQueue();

    AVFormatContext *getFormatContext();

    void run() override;

private:
    // 解码视频帧
    int decodeVideo();

private:
    AVFormatContext *pFormatCtx;    // 解复用上下文
    FrameQueue *frameQueue;         // 帧队列
    int mRotate;                    // 旋转角度

    bool mExit;                     // 退出标志
    Thread *decodeThread;           // 解码线程
    MediaClock *masterClock;        // 主时钟
};

#endif //EPLAYER_VIDEODECODER_H
