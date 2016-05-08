#pragma once
#include <exception>
#include <string>
#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}


class DecoderException: public std::exception {
    public:
        DecoderException(const std::string& what_arg) {
            m_arg = what_arg + "\n";
        }
        DecoderException(const std::string& what_arg, int error_code) {
            av_strerror(error_code, m_error_buffer, sizeof(m_error_buffer));
            m_arg = what_arg + " (error '" + m_error_buffer + "')\n";
        }
        ~DecoderException() throw() { }
        const char* what() const throw() { return m_arg.c_str(); };
    private:
        std::string m_arg;
        char m_error_buffer[255]; /* use same value as in transcode_aac example */
};


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
