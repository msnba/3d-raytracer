#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <functional>

#include "scene.h"
#include "log.h"

using Writer = rapidjson::Writer<rapidjson::FileWriteStream>;

struct ObjectIO
{
    std::function<std::unique_ptr<Object>(const rapidjson::Value &)> read;
    std::function<void(Writer &, const Object *)> write;
};

// read helpers for objects

static glm::vec3 readVec3(const rapidjson::Value &v)
{
    return glm::vec3(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat());
}

static glm::vec4 readVec4(const rapidjson::Value &v)
{
    return glm::vec4(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat(), v[3].GetFloat());
}

static Object::Transform readTransform(const rapidjson::Value &obj)
{
    Object::Transform t;
    if (obj.HasMember("position") && obj["position"].IsArray())
        t.position = readVec3(obj["position"]);
    if (obj.HasMember("rotation") && obj["rotation"].IsArray())
        t.rotation = readVec3(obj["rotation"]);
    if (obj.HasMember("scale") && obj["scale"].IsArray())
        t.scale = readVec3(obj["scale"]);
    return t;
}

static Object::Material readMaterial(const rapidjson::Value &obj)
{
    Object::Material m;
    if (obj.HasMember("color") && obj["color"].IsArray())
        m.color = readVec3(obj["color"]);
    if (obj.HasMember("emission") && obj["emission"].IsArray())
        m.emission = readVec4(obj["emission"]);
    if (obj.HasMember("smoothness") && obj["smoothness"].IsFloat())
        m.smoothness = obj["smoothness"].GetFloat();
    if (obj.HasMember("transparency") && obj["transparency"].IsFloat())
        m.transparency = obj["transparency"].GetFloat();
    if (obj.HasMember("ior") && obj["ior"].IsFloat())
        m.ior = obj["ior"].GetFloat();
    return m;
}

// object registry / write functions

static void writeVec3(Writer &w, const glm::vec3 &v)
{
    w.StartArray();
    w.Double(v.x);
    w.Double(v.y);
    w.Double(v.z);
    w.EndArray();
}

static void writeVec4(Writer &w, const glm::vec4 &v)
{
    w.StartArray();
    w.Double(v.x);
    w.Double(v.y);
    w.Double(v.z);
    w.Double(v.w);
    w.EndArray();
}

static void writeTransform(Writer &w, const Object::Transform &t)
{
    w.Key("position");
    writeVec3(w, t.position);
    w.Key("rotation");
    writeVec3(w, t.rotation);
    w.Key("scale");
    writeVec3(w, t.scale);
}

static void writeMaterial(Writer &w, const Object::Material &m)
{
    w.Key("color");
    writeVec3(w, m.color);
    w.Key("emission");
    writeVec4(w, m.emission);
    w.Key("smoothness");
    w.Double(m.smoothness);
    w.Key("transparency");
    w.Double(m.transparency);
    w.Key("ior");
    w.Double(m.ior);
}

static const std::unordered_map<std::string, ObjectIO> &registry()
{
    static std::unordered_map<std::string, ObjectIO> r =
        {
            {"sphere",
             {[](const rapidjson::Value &val) -> std::unique_ptr<Object>
              {
                  auto obj = std::make_unique<Sphere>();
                  obj->transform_ = readTransform(val);
                  obj->material_ = readMaterial(val);
                  if (val.HasMember("radius") && val["radius"].IsFloat())
                      obj->radius_ = val["radius"].GetFloat();
                  return obj;
              },
              [](Writer &w, const Object *obj)
              {
                  const Sphere *s = static_cast<const Sphere *>(obj);
                  writeTransform(w, s->transform_);
                  writeMaterial(w, s->material_);
                  w.Key("radius");
                  w.Double(s->radius_);
              }}},
            {"cube",
             {[](const rapidjson::Value &val) -> std::unique_ptr<Object>
              {
                  glm::vec3 pos(0), rot(0), scale(1);
                  if (val.HasMember("position") && val["position"].IsArray())
                      pos = readVec3(val["position"]);
                  if (val.HasMember("rotation") && val["rotation"].IsArray())
                      rot = readVec3(val["rotation"]);
                  if (val.HasMember("scale") && val["scale"].IsArray())
                      scale = readVec3(val["scale"]);
                  return std::make_unique<Cube>(pos, rot, scale.x, scale.y, scale.z, readMaterial(val));
              },
              [](Writer &w, const Object *obj)
              {
                  writeTransform(w, obj->transform_);
                  writeMaterial(w, obj->material_);
              }}},
            {"mesh",
             {[](const rapidjson::Value &val) -> std::unique_ptr<Object>
              {
                  if (!val.HasMember("path") || !val["path"].IsString() || val["path"].GetStringLength() == 0)
                      return nullptr;
                  return std::make_unique<Mesh>(val["path"].GetString(), readTransform(val), readMaterial(val));
              },
              [](Writer &w, const Object *obj)
              {
                  const Mesh *m = static_cast<const Mesh *>(obj);
                  writeTransform(w, m->transform_);
                  writeMaterial(w, m->material_);
                  w.Key("path");
                  w.String(m->path_.c_str());
              }}},
        };
    return r;
}

bool Scene::loadFromFile(std::string &filePath, bool saveLast)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

    std::string json((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    if (saveLast)
        saveToFile();

    currentFile_ = filePath;
    return loadFromSource(json.c_str(), false);
}
bool Scene::loadFromSource(const char *source, bool saveLast)
{
    if (saveLast)
        saveToFile();

    rapidjson::Document doc;
    doc.Parse(source);

    if (doc.HasParseError())
        return false;

    objects_.clear();
    materials_.clear();

    // per scene settings
    if (doc.HasMember("settings") && doc["settings"].IsObject())
    {
        // overrides
        const auto &s = doc["settings"];
        if (s.HasMember("maxBounce") && s["maxBounce"].IsUint())
            settings_.maxBounce = s["maxBounce"].GetUint();
        if (s.HasMember("numRaysPerPixel") && s["numRaysPerPixel"].IsUint())
            settings_.numRaysPerPixel = s["numRaysPerPixel"].GetUint();
        if (s.HasMember("isSSAAEnabled") && s["isSSAAEnabled"].IsUint())
            settings_.isSSAAEnabled = s["isSSAAEnabled"].GetUint();
    }

    // structure: "objects": {"spheres": [...], "meshes": [...]}
    if (doc.HasMember("objects") && doc["objects"].IsObject())
    {
        for (const auto &member : doc["objects"].GetObject())
        {
            std::string type = member.name.GetString();
            auto it = registry().find(type);
            if (it == registry().end() || !member.value.IsArray())
                continue;

            for (const auto &val : member.value.GetArray())
            {
                auto obj = it->second.read(val);
                if (obj)
                    add(std::move(obj));
            }
        }
    }

    syncMaterials();
    return true;
}
bool Scene::saveToFile(const std::string &filepath) const
{
    if (filepath.empty())
    {
        Log::error("Filepath is false.");
        return false;
    }
#ifdef _WIN32
    FILE *fp = std::fopen(filepath.c_str(), "wb");
#else
    FILE *fp = std::fopen(filepath.c_str(), "w");
#endif
    if (!fp)
    {
        Log::error("File not initialized correctly to write to.");
        return false;
    }

    char writeBuffer[65536];
    rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

    Writer writer(os);
    writer.StartObject();

    // settings
    writer.Key("settings");
    writer.StartObject();

    writer.Key("maxBounce");
    writer.Uint(settings_.maxBounce);
    writer.Key("numRaysPerPixel");
    writer.Uint(settings_.numRaysPerPixel);
    writer.Key("isSSAAEnabled");
    writer.Uint(settings_.isSSAAEnabled);

    writer.EndObject();

    std::unordered_map<std::string, std::vector<const Object *>> grouped;
    for (const auto &object : objects_)
        grouped[object->typeName()].push_back(object.get());

    writer.Key("objects");
    writer.StartObject();

    for (const auto &[type, objects] : grouped)
    {
        auto it = registry().find(type);
        if (it == registry().end())
            continue;

        writer.Key(type.c_str());
        writer.StartArray();

        for (const Object *object : objects)
        {
            writer.StartObject();
            it->second.write(writer, object);
            writer.EndObject();
        }

        writer.EndArray();
    }

    writer.EndObject();
    writer.EndObject();

    os.Flush();
    std::fclose(fp);
    return true;
}
bool Scene::saveToFile() const
{
    if (!currentFile_.empty())
        return saveToFile(currentFile_);

    return false;
}

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