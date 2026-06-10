#pragma once

#include "PhysicsDefines.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Anito::Physics {

	/**
	 * @brief Factory for creating common physics shapes
	 * 
	 * Provides convenient methods for creating standard physics shapes
	 * for use with physics bodies. Shapes are immutable once created.
	 */
	class PhysicsShapes {
	public:
		/**
		 * @brief Create a box shape
		 * @param halfExtents Half the width, height, and depth
		 */
		static std::shared_ptr<const JoltShape> CreateBox(const glm::vec3& halfExtents);

		/**
		 * @brief Create a sphere shape
		 * @param radius The sphere radius
		 */
		static std::shared_ptr<const JoltShape> CreateSphere(float radius);

		/**
		 * @brief Create a capsule shape
		 * @param halfHeight Half the height of the capsule (excluding the spheres)
		 * @param radius The radius of the capsule
		 */
		static std::shared_ptr<const JoltShape> CreateCapsule(float halfHeight, float radius);

		/**
		 * @brief Create a cylinder shape
		 * @param halfHeight Half the height of the cylinder
		 * @param radius The radius of the cylinder
		 */
		static std::shared_ptr<const JoltShape> CreateCylinder(float halfHeight, float radius);

		/**
		 * @brief Create a convex hull from vertices
		 * @param vertices List of vertex positions
		 */
		static std::shared_ptr<const JoltShape> CreateConvexHull(const std::vector<glm::vec3>& vertices);

		/**
		 * @brief Create a static triangle mesh (for terrain, level geometry)
		 * @param vertices List of vertex positions
		 * @param indices Triangle indices into the vertex list
		 */
		static std::shared_ptr<const JoltShape> CreateTriangleMesh(
			const std::vector<glm::vec3>& vertices,
			const std::vector<uint32_t>& indices
		);

		/**
		 * @brief Create a compound shape from multiple shapes
		 * @param shapes The shapes to combine
		 * @param positions Positions of each shape relative to parent
		 * @param rotations Rotations of each shape relative to parent
		 */
		static std::shared_ptr<const JoltShape> CreateCompound(
			const std::vector<std::shared_ptr<const JoltShape>>& shapes,
			const std::vector<glm::vec3>& positions,
			const std::vector<glm::quat>& rotations
		);

		/**
		 * @brief Create a plane shape (infinite flat surface)
		 */
		static std::shared_ptr<const JoltShape> CreatePlane();

		// --- Common prefabs ---

		/**
		 * @brief Create a 1x1x1 cube
		 */
		static std::shared_ptr<const JoltShape> CreateUnitCube();

		/**
		 * @brief Create a unit sphere (radius 1.0)
		 */
		static std::shared_ptr<const JoltShape> CreateUnitSphere();

		/**
		 * @brief Create a unit capsule
		 */
		static std::shared_ptr<const JoltShape> CreateUnitCapsule();
	};

} // namespace Anito::Physics
