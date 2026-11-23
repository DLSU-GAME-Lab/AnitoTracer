#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "AutoSerializer.hpp"

// Serializable reference to a .aprefab file, stored project-relative when possible.
class PrefabRef
{
public:
    PrefabRef() = default;
    explicit PrefabRef(std::string path) : m_path(std::move(path)) {}

    const std::string &GetPath() const { return m_path; }
    void SetPath(std::string path) { m_path = std::move(path); }
    bool IsEmpty() const { return m_path.empty(); }
    bool operator==(const PrefabRef &other) const { return m_path == other.m_path; }

    // Absolute path of the referenced prefab; empty if the slot is empty.
    std::filesystem::path Resolve() const;

    // Converts an absolute prefab path to the form stored in the slot.
    static std::string MakeStorable(const std::filesystem::path &absolutePath);

    // All .aprefab files under the current project (or working directory when no project is open).
    static std::vector<std::filesystem::path> EnumeratePrefabs();

private:
    std::string m_path;
};

#include "PropertyDrawers/prefab_drawer.hpp"

namespace gbe
{
    template <>
    class AutoSerializer<PrefabRef> : public AutoSerializerBase<PrefabRef>
    {
    public:
        using AutoSerializerBase<PrefabRef>::AutoSerializerBase;

        inline void Serialize(SerializedData &data) override
        {
            data.serialized_variables.insert_or_assign(m_id, Get().GetPath());
        }

        inline void Deserialize(SerializedData &data) override
        {
            auto it = data.serialized_variables.find(m_id);
            if (it != data.serialized_variables.end())
            {
                Get().SetPath(it->second);
            }

            if (m_on_init)
            {
                m_on_init(Get());
            }
        }
    };
}
