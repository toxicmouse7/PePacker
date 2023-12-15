//
// Created by Aleksej on 08.12.2023.
//

#ifndef PEFILE32_HPP
#define PEFILE32_HPP
#include <vector>
#include <string>
#include <Windows.h>


class PeFile32
{
private:
    char* _peContent = nullptr;
    size_t _peSize = 0;
    PIMAGE_DOS_HEADER _dosHeader = nullptr;
    PIMAGE_NT_HEADERS32 _ntHeaders = nullptr;
    std::vector<PIMAGE_SECTION_HEADER> _sectionHeaders;

    static unsigned long AlignBy(unsigned long number, unsigned long align);

    [[nodiscard]] unsigned long RvaToOffset(unsigned long rva) const;

    void AddDataToDecoder(std::vector<char>& decoderCode, _IMAGE_SECTION_HEADER* const& firstSection);

public:
    explicit PeFile32(const std::string& peFilePath);

    ~PeFile32();

    PIMAGE_SECTION_HEADER AddNewSection(std::vector<char>& decoderCode);


    void Encode(std::vector<char>& decoderCode);
};


#endif //PEFILE32_HPP
