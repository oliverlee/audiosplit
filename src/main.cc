#include <iostream>

extern "C" {
//#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

#include "decoder.h"


int main(int argc, char** argv) {

    /* register codecs and formats and other lavf/lavc components*/
    //avcodec_register_all()
    av_register_all();

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>\n";
        std::cout << "Print the number of audio channels in input file.\n\n";
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    try {
        Decoder decoder(filename);
        std::cout << decoder.channels() << " channels in '" << filename << "'\n";
    } catch (const DecoderException& e) {
        std::cout << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
