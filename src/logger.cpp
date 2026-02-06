#include "logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace bchtree {
void Logger::setLevel(const std::string& level) {
    if (level == "info")
        spdlog::set_level(spdlog::level::info);
    else if (level == "warn")
        spdlog::set_level(spdlog::level::warn);
    else if (level == "error")
        spdlog::set_level(spdlog::level::err);
    else if (level == "debug")
        spdlog::set_level(spdlog::level::debug);
    else
        spdlog::set_level(spdlog::level::info);
}

void Logger::setFile(const std::string& path) { file_path_ = path; }

void Logger::ensure_init() {
    if (initialized_) return;
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::vector<spdlog::sink_ptr> sinks{console_sink};

    if (!file_path_.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            file_path_, true);
        sinks.push_back(file_sink);
    }
    logger_ =
        std::make_shared<spdlog::logger>("bt", sinks.begin(), sinks.end());
    spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e %z [%l] %v");
    initialized_ = true;
}

void Logger::info(const std::string& msg) {
    ensure_init();
    spdlog::info(msg);
}
void Logger::warn(const std::string& msg) {
    ensure_init();
    spdlog::warn(msg);
}
void Logger::error(const std::string& msg) {
    ensure_init();
    spdlog::error(msg);
}
void Logger::debug(const std::string& msg) {
    ensure_init();
    spdlog::debug(msg);
}
void Logger::flush() { logger_->flush(); }

}  // namespace bchtree
