#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
}

int main(int argc, char** argv) {

    /* register codecs and formats and other lavf/lavc components*/
    avcodec_register_all();

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>\n";
        std::cout << "Print the number of channels in input file.\n\n.";
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];

    std::cout << 3 << " channels in " << filename << "\n";

    return EXIT_SUCCESS;
}
