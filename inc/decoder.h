#pragma once
#include <exception>
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
        void write_channels_to_files(const std::string& basename);

    private:
        AVFormatContext* m_context;
        AVCodecContext* m_codec_context;
        AVCodec* m_codec;
        std::vector<std::vector<uint8_t>> m_channel_data;
};
