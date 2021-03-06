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

    //if (!(m_codec = avcodec_find_encoder(source_codec_context->codec_id))) {
    /*
     * Some codecs are not available (WMA) so just use AAC.
     */
    if (!(m_codec = avcodec_find_encoder(AV_CODEC_ID_AAC))) {
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
    m_codec_context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

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
void Encoder::write_frame(AVFrame* frame) {
    /* packet used for temporary storage */
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;
    int error;

    auto cleanup = [&]() {
        av_packet_unref(&packet);
    };

    auto throw_if_real_error = [&]() {
        if ((error != AVERROR(EAGAIN)) && (error != AVERROR_EOF)) {
            cleanup();
            throw ASException("Could not encode", error);
        }
    };

    /* send frame to encoder */
    error = avcodec_send_frame(m_codec_context, frame);
    if (error < 0) {
        throw_if_real_error();
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
    }
    cleanup();
}

Encoder::~Encoder() {
    avcodec_close(m_codec_context);
    avio_close(m_context->pb);
    avformat_free_context(m_context);
}

void Encoder::write_trailer() {
    int error;
    if ((error = av_write_trailer(m_context)) < 0) {
        /*
         * The trailer is not written in the destructor in the event that an
         * error occurs, as most errors are handled by throwing exceptions.
         */
        throw ASException("Could not write output file trailer", error);
    }
}

const AVCodecContext* Encoder::codec_context() {
    return m_codec_context;
}
