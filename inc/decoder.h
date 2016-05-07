#pragma once
#include <exception>
#include <string>

extern "C" {
#include "libavformat/avformat.h"
}

class DecoderException: public std::exception {
    public:
        DecoderException(std::string what_arg) : m_arg(what_arg) { }
        ~DecoderException() throw() { }
        const char* what() const throw() { return m_arg.c_str(); };
    private:
        std::string m_arg;
};

class Decoder {
    public:
        Decoder(const char* filename);
        ~Decoder();
        unsigned int channels() const;

    private:
        AVFormatContext* m_context;
        AVCodecContext* m_codec_context;
        AVCodec* m_codec;
        char m_error_buffer[255];
};
