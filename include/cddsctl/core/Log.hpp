#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace cddsctl {
namespace log {

// Get or create the default logger
inline std::shared_ptr<spdlog::logger>& get_logger() {
    static std::shared_ptr<spdlog::logger> logger = []() {
        auto l = spdlog::stdout_color_mt("cddsctl");
        l->set_pattern("[%^%l%$] %v");
        l->set_level(spdlog::level::info);
        return l;
    }();
    return logger;
}

// Set log level
inline void set_level(spdlog::level::level_enum level) {
    get_logger()->set_level(level);
}

// Convenience functions for setting common log levels
inline void set_level_trace() { set_level(spdlog::level::trace); }
inline void set_level_debug() { set_level(spdlog::level::debug); }
inline void set_level_info() { set_level(spdlog::level::info); }
inline void set_level_warn() { set_level(spdlog::level::warn); }
inline void set_level_error() { set_level(spdlog::level::err); }

// Helper for LOG_SUCCESS - wraps message in green ANSI codes
template<typename... Args>
inline void log_success(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    auto msg = fmt::format(fmt, std::forward<Args>(args)...);
    get_logger()->info("\033[32m{}\033[0m", msg);
}

}  // namespace log
}  // namespace cddsctl

// Legacy log level enum for compatibility
enum RecorderLogLevel {
    RECORDER_LOG_LEVEL_TRACE = 0,
    RECORDER_LOG_LEVEL_DEBUG = 1,
    RECORDER_LOG_LEVEL_INFO = 2,
    RECORDER_LOG_LEVEL_WARN = 3,
    RECORDER_LOG_LEVEL_ERROR = 4,
    RECORDER_LOG_LEVEL_CRITICAL = 5,
    RECORDER_LOG_LEVEL_OFF = 6
};

// Legacy function for compatibility
inline void recorder_set_log_level(RecorderLogLevel level) {
    static const spdlog::level::level_enum level_map[] = {
        spdlog::level::trace,
        spdlog::level::debug,
        spdlog::level::info,
        spdlog::level::warn,
        spdlog::level::err,
        spdlog::level::critical,
        spdlog::level::off
    };
    cddsctl::log::set_level(level_map[level]);
}

// Log macros using spdlog fmt-style formatting
#define LOG_TRACE(...)    SPDLOG_LOGGER_TRACE(cddsctl::log::get_logger(), __VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(cddsctl::log::get_logger(), __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO(cddsctl::log::get_logger(), __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN(cddsctl::log::get_logger(), __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(cddsctl::log::get_logger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(cddsctl::log::get_logger(), __VA_ARGS__)

// LOG_SUCCESS - uses green color (info level)
#define LOG_SUCCESS(...) cddsctl::log::log_success(__VA_ARGS__)
