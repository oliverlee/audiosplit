#include <iostream>

extern "C" {
//#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

unsigned int count_audio_channels(AVFormatContext* context) {
    for (int i = 0; i < context->nb_streams; ++i) {
        AVCodecContext* codec = context->streams[i]->codec;
        if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* Return channel count for first audio type stream */
            std::cout << "channels:";
            for (int j = 0; j < codec->channels; ++j) {
                std::cout << " "
                    << av_get_channel_name(av_channel_layout_extract_channel(codec->channel_layout, j));
            }
            std::cout << "\n";
            return codec->channels;
        }
    }
    return 0;
}

int main(int argc, char** argv) {

    /* register codecs and formats and other lavf/lavc components*/
    //avcodec_register_all();
    av_register_all();

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>\n";
        std::cout << "Print the number of audio channels in input file.\n\n";
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    AVFormatContext* context;
    if (avformat_open_input(&context, filename, nullptr, nullptr) < 0) {
        std::cout << "Error opening file: " << filename << "\n";
        return EXIT_FAILURE;
    }

    //av_log_set_level(AV_LOG_FATAL);
    if (avformat_find_stream_info(context, nullptr) < 0) {
        std::cout << "Error reading stream information\n";
        avformat_close_input(&context);
        return EXIT_FAILURE;
    }
    //av_dump_format(context, 0, filename, 0);

    std::cout << count_audio_channels(context) << " channels in '" << filename << "'\n";
    avformat_close_input(&context);

    return EXIT_SUCCESS;
}
