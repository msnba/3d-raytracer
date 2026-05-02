#include "settings.h"

#include <stdint.h>
#include <fstream>
#include <sstream>
#include <charconv>

static std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

bool Settings::loadFromFile(const std::string &filepath, bool saveLast)
{
    if (saveLast)
        saveToFile(currentFile_);

    std::ifstream file(filepath);
    if (!file.is_open())
        return false;

    std::lock_guard lock(mutex_);
    currentFile_ = filepath;

    std::string line;
    while (getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string key, value;
        if (!std::getline(ss, key, '=') || !std::getline(ss, value))
            continue;

        key = trim(key);
        value = trim(value);

        if (!key.empty())
            settingsMap_[key] = parseValue(value);
    }

    return true;
}

bool Settings::loadFromSource(const char *source, bool saveLast)
{
    if (saveLast)
        saveToFile(currentFile_);

    if (!source)
        return false;

    std::lock_guard lock(mutex_);
    currentFile_ = "";

    std::istringstream stream(source);
    std::string line;
    while (getline(stream, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string key, value;
        if (!std::getline(ss, key, '=') || !std::getline(ss, value))
            continue;

        key = trim(key);
        value = trim(value);

        if (!key.empty())
            settingsMap_[key] = parseValue(value);
    }

    return true;
}

bool Settings::saveToFileImpl(const std::string &filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open())
        return false;

    for (const auto &[key, value] : settingsMap_)
    {
        file << key << " = ";
        std::visit([&file](const auto &v)
                   {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>)
                file << (v ? "true" : "false");
            else
                file << v; }, value);
        file << "\n";
    }

    return file.good();
}

bool Settings::saveToFile(const std::string &filepath) const
{
    std::lock_guard lock(mutex_);
    return saveToFileImpl(filepath);
}

bool Settings::saveToFile() const
{
    std::lock_guard lock(mutex_);
    if (currentFile_.empty())
        return false;
    return saveToFileImpl(currentFile_);
}

SettingValue Settings::parseValue(const std::string &value)
{
    if (value == "true" || value == "false")
        return value == "true";

    // type checking without having to use try/catch
    int i;
    auto [ptr_i, ec_i] = std::from_chars(value.data(), value.data() + value.size(), i);
    if (ec_i == std::errc{} && ptr_i == value.data() + value.size())
        return i;

    float f;
    auto [ptr_f, ec_f] = std::from_chars(value.data(), value.data() + value.size(), f);
    if (ec_f == std::errc{} && ptr_f == value.data() + value.size())
        return f;

    return value;
}