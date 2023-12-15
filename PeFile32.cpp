//
// Created by Aleksej on 08.12.2023.
//

#include "PeFile32.hpp"
#include <fstream>

unsigned long PeFile32::AlignBy(const unsigned long number, const unsigned long align)
{
    return number + align - 1 & ~(align - 1);
}

unsigned long PeFile32::RvaToOffset(const unsigned long rva) const
{
    for (const auto& sectionHeader: _sectionHeaders)
    {
        const auto sectionStart = sectionHeader->VirtualAddress;

        if (const auto sectionEnd = sectionStart + sectionHeader->SizeOfRawData;
            rva >= sectionStart && rva < sectionEnd)
        {
            const auto offsetInSection = rva - sectionStart + sectionHeader->PointerToRawData;
            return offsetInSection;
        }
    }

    return 0;
}

PeFile32::PeFile32(const std::string& peFilePath)
{
    std::ifstream peFile(peFilePath, std::ios::binary);

    peFile.seekg(0, std::ios::end);
    _peSize = peFile.tellg();
    peFile.seekg(0, std::ios::beg);

    _peContent = new char[_peSize];

    peFile.read(_peContent, static_cast<long long>(_peSize));
    peFile.close();

    _dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(_peContent);
    _ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS32>(reinterpret_cast<byte*>(_dosHeader) + _dosHeader->e_lfanew);

    auto* sectionHeaders = IMAGE_FIRST_SECTION(_ntHeaders);
    for (auto i = 0; i < _ntHeaders->FileHeader.NumberOfSections; ++i)
        _sectionHeaders.push_back(&sectionHeaders[i]);
}

PeFile32::~PeFile32()
{
    delete[] _peContent;
}

PIMAGE_SECTION_HEADER PeFile32::AddNewSection(std::vector<char>& decoderCode)
{
    const auto newSectionHeader = &IMAGE_FIRST_SECTION(_ntHeaders)[_ntHeaders->FileHeader.NumberOfSections];
    strcpy_s(reinterpret_cast<char*>(newSectionHeader->Name), sizeof(newSectionHeader->Name), ".crypto");
    const auto& lastSection = _sectionHeaders.back();
    const auto sectionAlignment = _ntHeaders->OptionalHeader.SectionAlignment;
    const auto fileAlignment = _ntHeaders->OptionalHeader.FileAlignment;

    newSectionHeader->VirtualAddress = AlignBy(lastSection->VirtualAddress
                                               + lastSection->Misc.VirtualSize,
                                               _ntHeaders->OptionalHeader.SectionAlignment);
    newSectionHeader->Misc.VirtualSize = decoderCode.size();
    newSectionHeader->Characteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE;
    newSectionHeader->VirtualAddress = lastSection->VirtualAddress + AlignBy(
                                           lastSection->Misc.VirtualSize, sectionAlignment);
    newSectionHeader->SizeOfRawData = AlignBy(decoderCode.size(), fileAlignment);
    newSectionHeader->PointerToRawData = lastSection->PointerToRawData + lastSection->SizeOfRawData;

    _ntHeaders->FileHeader.NumberOfSections += 1;
    _ntHeaders->OptionalHeader.SizeOfImage = AlignBy(
        newSectionHeader->VirtualAddress + newSectionHeader->Misc.VirtualSize, sectionAlignment);

    _sectionHeaders.push_back(newSectionHeader);
    return newSectionHeader;
}

void PeFile32::AddDataToDecoder(std::vector<char>& decoderCode, _IMAGE_SECTION_HEADER* const& firstSection)
{
    decoderCode[decoderCode.size() - 12] = static_cast<char>(firstSection->VirtualAddress & 0x000000FF);
    decoderCode[decoderCode.size() - 11] = static_cast<char>((firstSection->VirtualAddress & 0x0000FF00) >> 8);
    decoderCode[decoderCode.size() - 10] = static_cast<char>((firstSection->VirtualAddress & 0x00FF0000) >> 16);
    decoderCode[decoderCode.size() - 9] = static_cast<char>((firstSection->VirtualAddress & 0xFF000000) >> 24);

    decoderCode[decoderCode.size() - 8] = static_cast<char>(firstSection->Misc.VirtualSize & 0x000000FF);
    decoderCode[decoderCode.size() - 7] = static_cast<char>((firstSection->Misc.VirtualSize & 0x0000FF00) >> 8);
    decoderCode[decoderCode.size() - 6] = static_cast<char>((firstSection->Misc.VirtualSize & 0x00FF0000) >> 16);
    decoderCode[decoderCode.size() - 5] = static_cast<char>((firstSection->Misc.VirtualSize & 0xFF000000) >> 24);

    decoderCode[decoderCode.size() - 4] = static_cast<char>(_ntHeaders->OptionalHeader.AddressOfEntryPoint & 0x000000FF);
    decoderCode[decoderCode.size() - 3] = static_cast<char>((_ntHeaders->OptionalHeader.AddressOfEntryPoint & 0x0000FF00) >> 8);
    decoderCode[decoderCode.size() - 2] = static_cast<char>((_ntHeaders->OptionalHeader.AddressOfEntryPoint & 0x00FF0000) >> 16);
    decoderCode[decoderCode.size() - 1] = static_cast<char>((_ntHeaders->OptionalHeader.AddressOfEntryPoint & 0xFF000000) >> 24);
}

void PeFile32::Encode(std::vector<char>& decoderCode)
{
    const auto* newSectionHeader = AddNewSection(decoderCode);
    const auto& firstSection = _sectionHeaders.front();
    firstSection->Characteristics |= IMAGE_SCN_MEM_WRITE;

    auto relocsDataDirectory = _ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    auto relocs = reinterpret_cast<PIMAGE_BASE_RELOCATION>(_ntHeaders +
        RvaToOffset(_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress));

    AddDataToDecoder(decoderCode, firstSection);

    const auto entryOffset = RvaToOffset(firstSection->VirtualAddress);
    for (auto i = 0; i < firstSection->Misc.VirtualSize; ++i)
    {
        *(_peContent + entryOffset + i) = static_cast<char>(*(_peContent + entryOffset + i) ^ 228);
    }

    _ntHeaders->OptionalHeader.AddressOfEntryPoint = newSectionHeader->VirtualAddress;

    std::ofstream outputFile("../decoded.exe", std::ios::binary | std::ios::out);
    outputFile.write(_peContent, static_cast<long long>(_peSize));
    outputFile.write(decoderCode.data(), static_cast<long long>(decoderCode.size()));
    for (auto i = 0; i < newSectionHeader->SizeOfRawData - decoderCode.size(); ++i)
        outputFile << '\0';
    outputFile.close();
}
