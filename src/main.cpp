#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <climits>
#include "racjin/racjin.hpp"

namespace fs = std::filesystem;


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#include <processenv.h>
#include <shellapi.h>
#include <stringapiset.h>
std::string WstrToUtf8Str(const std::wstring& wstr)
{
    std::string retStr;
    if(!wstr.empty())
    {
        int sizeRequired = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);

        if(sizeRequired > 0)
        {
            std::vector<char> utf8String(sizeRequired);
            int bytesConverted = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                                                     -1, &utf8String[0], utf8String.size(), NULL,
                                                     NULL);
            if(bytesConverted != 0)
            {
                retStr = &utf8String[0];
            }
            else
            {
                std::stringstream err;
                err << __FUNCTION__
                    << " std::string WstrToUtf8Str failed to convert wstring '"
                    << wstr.c_str() << L"'";
                throw std::runtime_error(err.str());
            }
        }
    }
    return retStr;
}
#endif

template <typename T>
T SwapEndian(T u)
{
    static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");

    union
    {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for(size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

struct FileRecord
{
    union
    {
        struct
        {
            uint32_t offset;
            uint32_t compressed_size;
            uint16_t section_count;
            uint16_t is_compressed;
            uint32_t decompressed_size;
        } cfc;

        struct
        {
            uint32_t offset;
            uint32_t decompressed_size;
            uint32_t section_count;
            uint32_t compressed_size;//set to 0 if the file is not compressed
        } cddata;

    };
};

static_assert(sizeof(FileRecord) == 0x10, "sizeof(FileRecord) is not 0x10!");

bool is_cfc = false; //if cfc.dig or cddata.dig
bool is_big_endian = false; //for wii titles

const unsigned int kSectorSize = 0x800; //the size of a disc sector

const unsigned int kMagicBytesPGD = 0x44'47'50'00; //.PGD Protected Game Data

int main(int argc, char** argv)
{
    std::vector<std::string> arguments;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    int n_args = 0;
    auto cmd_args = CommandLineToArgvW(GetCommandLineW(), &n_args);
    for(int i = 0; i < n_args; i++)
    {
        std::wstring string_to_convert = cmd_args[i];
        arguments.push_back(WstrToUtf8Str(string_to_convert));
    }
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#else
    for(int i = 0; i < argc; i++)
    {

        arguments.push_back(argv[i]);
    }
#endif

    fs::path p;

    if(argc > 1)
    {
        p = std::filesystem::u8path(arguments.at(1));
        p = std::filesystem::absolute(p);
    }

    if(p.empty() || !std::filesystem::is_regular_file(p) || !std::filesystem::exists(p))
    {
        std::cout << "error: the path to a cfc.dig or cddata.dig file is invalid! Specify the path and other optional arguments manually or drop the file onto the executable;\n\nfile_path <record_section_end_hex_offset> <is_cfc.dig> <is_big_endian>\n\n";
        std::cout << p.u8string() << std::endl;
        std::cin.get();
        return 1;
    }


    fs::create_directories(p.parent_path() / std::filesystem::u8path("unpacked/0"));
    fs::create_directories(p.parent_path() /  std::filesystem::u8path("unpacked/1"));

    std::ifstream container(p, std::ios::binary); //cfc.dig or cddata.dig

    if(!container.is_open())
    {
        std::cout << "error: failed to open the file: " << p.u8string() << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "the file was opened: " << p.u8string() << "\n" << std::endl;


    {
        FileRecord file_record, empty_record;
        container.read(reinterpret_cast<char*>(&file_record), sizeof(FileRecord));

        //in cfc.dig files the first 0x10 bytes are usually 0
        std::memset(&empty_record, 0, sizeof(FileRecord));

        is_cfc = std::memcmp(&file_record, &empty_record, sizeof(FileRecord)) == 0;

        if(file_record.cfc.offset == kMagicBytesPGD)
        {
            std::cout << "error: the file is encrypted (PGD)" << std::endl;
            std::cin.get();
            return 1;
        }
    }

    uint32_t record_section_start = is_cfc ? 0x10 : 0;
    uint32_t record_section_end;

    //note: in some cases it's difficult to determine the exact size of the section that contains file records and their type

    container.seekg(record_section_start);

    container.read(reinterpret_cast<char*>(&record_section_end), sizeof(record_section_end));

    is_big_endian = (0xFF'FF'00'00 & record_section_end) != 0 ;

    if(is_big_endian) record_section_end = SwapEndian(record_section_end);

    record_section_end *= kSectorSize; //might be wrong in some cases if the first record doesn't point to the first file

    //extra arguments: records section end, is_cfc, is_big_endian

    if(arguments.size() > 2) record_section_end = std::stoul(arguments.at(2), 0, 0x10);
    if(arguments.size() > 3) is_cfc = (bool)std::stoul(arguments.at(3));
    if(arguments.size() > 4) is_big_endian = (bool)std::stoul(arguments.at(4));

    std::cout << "archive type: " << (is_cfc ? "cfc.dig" : "cddata.dig") << "\n";
    std::cout << "record section end: 0x" << std::hex << record_section_end << "\n";
    std::cout << "endianness: " << (is_big_endian ? "big_endian" : "little_endian") << "\n" << std::endl;

    container.seekg(record_section_start);

    std::vector<FileRecord> file_records;

    while(container.tellg() < record_section_end)
    {
        FileRecord file_record;
        container.read(reinterpret_cast<char*>(&file_record), sizeof(FileRecord));

        if(is_cfc)
        {
            if(is_big_endian)
            {
                file_record.cfc.offset = SwapEndian(file_record.cfc.offset);
                file_record.cfc.decompressed_size = SwapEndian(file_record.cfc.decompressed_size);
                file_record.cfc.section_count = SwapEndian(file_record.cfc.section_count);
                file_record.cfc.is_compressed = SwapEndian(file_record.cfc.is_compressed);
                file_record.cfc.compressed_size = SwapEndian(file_record.cfc.compressed_size);
            }

        }
        else
        {
            if(is_big_endian)
            {
                file_record.cddata.offset = SwapEndian(file_record.cddata.offset);
                file_record.cddata.decompressed_size = SwapEndian(file_record.cddata.decompressed_size);
                file_record.cddata.section_count = SwapEndian(file_record.cddata.section_count);
                file_record.cddata.compressed_size = SwapEndian(file_record.cddata.compressed_size);
            }
        }

        file_records.push_back(file_record);
    }


    std::cout << "unpacking...\n" << std::endl;

    unsigned int file_counter = 0;

    for(std::size_t i = 0; i < file_records.size(); i++)
    {
        const FileRecord& file_record = file_records[i];

        uint32_t offset = 0;
        uint32_t compressed_size  = 0;
        uint32_t decompressed_size = 0;
        uint32_t section_count = 0;
        uint32_t is_compressed = 0;

        if(is_cfc)
        {
            offset = file_record.cfc.offset * kSectorSize;
            compressed_size  = file_record.cfc.compressed_size ;
            decompressed_size = file_record.cfc.decompressed_size;
            section_count = file_record.cfc.section_count;
            is_compressed = file_record.cfc.is_compressed;

        }
        else
        {
            offset = file_record.cddata.offset * kSectorSize;
            compressed_size  = file_record.cddata.compressed_size > 0 ? file_record.cddata.compressed_size * kSectorSize : file_record.cddata.decompressed_size * kSectorSize;
            decompressed_size = file_record.cddata.decompressed_size * kSectorSize;
            section_count = file_record.cddata.section_count;
            is_compressed = file_record.cddata.compressed_size > 0;
        }



        std::cout << "file record: " << std::hex << i << "\n";

        if(offset == 0)
        {
            std::cout << "skipped an empty record (placeholder/padding)" << "\n\n";
            continue;
        };

        std::cout << "offset: " << std::hex << offset << "\n";
        std::cout << "compressed size : " << std::hex << compressed_size << "\n";
        std::cout << "section count : " << std::hex << section_count << "\n";
        std::cout << "is compressed: " << std::hex << is_compressed << "\n";
        std::cout << "decompressed size : " << std::hex << decompressed_size << "\n\n";

        if(compressed_size > decompressed_size)
        {
            std::cout << "error: the compressed size > decompressed size (not a file record?)\n" << std::endl;
            break;
        }

        if(is_compressed > 1)
        {
            std::cout << "error: the compressed flag is > 1 (not a file record?)" << std::endl;
            break;
        }

        container.seekg(offset);

        if(container.tellg() == -1)
        {
            std::cout << "error: the file at the offset 0x" << offset << " doesn't exist! \n\n";
            break;
        }

        std::vector<uint8_t> buffer(compressed_size, 0);

        container.read(reinterpret_cast<char*>(buffer.data()), compressed_size);


        std::string output_path;

        std::stringstream stream;
        stream << std::hex << offset;
        std::string name(stream.str());

        output_path = is_compressed ? "unpacked/1/" : "unpacked/0/";
        output_path += std::to_string(i) + "_" + name + ".bin";

        std::ofstream single_file(p.parent_path() / std::filesystem::u8path(output_path), std::ios::binary | std::ios::trunc);

        if(!single_file.is_open())
        {
            std::cout << "error: couldn't create the output file " << std::endl;
            break;
        }

        if(is_compressed)
        {
            std::vector<uint8_t> decompressed_buffer = Decompress(buffer, decompressed_size);

            // std::vector<uint8_t> compressed_buffer = Compress(decompressed_buffer); //note: see encode.cpp to get the same compression for cddata.dig files

            single_file.write(reinterpret_cast<char*>(decompressed_buffer.data()), decompressed_buffer.size());

        }
        else
        {
            single_file.write(reinterpret_cast<char*>(buffer.data()), decompressed_size);
        }

        ++file_counter;

        single_file.close();


    }


    container.close();

    std::cout << "the total number of extracted files: " << std::dec << file_counter << "\n";


    std::cout << "press any key to exit...\n";

    std::cin.get();

    return 0;
}

