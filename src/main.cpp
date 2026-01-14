#include "io/PlyReader.hpp"
#include "io/PlyWriter.hpp"
#include "utils/Timer.hpp"

int main(int argc, char **argv) {
    TICK(load);
    auto data = PlyReader::readDataFromFile("G:\\code\\cpp\\gaussian-stream\\test.ply");
    data.printElementInfos();
    TOCK(load);
    TICK(save_text);
    PlyWriter::writeDataToFile("G:\\code\\cpp\\gaussian-stream\\out-text.ply", data, PlyFormat::ASCII);
    TOCK(save_text);
    TICK(save_binary);
    PlyWriter::writeDataToFile("G:\\code\\cpp\\gaussian-stream\\out-binary.ply", data, PlyFormat::BINARY_LITTLE_ENDIAN);
    TOCK(save_binary);
    
    return 0;
}
