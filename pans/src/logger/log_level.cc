#include "pans/include/pans/logger/log_level.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace pans
{
std::string_view LogLevel::ToString(Level level) noexcept
{
    switch (level)
    {
#define XX(name) case Level::LOG_LV_##name: return #name;
        XX(DEBUG)
        XX(INFO)
        XX(WARN)
        XX(ERROR)
        XX(FATAL)
        XX(OFF)
#undef XX
    }
    return "UNKNOWN";
}

LogLevel::Level LogLevel::FromString(std::string_view value) noexcept
{
    std::array<char, 6> normalized{};
    std::transform(value.begin(), value.end(), normalized.begin(), [](unsigned char character){
        return static_cast<char>(std::toupper(character));
    });
    const std::string_view upper_value(normalized.data(), value.size());
#define XX(name) if(upper_value == #name) return Level::LOG_LV_##name;
    XX(DEBUG)
    XX(INFO)
    XX(WARN)
    XX(ERROR)
    XX(FATAL)
    XX(OFF)
#undef XX
    return LogLevel::Level::LOG_LV_OFF;
}



} // namespace pans
