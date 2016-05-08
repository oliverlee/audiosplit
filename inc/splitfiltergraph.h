#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

extern "C" {
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
//#include "libavfilter/channelsplit.h"
}

#include "asexception.h"


class SplitFilterGraph {
    public:
        SplitFilterGraph(const AVCodecContext& source_codec_context,
                const AVCodecContext& sink_codec_context):
            m_abuffersrc(avfilter_get_by_name("abuffer")),
            m_abuffersink(avfilter_get_by_name("abuffersink")),
            m_aformat(avfilter_get_by_name("aformat")),
            m_asetnsamples(avfilter_get_by_name("asetnsamples")),
            m_afchannelsplit(avfilter_get_by_name("channelsplit")),
            m_format_contexts(source_codec_context.channels, nullptr),
            m_setn_contexts(source_codec_context.channels, nullptr),
            m_sink_contexts(source_codec_context.channels, nullptr) {
            if (!(m_graph = avfilter_graph_alloc())) {
                throw ASException("Could not allocate filter graph");
            }

            int error;
            std::stringstream ss;
            char channel_layout_name[100];
            av_get_channel_layout_string(channel_layout_name, 100,
                    source_codec_context.channels, source_codec_context.channel_layout);
            /* create abuffer filter */
            ss << "time_base=" << 1 << "/" << source_codec_context.sample_rate
                << ":sample_rate=" << source_codec_context.sample_rate
                << ":sample_fmt=" << av_get_sample_fmt_name(source_codec_context.sample_fmt)
                << ":channel_layout=" << channel_layout_name;
            //std::cout << "abuffer: " << ss.str().c_str() << std::endl;
            if ((error = avfilter_graph_create_filter(
                            &m_src_context, m_abuffersrc,
                            nullptr, ss.str().c_str(), nullptr, m_graph)) < 0) {
                avfilter_graph_free(&m_graph);
                throw ASException("Could not create abuffer filter", error);
            }

            ss.str("");
            /* create channelsplit filter */
            ss << "channel_layout=" << channel_layout_name;
            //std::cout << "channelsplit: " << ss.str().c_str() << std::endl;
            if ((error = avfilter_graph_create_filter(
                            &m_afchannelsplit_context, m_afchannelsplit,
                            nullptr, ss.str().c_str(), nullptr, m_graph)) < 0) {
                avfilter_graph_free(&m_graph);
                throw ASException("Could not create channelsplit filter", error);
            }

            ss.str("");
            av_get_channel_layout_string(channel_layout_name, 100,
                    sink_codec_context.channels, sink_codec_context.channel_layout);
            /* create aformat filters */
            ss << "sample_fmts=" << av_get_sample_fmt_name(sink_codec_context.sample_fmt)
                << ":sample_rates=" << sink_codec_context.sample_rate
                << ":channel_layouts=" << channel_layout_name;
            //std::cout << "aformat: " << ss.str().c_str() << std::endl;
            for (auto&& i: m_format_contexts) {
                if ((error = avfilter_graph_create_filter(&i, m_aformat,
                                nullptr, ss.str().c_str(), nullptr, m_graph)) < 0) {
                    avfilter_graph_free(&m_graph);
                    throw ASException("Could not create aformat filter", error);
                }
            }

            ss.str("");
            /* create asetnsamples filters */
            ss << "nb_out_samples=" << sink_codec_context.frame_size
                << ":pad=" << 0;
            //std::cout << "asetnsamples: " << ss.str().c_str() << std::endl;
            for (auto&& i: m_setn_contexts) {
                if ((error = avfilter_graph_create_filter(&i, m_asetnsamples,
                                nullptr, ss.str().c_str(), nullptr, m_graph)) < 0) {
                    avfilter_graph_free(&m_graph);
                    throw ASException("Could not create aformat filter", error);
                }
            }

            /* create abuffersink filters */
            //std::cout << "abuffersink" << std::endl;
            for (auto&& i: m_sink_contexts) {
                if ((error = avfilter_graph_create_filter(&i, m_abuffersink,
                                nullptr, nullptr, nullptr, m_graph)) < 0) {
                    avfilter_graph_free(&m_graph);
                    throw ASException("Could not create abuffersink filter", error);
                }
            }

            /* connect filters, each output of channelsplit -> format -> setnsamples -> sink */
            if ((error = avfilter_link(m_src_context, 0,
                            m_afchannelsplit_context, 0)) < 0) {
                throw ASException("Could not link filters", error);
            }
            for (int i = 0; i < m_sink_contexts.size(); ++i) {
                if ((error = avfilter_link(m_afchannelsplit_context, i,
                                m_format_contexts[i], 0)) < 0) {
                    throw ASException("Could not link filters", error);
                }
                if ((error = avfilter_link(m_format_contexts[i], 0,
                                m_setn_contexts[i], 0)) < 0) {
                    throw ASException("Could not link filters", error);
                }
                if ((error = avfilter_link(m_setn_contexts[i], 0,
                                m_sink_contexts[i], 0)) < 0) {
                    throw ASException("Could not link filters", error);
                }
            }

            /* configure filter graph */
            if ((error = avfilter_graph_config(m_graph, nullptr)) < 0) {
                throw ASException("Could not configure filter graph", error);
            }
        }

        ~SplitFilterGraph() {
            avfilter_graph_free(&m_graph); /* this also frees all graph filters */
        }

        int send_frame(AVFrame* frame) {
            return av_buffersrc_add_frame(m_src_context, frame);
        }

        int receive_sink_frame(AVFrame* frame, int sink) {
            av_frame_unref(frame);
            return av_buffersink_get_frame(m_sink_contexts[sink], frame);
        }

    private:
        AVFilter* m_abuffersrc;
        AVFilter* m_abuffersink;
        AVFilter* m_aformat;
        AVFilter* m_asetnsamples;
        AVFilter* m_afchannelsplit;
        AVFilterGraph* m_graph;
        AVFilterContext* m_src_context;
        std::vector<AVFilterContext*> m_format_contexts;
        std::vector<AVFilterContext*> m_setn_contexts;
        std::vector<AVFilterContext*> m_sink_contexts;
        AVFilterContext* m_afchannelsplit_context;
};
