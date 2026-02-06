#pragma once
#include <spdlog/spdlog.h>

#include <string>

namespace bchtree {
class Logger {
   public:
    void setLevel(const std::string& level);
    void setFile(const std::string& path);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void debug(const std::string& msg);
    void flush();

   private:
    void ensure_init();
    bool initialized_{false};
    std::shared_ptr<spdlog::logger> logger_;
    std::string file_path_;
};
}  // namespace bchtree
