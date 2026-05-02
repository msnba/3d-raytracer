#include <algorithm>

#include "scene.h"

bool Scene::remove(Object *target)
{
    if (!target)
        return false;

    // iterates through unique_ptr objects to find target
    auto iterator = std::find_if(objects_.begin(), objects_.end(), [target](const std::unique_ptr<Object> &obj)
                                 { return obj.get() == target; });

    if (iterator == objects_.end())
        return false;

    objects_.erase(iterator);

    syncMaterials();
    return true;
}

void Scene::syncMaterials()
{
    materials_.clear();
    materials_.reserve(objects_.size());

    for (const std::unique_ptr<Object> &obj : objects_)
        materials_.push_back(&obj->material_);

    // rebuild material indices
    for (uint32_t i = 0; i < static_cast<uint32_t>(objects_.size()); i++)
        objects_[i]->materialIdx_ = i;
}