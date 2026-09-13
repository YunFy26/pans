#ifndef PANS_INCLUDE_PANS_LOGGER_LOG_LEVEL_H
#define PANS_INCLUDE_PANS_LOGGER_LOG_LEVEL_H

#include <cstdint>
#include <string_view>
#include "pans/export.h"

namespace pans
{
class PANS_API LogLevel final
{
public:
    enum class Level : std::uint8_t
    {
        LOG_LV_DEBUG = 1,
        LOG_LV_INFO = 2,
        LOG_LV_WARN = 3,
        LOG_LV_ERROR = 4,
        LOG_LV_FATAL = 5,
        LOG_LV_OFF = 6,
    };

    [[nodiscard]] static std::string_view ToString(Level level) noexcept;
    [[nodiscard]] static LogLevel::Level FromString(std::string_view value) noexcept;

};

    
} // namespace pans


#endif