#include "encoder.h"
#include "asexception.h"
#include <fstream>

/* refer to ffmpeg/doc/examples/transcode_aac.c for encode procedure */

Encoder::Encoder(const std::string& filename, const AVCodecContext* source_codec_context):
    m_context(nullptr), m_codec_context(nullptr), m_codec(nullptr) {
    int error;
    AVIOContext* output_io_context;
    if ((error = avio_open(&output_io_context, filename.c_str(), AVIO_FLAG_WRITE)) < 0) {
        throw ASException(std::string("Error opening file: ") + filename, error);
    }

    if (!(m_context = avformat_alloc_context())) {
        avformat_free_context(m_context);
        throw ASException("Could not allocate format context");
    }

    /* Associate the output file (pointer) with the container format context. */
    m_context->pb = output_io_context;

    auto cleanup = [&]() {
        avio_close(m_context->pb);
        avformat_free_context(m_context);
    };

    if (!(m_context->oformat = av_guess_format(nullptr, filename.c_str(), nullptr))) {
        cleanup();
        throw ASException("Could not find output file format");
    }

    strlcpy(m_context->filename, filename.c_str(), sizeof(m_context->filename));

    if (!(m_codec = avcodec_find_encoder(source_codec_context->codec_id))) {
        cleanup();
        throw ASException("Could not find audio codec");
    }

    AVStream* stream = avformat_new_stream(m_context, m_codec);
    if (!stream) {
        cleanup();
        throw ASException("Could not create new audio stream");
    }

    /* set encoder parameters  */
    m_codec_context = stream->codec;
    m_codec_context->channels = 1;
    m_codec_context->channel_layout = av_get_default_channel_layout(1);
    m_codec_context->sample_rate = source_codec_context->sample_rate;
    m_codec_context->sample_fmt = m_codec->sample_fmts[0];
    m_codec_context->bit_rate = source_codec_context->bit_rate;

    /* set stream sample rate */
    stream->time_base.den = source_codec_context->sample_rate;
    stream->time_base.num = 1;

    /*
     * Some container formats (like MP4) require global headers to be present
     * Mark the encoder so that it behaves accordingly.
     */
    if (m_context->oformat->flags & AVFMT_GLOBALHEADER) {
        m_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    /* open encoder for later use */
    if ((error = avcodec_open2(m_codec_context, m_codec, nullptr)) < 0) {
        cleanup();
        throw ASException("Could not open output codec", error);
    }

    if ((error = avformat_write_header(m_context, nullptr)) < 0) {
        throw ASException("Could not write output file header", error);
    }
}

void Encoder::write_encode_audio_frames(std::vector<uint8_t>* channel_data) {
    AVPacket packet; /* packet used for temporary storage */
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;
    int error;
    int frame_size = FFMIN(channel_data->size(), m_codec_context->frame_size);
    AVFrame* frame = alloc_frame(frame_size);

    while (channel_data->size() > 0) {
        /* determine frame size */
        int new_size = FFMIN(channel_data->size(), m_codec_context->frame_size);
        if (new_size != frame_size) {
            av_frame_free(&frame);
            alloc_frame(new_size);
            frame_size = new_size;
        }

        /* move channel data to frame */
        std::copy(channel_data->data(), channel_data->data() + frame_size, frame->data[0]);
        channel_data->erase(channel_data->begin(), channel_data->begin() + frame_size);

        auto cleanup = [&]() {
            av_packet_unref(&packet);
            av_frame_free(&frame);
        };

        auto throw_if_real_error = [&]() {
            if ((error != AVERROR(EAGAIN)) && (error != AVERROR_EOF)) {
                cleanup();
                throw ASException("", error);
            }
        };

        /* send frame to encoder */
        error = avcodec_send_frame(m_codec_context, frame);
        if (error < 0) {
            throw_if_real_error();
            break;
        }

        /* read all packets in frame */
        while ((error = avcodec_receive_packet(m_codec_context, &packet)) >= 0) {
            if ((error = av_write_frame(m_context, &packet)) < 0) {
                cleanup();
                throw ASException("Could not write frame", error);
            }
        }
        if (error < 0) {
            throw_if_real_error();
            continue;
        }
    }
    av_frame_free(&frame);
    av_packet_unref(&packet);

    if ((error = av_write_trailer(m_context)) < 0) {
        throw ASException("Could not write output file trailer", error);
    }
}

AVFrame* Encoder::alloc_frame(int frame_size) const {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        throw ASException("Could not allocate frame");
    }

    frame->nb_samples = frame_size;
    frame->channel_layout = m_codec_context->channel_layout;
    frame->format = m_codec_context->sample_fmt;
    frame->sample_rate = m_codec_context->sample_rate;

    int error;
    if ((error = av_frame_get_buffer(frame, 0)) < 0) {
        av_frame_free(&frame);
        throw ASException("Could allocate output frame samples", error);
    }

    return frame;
}

Encoder::~Encoder() {
    avcodec_close(m_codec_context);
    avio_close(m_context->pb);
    avformat_free_context(m_context);
}
