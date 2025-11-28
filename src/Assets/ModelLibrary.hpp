#pragma once
#include "Model.hpp"

namespace Assets
{
    struct ModelData
    { 
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
		std::string texturePath;
    };

    class ModelLibrary
    {
    public:
        using String = std::string;
		using ModelPtr = std::shared_ptr<Model>; // Should be a unique_ptr but due to how proliferate it was used, we stick to shared_ptr; can also send shared in the future if Rendering methods is changed
        using ModelMap = std::unordered_map<String, ModelPtr>;

        static ModelLibrary* getInstance();
        static void initialize();
        static void destroy();

        ModelPtr LoadModel(const std::string& filePath);
        ModelPtr GetModel(const String& meshName);

        int GetInstanceId();
    private:
        ModelLibrary();
        ~ModelLibrary() = default;
        ModelLibrary(ModelLibrary const&) {};             // copy constructor is private
        ModelLibrary& operator=(ModelLibrary const&) {};  // assignment operator is private*/

        static Assets::ModelLibrary* sharedInstance;

        void LoadInitialModels();
        ModelPtr LoadBox();
        ModelPtr LoadPlane();
        ModelPtr LoadSphere();
        ModelPtr LoadCapsule();
        ModelPtr LoadCylinder();
        ModelPtr LoadCornellBox();

        /* Cube Properties */
        glm::vec3 m_cube_p0 = glm::vec3(0, 0, -50);
        glm::vec3 m_cube_p1 = glm::vec3(50, 50, 0);

        /* Plane Properties */
        glm::vec3 m_plane_p0 = glm::vec3(0, 0, -100);
        glm::vec3 m_plane_p1 = glm::vec3(100, -100, 0);

        /* Sphere Properties */
        glm::vec3 m_sphere_center = glm::vec3(0, 0, 0);
		float m_sphere_radius = 50.0f;

        /* Cylinder Properties */
        float m_cylinder_radius = 25.0f;
        float m_cylinder_height = 100.0f;

        /* Capsule Properties */
        float m_capsule_radius = 25.0f;
        float m_capsule_height = 100.0f;

        /* Cornell Properties */
        float m_cornell_scale = 555.0f;

        ModelMap m_meshMap;
		std::shared_ptr<Material> defaultMat;
        int instancesIdCount = 0;
    };
}
