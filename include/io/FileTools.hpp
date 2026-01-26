#pragma once

#include <filesystem>
#include <regex>
#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>

class FileTools {
public:
    /**
     * @brief 检查文件是否存在
     * @param filename 文件路径
     * @throw std::runtime_error 如果文件不存在
     */
    static void checkFileExists(const std::string& filename) {
        if(!std::filesystem::exists(filename)) {
            // std::cerr << "File not found: " << filename << std::endl;
            SPDLOG_ERROR("File not found: {}", filename);
            throw std::runtime_error("File not found: " + filename);
        }
    }
    /**
     * @brief 检查目录是否存在，如果不存在则创建
     * @param filePath  文件路径
     * @throw std::runtime_error 如果创建目录失败
     */
    static void checkAndCreateDir(const std::string& filePath) {
        namespace fs = std::filesystem;
        // 获取文件的目录路径
        fs::path dirPath = fs::path(filePath).parent_path();

        // 检查目录是否存在，如果不存在则创建
        if (!dirPath.empty() && !fs::exists(dirPath)) {
            if (!fs::create_directories(dirPath)) { // 尝试创建目录
                SPDLOG_ERROR("Failed to create directory: {}", dirPath.string());
                throw std::runtime_error("Failed to create directory: " + dirPath.string());
            }
        }
    }

    /**
     * @brief 在指定目录下查找符合给定正则表达式的所有文件路径
     * @param directory 需要查找的目录
     * @param pattern 文件名的正则表达式
     * @return 符合条件的文件路径列表
     */
    static std::vector<std::filesystem::path> findFilesMatchingPattern(const std::string& directory, const std::string& pattern) {
        namespace fs = std::filesystem;
        std::vector<fs::path> matchedFiles;
        std::regex regexPattern(pattern);

        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            SPDLOG_ERROR("Directory not found or is not a directory: {}", directory);
            throw std::runtime_error("Directory not found or is not a directory: " + directory);
        }

        // 遍历目录中的所有文件
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (fs::is_regular_file(entry.path())) {
                const std::string fileName = entry.path().filename().string();

                // 如果文件名匹配正则表达式，则将其添加到结果中
                if (std::regex_match(fileName, regexPattern)) {
                    matchedFiles.push_back(entry.path());
                }
            }
        }

        return matchedFiles;
    }
    /**
     * @brief 将std::vector<T>中的数据保存到二进制文件中
     * @param data 数据
     * @param filePath 文件路径
     * @throw std::runtime_error 如果打开文件失败
     */
    template<typename T>
    static void writeToFile(const std::vector<T>& data, const std::string& filePath) {
        checkAndCreateDir(filePath);
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            SPDLOG_ERROR("Failed to open file for writing: {}", filePath);
            throw std::runtime_error("Failed to open file for writing: " + filePath);
        }
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size() * sizeof(T)));
        file.close();
    }
};