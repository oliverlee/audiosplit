#pragma once
#include <string>
#include <iostream>
#include <vector>

extern "C" {
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}

#include "asexception.h"


class SplitFilterGraph {
    public:
        /* sink codec context is the same for all output channels */
        SplitFilterGraph(const AVCodecContext& source_codec_context,
                const AVCodecContext& sink_codec_context);
        ~SplitFilterGraph();
        int send_frame(AVFrame* frame);
        int receive_sink_frame(AVFrame* frame, int sink);

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
