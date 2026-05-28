#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdint.h>
#include <string>

namespace tinytracer::core {

class FilePicker {
public:
  struct Entry {
    std::string fileExtension;
    std::string defaultFilename;
    std::string dialogTitle;
  };

  static constexpr size_t kMaxQueue = 8;

  static FilePicker &get() {
    static FilePicker instance; // keeps one instance of FilePicker
    return instance;
  }

  ~FilePicker();

  std::string
  query(const Entry &entry, bool isSave,
        std::function<void(const std::string &)> onResult = nullptr);

private:
  struct QueuedEntry {
    std::promise<std::string> promise;
    Entry entry;
    bool isSave = false;
    std::function<void(const std::string &)> callback;
  };

  FilePicker();

  void workerLoop();
  std::string runDialog(const QueuedEntry &entry);

  static std::string ensureExtension(std::string path, const std::string &ext);

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<QueuedEntry> entries_;
  std::thread thread_;
  bool isShutdown_ = false;
};

}