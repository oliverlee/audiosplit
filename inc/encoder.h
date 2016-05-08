#pragma once
#include <string>
#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}


class Encoder {
    public:
        Encoder(const std::string& filename, const AVCodecContext* source_codec_context);
        ~Encoder();
        void write_frame(AVFrame* frame);
        void write_trailer();
        const AVCodecContext* codec_context();

    private:
        AVFormatContext* m_context;
        AVCodecContext* m_codec_context;
        AVCodec* m_codec;
};
