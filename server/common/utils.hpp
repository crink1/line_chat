// 实现工具接口
#include <iostream>
#include <string>
#include <atomic>
#include <random>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "logger.hpp"

namespace crin_lc
{
    // 1.生成uuid
    std::string uuid()
    {
        // 12位十六进制随机数+4位十六进制的编号
        std::random_device rd;                                   // 实例化设备随机数对象
        std::mt19937 generator(rd());                            // 以设备随机数对象为种子
        std::uniform_int_distribution<int> distribution(0, 255); // 限定数据范围

        std::stringstream ss;
        for (int i = 0; i < 6; i++)
        {
            if (i == 2)
            {
                ss << "-";
            }
            ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
        }

        ss << "-";
        static std::atomic<short> idx(0);
        short tmp = idx.fetch_add(1);
        ss << std::setw(4) << std::setfill('0') << std::hex << tmp;

        return ss.str();
    }

    std::string vcode()
    {
        std::random_device rd;                                   // 实例化设备随机数对象
        std::mt19937 generator(rd());                            // 以设备随机数对象为种子
        std::uniform_int_distribution<int> distribution(0, 9); // 限定数据范围
        std::stringstream ss;
        for (int i = 0; i < 4; i++)
        {
            ss << distribution(generator);
        }
        return ss.str();
    }

    // 2.生成文件读写操作
    bool readFile(const std::string &filename, std::string &body)
    {
        std::ifstream ifs(filename, std::ios::binary | std::ios::in);
        if (!ifs.is_open())
        {
            LOG_ERROR("打开文件 {} 失败", filename);
            return false;
        }
        ifs.seekg(0, std::ios::end);
        size_t flen = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        body.resize(flen);
        ifs.read(&body[0], flen);
        if (!ifs.good())
        {
            LOG_ERROR("读取文件 {} 失败", filename);
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }


    bool writeFile(const std::string &filename, const std::string &body)
    {
        std::ofstream ofs(filename, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!ofs.is_open())
        {
            LOG_ERROR("打开文件 {} 失败", filename);
            ofs.close();

            return false;
        }
        ofs.write(body.c_str(), body.size());
        if(!ofs.good())
        {
            LOG_ERROR("写入文件 {} 失败", filename);
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }
}
