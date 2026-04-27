#pragma once

#include <vector>
#include <memory>

#include "object.h"

class Scene
{
public:
    std::vector<std::unique_ptr<Object>> objects_;
    std::vector<Object::Material *> materials_; // owned by objects_

    Scene() = default;

    template <typename T_>
    bool add(std::unique_ptr<T_> object)
    {
        if (!object)
            return false;

        object->materialIdx_ = static_cast<uint32_t>(materials_.size());
        materials_.push_back(&object->material_);
        objects_.push_back(std::move(object));

        return true;
    }

    template <typename T_>
    std::vector<T_ *> getObjectsOfType()
    {
        std::vector<T_ *> output;
        for (const std::unique_ptr<Object> &obj : objects_)
        {
            T_ *casted = dynamic_cast<T_ *>(obj.get()); // returns nullptr if obj is not of type T_
            if (casted)
                output.push_back(casted);
        }

        return output;
    }
};