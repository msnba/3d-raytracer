#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <variant>
#include <stdexcept>

using SettingValue = std::variant<int, float, bool, std::string>;

class Settings
{
public:
    static Settings &get()
    {
        static Settings instance; // keeps one instance of settings
        return instance;
    }

    bool loadFromFile(const std::string &filepath, bool saveLast);
    bool loadFromSource(const char *source, bool saveLast);

    bool saveToFileImpl(const std::string &filepath) const;
    bool saveToFile(const std::string &filepath) const;
    bool saveToFile() const;

    template <typename T_>
    void setValue(const std::string &key, T_ value)
    {
        std::lock_guard lock(mutex_);
        settingsMap_[key] = SettingValue(value);
    }

    template <typename T_>
    T_ getValue(const std::string &key, T_ fallback) const
    {
        std::lock_guard lock(mutex_);
        auto it = settingsMap_.find(key);
        if (it == settingsMap_.end())
            return fallback;

        if (auto *val = std::get_if<T_>(&it->second))
            return *val;

        throw std::runtime_error("Key \"" + key + "\" exists, but wrong type \"" + typeid(T_).name() + "\" requested.");
    }

private:
    static SettingValue parseValue(const std::string &value);

    Settings() {}

    std::unordered_map<std::string, SettingValue> settingsMap_;
    mutable std::mutex mutex_;
    std::string currentFile_ = "";
};
