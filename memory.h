#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>


struct Memory
{
    std::vector<uint8_t> bytes;


    // 加载文件
    bool loadFile(const std::string& filename, size_t addr = 0)
    {
        std::ifstream file(filename, std::ios::binary);

        if (!file)
        {
            std::cerr << "open file failed\n";
            return false;
        }


        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);


        // 自动扩容
        if (addr + fileSize > bytes.size())
        {
            bytes.resize(addr + fileSize, 0);
        }


        file.read(
            reinterpret_cast<char*>(bytes.data() + addr),
            fileSize
        );


        return true;
    }



    // 写8位
    void write8(size_t addr, uint8_t value)
    {
        // 自动扩容
        if (addr >= bytes.size())
        {
            bytes.resize(addr + 1, 0);
        }

        bytes[addr] = value;
    }



    // 读8位
    uint8_t read8(size_t addr)
    {
        if (addr >= bytes.size())
        {
            bytes.resize(addr + 1, 0);
        }

        return bytes[addr];
    }



    // 写32位 (小端)
    void write32(size_t addr, uint32_t value)
    {
        write8(addr + 0, value & 0xff);
        write8(addr + 1, (value >> 8) & 0xff);
        write8(addr + 2, (value >> 16) & 0xff);
        write8(addr + 3, (value >> 24) & 0xff);
    }



    // 读32位 (小端)
    uint32_t read32(size_t addr)
    {
        uint32_t value = 0;


        value |= read8(addr + 0);
        value |= read8(addr + 1) << 8;
        value |= read8(addr + 2) << 16;
        value |= read8(addr + 3) << 24;


        return value;
    }
    // 写64位
    void write64(size_t addr, uint64_t value)
    {
        write8(addr + 0, value & 0xff);
        write8(addr + 1, (value >> 8) & 0xff);
        write8(addr + 2, (value >> 16) & 0xff);
        write8(addr + 3, (value >> 24) & 0xff);
        write8(addr + 4, (value >> 32) & 0xff);
        write8(addr + 5, (value >> 40) & 0xff);
        write8(addr + 6, (value >> 48) & 0xff);
        write8(addr + 7, (value >> 56) & 0xff);
    }


    // 读64位
    uint64_t read64(size_t addr)
    {
        uint64_t value = 0;

        value |= (uint64_t)read8(addr + 0);
        value |= (uint64_t)read8(addr + 1) << 8;
        value |= (uint64_t)read8(addr + 2) << 16;
        value |= (uint64_t)read8(addr + 3) << 24;
        value |= (uint64_t)read8(addr + 4) << 32;
        value |= (uint64_t)read8(addr + 5) << 40;
        value |= (uint64_t)read8(addr + 6) << 48;
        value |= (uint64_t)read8(addr + 7) << 56;

        return value;
    }

    size_t size() const
    {
        return bytes.size();
    }
};