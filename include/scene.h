#pragma once

#include <vector>
#include <memory>

#include "object.h"

class Scene
{
public:
    struct SceneSettings
    {
        uint32_t maxBounce = 4;
        uint32_t numRaysPerPixel = 1;
        uint32_t isSSAAEnabled = 0;
    };

    Scene() = default;
    ~Scene() = default;

    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;

    bool loadFromFile(std::string &filePath, bool saveLast);
    bool loadFromSource(const char *source, bool saveLast);

    bool saveToFile(const std::string &filepath) const;
    bool saveToFile() const;

    template <typename T_>
    T_ *add(std::unique_ptr<T_> object)
    {
        static_assert(std::is_base_of_v<Object, T_>, "Object must derive from Object");

        if (!object)
            return nullptr;

        object->materialIdx_ = static_cast<uint32_t>(materials_.size());
        materials_.push_back(&object->material_);

        T_ *raw = object.get();
        objects_.push_back(std::move(object));

        return raw;
    }

    bool remove(Object *target);

    void syncMaterials();

    template <typename T_>
    std::vector<T_ *> getObjectsOfType() const
    {
        std::vector<T_ *> output;
        for (const std::unique_ptr<Object> &obj : objects_)
            if (T_ *casted = dynamic_cast<T_ *>(obj.get())) // i had no idea you can instantiate variables in if statements
                output.push_back(casted);

        return output;
    }

    void setSettings(SceneSettings settings) { settings_ = settings; }
    const SceneSettings &settings() const { return settings_; }

    size_t objectCount() const { return objects_.size(); }
    size_t materialCount() const { return materials_.size(); }

    std::vector<std::unique_ptr<Object>> objects_;
    std::vector<Object::Material *> materials_; // owned by objects_

private:
    SceneSettings settings_;
    std::string currentFile_ = "";
};