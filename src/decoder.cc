#include "decoder.h"
#include "encoder.h"
#include "asexception.h"
#include "splitfiltergraph.h"
#include <fstream>
#include <iostream>

Decoder::Decoder(const std::string& filename):
    m_context(nullptr), m_codec_context(nullptr), m_codec(nullptr),
    m_filename(filename) {
    int error;
    if ((error = avformat_open_input(&m_context, filename.c_str(), nullptr, nullptr)) < 0) {
        throw ASException(std::string("Error opening file: ") + filename, error);
    }
    if ((error = avformat_find_stream_info(m_context, nullptr)) < 0) {
        avformat_close_input(&m_context);
        throw ASException("Could not open find stream info", error);
    }
    for (int i = 0; i < m_context->nb_streams; ++i) {
        /*
         * libavformat/internal.h not installed to include dir so
         * streams[i]->internal->codex is not accessible
         */
        m_codec_context = m_context->streams[i]->codec;
        if (m_codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {
            break; /* Use first detected audio type stream */
        }
        m_codec_context = nullptr;
    }
    if (m_codec_context == nullptr) {
        avformat_close_input(&m_context);
        throw ASException("Could not find audio stream");
    }
    if (!(m_codec = avcodec_find_decoder(m_codec_context->codec_id))) {
        avformat_close_input(&m_context);
        throw ASException("Could not find audio codec");
    }
    if ((error = avcodec_open2(m_codec_context, m_codec, nullptr)) < 0) {
        avformat_close_input(&m_context);
        throw ASException("Could not open input codec", error);
    }
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
        throw ASException("Decoding for non-planar audio formats not yet implemented");
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        throw ASException("Could not allocate audio frame");
    }

    AVPacket packet; /* packet used for temporary storage */
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;
    int error = 0;

    std::vector<Encoder*> encoders(channels(), nullptr);

    std::string::size_type index = m_filename.rfind('.');
    std::string basename = m_filename.substr(0, index + 1);

    for (int i = 0; i < channels(); ++i) {
        const char* channel_name = av_get_channel_name(
                av_channel_layout_extract_channel(m_codec_context->channel_layout, i));
        std::string filename = basename + channel_name + ".aac";
        std::cout << "creating file: " << filename << std::endl;
        encoders[i] = new Encoder(basename + channel_name + ".aac", m_codec_context);
    }
    SplitFilterGraph splitter(*m_codec_context, *encoders[0]->codec_context());

    auto cleanup = [&]() {
        for (auto i: encoders) {
            delete i;
        }
        av_packet_unref(&packet);
        av_frame_free(&frame);
    };

    auto throw_if_real_error = [&]() {
        if ((error != AVERROR(EAGAIN)) && (error != AVERROR_EOF)) {
            cleanup();
            throw ASException("Could not decode", error);
        }
    };


    while (1) {
        /* get packet from format context */
        error = av_read_frame(m_context, &packet);
        if (error < 0) {
            throw_if_real_error();
            break;
        }

        /* send packet to decoder */
        error = avcodec_send_packet(m_codec_context, &packet);
        if (error < 0) {
            throw_if_real_error();
            continue;
        }

        /* read all frames in packet */
        while ((error = avcodec_receive_frame(m_codec_context, frame)) >= 0) {
            if ((error = splitter.send_frame(frame)) < 0) {
                throw ASException("Could not send frame to splitter", error);
            }
            // TODO: use multiple threads/queues for reading/splitting/writing frames
            for (int i = 0; i < channels(); ++i) {
                while ((error = splitter.receive_sink_frame(frame, i)) >= 0) {
                    encoders[i]->write_frame(frame);
                }
                throw_if_real_error();
            }
        }
        if (error < 0) {
            throw_if_real_error();
            continue;
        }
    }

    for (auto i: encoders) {
        i->write_trailer();
    }
    cleanup();
    return;
}
