#pragma once
#include <string>
#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}


class Decoder {
    public:
        Decoder(const std::string& filename);
        ~Decoder();
        unsigned int channels() const;
        void decode_audio_frames();

    private:
        AVFormatContext* m_context;
        AVCodecContext* m_codec_context;
        AVCodec* m_codec;
        const std::string m_filename;
};
