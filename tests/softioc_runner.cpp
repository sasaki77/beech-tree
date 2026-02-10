#include "softioc_runner.h"

#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <string>
#include <thread>

pid_t SoftIocRunner::Start(const std::string& db_text) {
    WriteDBtoTemp(db_text);

    // Spawn a child process to run 'cmd' and return PID
    pid_ = fork();
    const std::string cmd = "softIoc -d \"" + temp_db_path_.string() + "\"";

    if (pid_ == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);

        // exec failed: exit child immediately (127 = command-not-found).
        // Use _exit() to avoid stdio/atexit effects.
        _exit(127);
    }

    return pid_;
}

void SoftIocRunner::KillIfRunning() {
    // If process is alive, try SIGTERM then SIGKILL
    if (pid_ <= 0) {
        return;
    }

    // Remove the temp db file
    std::error_code ec;
    if (!temp_db_path_.empty()) {
        std::filesystem::remove(temp_db_path_, ec);
    }

    // Try graceful SIGINT
    kill(pid_, SIGINT);
    for (int i = 0; i < 100; ++i) {  // 5s
        if (waitpid(pid_, nullptr, WNOHANG) == pid_) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Then SIGTERM with longer grace
    kill(pid_, SIGTERM);
    for (int i = 0; i < 100; ++i) {  // 5s
        if (waitpid(pid_, nullptr, WNOHANG) == pid_) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Last resort
    kill(pid_, SIGKILL);
    waitpid(pid_, nullptr, 0);
}

// Write DB text to a safely created temp file under testing::TempDir()
void SoftIocRunner::WriteDBtoTemp(const std::string& db_text) {
    const std::string base = ::testing::TempDir();

    if (base.empty()) {
        throw std::runtime_error("testing::TempDir() returned empty");
    }

    // Build mkstemps() template
    std::filesystem::path dir(base);
    std::string tmpl = (dir / "bch-db-XXXXXX.db").string();

    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');

    int fd = mkstemps(buf.data(), /*suffixlen=*/3);
    if (fd == -1) {
        throw std::runtime_error("mkstemps failed for: " + tmpl);
    }

    // Restrict permission
    fchmod(fd, 0600);

    // Close the fd immediately
    // subsequent writes will go through ofstream
    close(fd);

    // Open std::ofstream and enable exceptions.
    std::filesystem::path file_path(buf.data());
    std::ofstream ofs(file_path, std::ios::binary);
    ofs.exceptions(std::ios::badbit | std::ios::failbit);

    // Stream the text as-is
    ofs << db_text;

    // Ensure data is on disk; with exceptions set, errors throw here as
    // well.
    ofs.flush();

    temp_db_path_ = file_path;
}
