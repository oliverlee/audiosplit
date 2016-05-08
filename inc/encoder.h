#pragma once
#include <exception>
#include <string>
#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}


class EncoderException: public std::exception {
    public:
        EncoderException(const std::string& what_arg) {
            m_arg = what_arg + "\n";
        }
        EncoderException(const std::string& what_arg, int error_code) {
            av_strerror(error_code, m_error_buffer, sizeof(m_error_buffer));
            m_arg = what_arg + " (error '" + m_error_buffer + "')\n";
        }
        ~EncoderException() throw() { }
        const char* what() const throw() { return m_arg.c_str(); };
    private:
        std::string m_arg;
        char m_error_buffer[255]; /* use same value as in transcode_aac example */
};


class Encoder {
    public:
        Encoder(const std::string& filename, const AVCodecContext* source_codec_context);
        ~Encoder();
        void write_encode_audio_frames(std::vector<uint8_t>* channel_data);

    private:
        AVFormatContext* m_context;
        AVCodecContext* m_codec_context;
        AVCodec* m_codec;
        std::vector<std::vector<uint8_t>> m_channel_data;
        AVFrame* alloc_frame(int frame_size) const;
};
