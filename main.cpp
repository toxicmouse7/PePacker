#include <fstream>

#include "PeFile32.hpp"

std::vector<char> LoadDecoder(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    std::vector<char> decoder;

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    decoder.resize(size);

    file.read(decoder.data(), size);
    return decoder;
}

int main()
{
    auto decoder = LoadDecoder("../decoder.BIN");
    auto peFile = PeFile32("../TestExecutable.exe");

    peFile.Encode(decoder);

    return 0;
}
