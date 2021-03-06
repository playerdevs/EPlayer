

#ifndef EPLAYER_AUDIODECODER_H
#define EPLAYER_AUDIODECODER_H
#include "MediaDecoder.h"
#include "PlayerState.h"

class AudioDecoder : public MediaDecoder {
public:
    AudioDecoder(AVCodecContext *avctx, AVStream *stream, int streamIndex, PlayerState *playerState);

    virtual ~AudioDecoder();

    int getAudioFrame(AVFrame *frame);

private:
    bool packetPending; // 一次解码无法全部消耗完AVPacket中的数据的标志
    AVPacket *packet;
    int64_t next_pts;
    AVRational next_pts_tb;
};
#endif //EPLAYER_AUDIODECODER_H
