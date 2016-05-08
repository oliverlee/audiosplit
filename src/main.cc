#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
}

#include "decoder.h"
#include "asexception.h"


int main(int argc, char** argv) {
    /*
     * register codecs and formats and other lavf/lavc components
     * TODO: move to respective classes
     */
    avcodec_register_all();
    av_register_all();
    avfilter_register_all();
    av_log_set_level(AV_LOG_FATAL);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>\n";
        std::cout << "Split audio channels in input file in to separate files.\n\n"
            << "Output files are encoded as single channel (mono) .aac files.\n"
            << "Other audio file properties are retained from the input file.\n"
            << "Output files are written to the same directory as the input file\n"
            << "and have the abbreviated channel inserted into the filename.\n\n";
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    try {
        Decoder decoder(filename);
        std::cout << "Creating " << decoder.channels()
            << " output files for '" << filename << "'\n";
        decoder.decode_audio_frames();
    } catch (const ASException& e) {
        std::cout << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
