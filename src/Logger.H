// Logger.H - Structured logging wrapper for dbd2netcdf
//
// Provides consistent logging across all executables with configurable
// log levels and output formatting.
//
// Feb-2026, Pat Welch, pat@mousebrains.com

#ifndef INC_Logger_H
#define INC_Logger_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace dbd {

// Log levels matching spdlog
enum class LogLevel {
    Trace = spdlog::level::trace,
    Debug = spdlog::level::debug,
    Info = spdlog::level::info,
    Warn = spdlog::level::warn,
    Error = spdlog::level::err,
    Critical = spdlog::level::critical,
    Off = spdlog::level::off
};

class Logger {
public:
    // Get the singleton logger instance
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    // Initialize with program name and log level
    void init(const std::string& name, LogLevel level = LogLevel::Info) {
        if (mInitialized) return;

        try {
            mLogger = spdlog::stderr_color_mt(name);
            mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
            mLogger->set_level(static_cast<spdlog::level::level_enum>(level));
            mInitialized = true;
        } catch (const spdlog::spdlog_ex& ex) {
            // Logger already exists, get it
            mLogger = spdlog::get(name);
            if (mLogger) {
                mLogger->set_level(static_cast<spdlog::level::level_enum>(level));
                mInitialized = true;
            }
        }
    }

    // Set log level
    void setLevel(LogLevel level) {
        if (mLogger) {
            mLogger->set_level(static_cast<spdlog::level::level_enum>(level));
        }
    }

    // Get the underlying spdlog logger
    std::shared_ptr<spdlog::logger> get() {
        if (!mInitialized) {
            init("dbd2netcdf");
        }
        return mLogger;
    }

    // Convenience methods
    template<typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        get()->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        get()->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        get()->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        get()->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        get()->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        get()->critical(fmt, std::forward<Args>(args)...);
    }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::shared_ptr<spdlog::logger> mLogger;
    bool mInitialized = false;
};

// Convenience function to get the logger
inline Logger& logger() {
    return Logger::instance();
}

// Convert string to LogLevel
inline LogLevel logLevelFromString(const std::string& level) {
    if (level == "trace") return LogLevel::Trace;
    if (level == "debug") return LogLevel::Debug;
    if (level == "info") return LogLevel::Info;
    if (level == "warn" || level == "warning") return LogLevel::Warn;
    if (level == "error") return LogLevel::Error;
    if (level == "critical") return LogLevel::Critical;
    if (level == "off") return LogLevel::Off;
    return LogLevel::Info;  // Default
}

// Convert LogLevel to string
inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "trace";
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warn: return "warn";
        case LogLevel::Error: return "error";
        case LogLevel::Critical: return "critical";
        case LogLevel::Off: return "off";
    }
    return "info";
}

} // namespace dbd

// Convenience macros for logging
#define LOG_TRACE(...) dbd::logger().trace(__VA_ARGS__)
#define LOG_DEBUG(...) dbd::logger().debug(__VA_ARGS__)
#define LOG_INFO(...) dbd::logger().info(__VA_ARGS__)
#define LOG_WARN(...) dbd::logger().warn(__VA_ARGS__)
#define LOG_ERROR(...) dbd::logger().error(__VA_ARGS__)
#define LOG_CRITICAL(...) dbd::logger().critical(__VA_ARGS__)

#endif // INC_Logger_H
