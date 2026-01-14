#include "io/PlySchema.hpp"

/*

spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v (%s:%#)");
//                 └───┬───┘ └─┬─┘  └─┬─┘ └──┬──┘
//                     │       │      │      │
//                     │       │      │      └─ 源文件:行号
//                     │       │      └─ 消息内容
//                     │       └─ 日志级别
//                     └─ 时间戳

*/
namespace {
    auto _ = [](){
        spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v (%s:%#)");

        RegisteredSchema::registerSchema("vertex", "x", {"float"}, PropertyStorageType::FLOAT32);
        RegisteredSchema::registerSchema("vertex", "y", {"float"}, PropertyStorageType::FLOAT32);
        RegisteredSchema::registerSchema("vertex", "z", {"float"}, PropertyStorageType::FLOAT32);

        SPDLOG_INFO("config initialized");
        return 0;
    }();
};