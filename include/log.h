#pragma once
#include <string>
#include <stdint.h>
#include <chrono>
#include <mutex>

namespace Log
{
    enum class Level : uint8_t
    {
        Debug,
        Info,
        Warning,
        Error
    };

    struct Entry
    {
        Level level;
        std::string msg;
        float time;
    };

    class Logger
    {
    public:
        static constexpr size_t kCapacity = 1024;

        static Logger &get()
        {
            static Logger instance; // keeps one instance of logger
            return instance;
        }

        void push(Level level, std::string msg)
        {
            std::lock_guard lock(mutex_); // prevents other threads from writing to the log

            entries_[head_] = Entry{level, msg, std::chrono::duration<float>(std::chrono::steady_clock::now() - start_).count()};
            head_ = (head_ + 1) % kCapacity;
            if (count_ < kCapacity)
                count_++;
        }

        void clear()
        {
            std::lock_guard lock(mutex_);
            head_ = 0;
            count_ = 0;
        }

        size_t size()
        {
            std::lock_guard lock(mutex_);
            return count_;
        }

        template <typename T_>
        void forEach(T_ &&fn) const
        {
            std::lock_guard lock(mutex_);
            size_t start = (count_ < kCapacity) ? 0 : head_;
            for (size_t i = 0; i < count_; i++)
                fn(entries_[(start + i) % kCapacity]);
        }

    private:
        std::chrono::steady_clock::time_point start_;
        Logger() { start_ = std::chrono::steady_clock::now(); }

        mutable std::mutex mutex_;
        Entry entries_[kCapacity];
        size_t head_ = 0;
        size_t count_ = 0;
    };

    inline void debug(std::string msg) { Logger::get().push(Level::Debug, msg); }
    inline void info(std::string msg) { Logger::get().push(Level::Info, msg); }
    inline void warning(std::string msg) { Logger::get().push(Level::Warning, msg); }
    inline void error(std::string msg) { Logger::get().push(Level::Error, msg); }

}; // namespace Log