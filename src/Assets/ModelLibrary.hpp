#pragma once
#include "Model.hpp"
#include <queue>

namespace Vulkan
{
	class CommandPool;
}

namespace Assets
{
    struct ModelLoadResult
    {
        std::vector<std::shared_ptr<Model>> modelsData;
        std::vector<glm::vec3> originalPositions;
    };

    class ModelLibrary
    {
    public:
        using String = std::string;
		using ModelPtr = std::shared_ptr<Model>;
        using ModelList = std::vector<ModelPtr>;
        using ModelMap = std::unordered_map<String, ModelLoadResult>;

        static ModelLibrary* getInstance();
        static void initialize();
        static void destroy();

        ModelLoadResult LoadModel(const std::string& filePath);
        ModelLoadResult GetModel(const String& meshName);

        void BuildScheduledModelBLAS(Vulkan::CommandPool& commandPool);
        bool HasScheduledModels() const { return !m_scheduledModels.empty(); }

    private:
        ModelLibrary();
        ~ModelLibrary() = default;
        ModelLibrary(ModelLibrary const&) {};             // copy constructor is private
        ModelLibrary& operator=(ModelLibrary const&) {};  // assignment operator is private*/

        static Assets::ModelLibrary* sharedInstance;

        void LoadInitialModels();
        void LoadBox();
        void LoadPlane();
        void LoadSphere();
        void LoadCapsule();
        void LoadCylinder();
        void LoadCornellBox();

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
        std::queue<ModelPtr> m_scheduledModels;
    };
}
