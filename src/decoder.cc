#include "decoder.h"
#include <iostream>

Decoder::Decoder(const char* filename):
    m_context(nullptr), m_codec_context(nullptr), m_codec(nullptr) {
    int error;
    if ((error = avformat_open_input(&m_context, filename,
                    nullptr, nullptr)) < 0) {
        throw DecoderException(std::string("Error opening file: ") + filename, error);
    }
    if ((error = avformat_find_stream_info(m_context, nullptr)) < 0) {
        avformat_close_input(&m_context);
        throw DecoderException("Could not open find stream info", error);
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
        throw DecoderException("Could not find audio stream");
    }
    if (!(m_codec = avcodec_find_decoder((m_codec_context->codec_id)))) {
        avformat_close_input(&m_context);
        throw DecoderException("Could not find audio codec");
    }
    if ((error = avcodec_open2(m_codec_context, m_codec, nullptr)) < 0) {
        avformat_close_input(&m_context);
        throw DecoderException("Could not open input codec", error);
    }
    m_channel_data = std::vector<std::vector<uint8_t>>(channels(), std::vector<uint8_t>(channels()));
}

Decoder::~Decoder() {
    avformat_close_input(&m_context);
}

unsigned int Decoder::channels() const {
    return m_codec_context->channels;
}

void Decoder::decode_audio_frames() {
    if (!av_sample_fmt_is_planar(m_codec_context->sample_fmt)) {
        /* TODO: convert frame to planar format */
        throw DecoderException("Decoding for non-planar audio formats not yet implemented");
    }

    /* clear previous channel data if any */
    for (int i = 0; i < channels(); ++i) {
        m_channel_data[i].clear();
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        throw DecoderException("Count not allocate audio frame");
    }

    AVPacket packet; /* packet used for temporary storage */
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;
    int error = 0;

    auto cleanup = [&]() {
        av_packet_unref(&packet);
        av_frame_free(&frame);
    };

    auto throw_if_real_error = [&]() {
        if ((error != AVERROR(EAGAIN)) && (error != AVERROR_EOF)) {
            cleanup();
            throw DecoderException("", error);
        }
    };

    while (1) {
        /* get packet from format context */
        error = av_read_frame(m_context, &packet);
        if (error < 0) {
            throw_if_real_error();
            av_packet_unref(&packet);
            break;
        }

        /* send packet to decoder */
        error = avcodec_send_packet(m_codec_context, &packet);
        if (error < 0) {
            throw_if_real_error();
            av_packet_unref(&packet);
            continue;
        }

        /* read all frames in packet */
        while ((error = avcodec_receive_frame(m_codec_context, frame)) >= 0) {
            for (int i = 0; i < channels(); ++i) {
                m_channel_data[i].insert(m_channel_data[i].end(),
                        frame->extended_data[i],
                        frame->extended_data[i] + frame->linesize[0]);
            }
        }
        if (error < 0) {
            throw_if_real_error();
            av_packet_unref(&packet);
            continue;
        }
    }

    cleanup();
    return;
}
