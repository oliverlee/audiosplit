#include "decoder.h"

Decoder::Decoder(const char* filename): m_context(nullptr), m_codec_context(nullptr), m_codec(nullptr) {
    int error;
    if ((error = avformat_open_input(&m_context, filename, nullptr, nullptr)) < 0) {
        av_strerror(error, m_error_buffer, sizeof(m_error_buffer));
        throw DecoderException(
                std::string("Error opening file: ") + filename + " (error '" + m_error_buffer + "')\n");
    }
    if ((error = avformat_find_stream_info(m_context, nullptr)) < 0) {
        avformat_close_input(&m_context);
        av_strerror(error, m_error_buffer, sizeof(m_error_buffer));
        throw DecoderException(
                std::string("Could not open find stream info (error '") + m_error_buffer + "')\n");
    }
    for (int i = 0; i < m_context->nb_streams; ++i) {
        m_codec_context = m_context->streams[i]->codec;
        if (m_codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {
            break; /* Use first detected audio type stream */
        }
        m_codec_context = nullptr;
    }
    if (m_codec_context == nullptr) {
        avformat_close_input(&m_context);
        throw DecoderException("Could not find audio stream\n");
    }
    if (!(m_codec = avcodec_find_decoder((m_codec_context->codec_id)))) {
        avformat_close_input(&m_context);
        throw DecoderException("Could not find audio codec\n");
    }
    if ((error = avcodec_open2(m_codec_context, m_codec, nullptr)) < 0) {
        avformat_close_input(&m_context);
        throw DecoderException(std::string("Could not open input codec (error '") + m_error_buffer + "')\n");
    }
}

Decoder::~Decoder() {
    avformat_close_input(&m_context);
}

unsigned int Decoder::channels() const {
    return m_codec_context->channels;
}
