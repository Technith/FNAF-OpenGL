#pragma once
#include <memory>
#include "ShaderProgram.h"
#include "Mesh.h"

class Object3D {
private:
	// The object's list of meshes and children.

	// An object might have many Meshes, all drawn with the same model matrix...
	std::vector<Mesh> m_meshes{};
	// ... and it also might have many child Object, that have their own model matrix that is 
	// relative to their parent.
	std::vector<Object3D> m_children{};

	// The object's position, orientation, and scale in world space.
	glm::vec3 m_position{};
	glm::vec3 m_orientation{};
	glm::vec3 m_scale{1.0, 1.0, 1.0};
	// The object's center of rotation, if we want to rotate around some other point
	// than the local space origin.
	glm::vec3 m_center{};

	// The object's material parameters (ambient, diffuse, specular, shininess).
	glm::vec4 m_material{0.1, 1.0, 0.3, 4};

	// The object's base transformation matrix, which is used by some model formats to set a "starting" 
	// transformation for this object relative to its parent.
	glm::mat4 m_baseTransform{};

	// Some objects from Assimp imports have a "name" field, useful for debugging.
	std::string m_name{};

	// Recomputes the local->world transformation matrix.
	glm::mat4 buildModelMatrix() const;

public:
	// No default constructor; you must have a mesh to initialize an object.
	Object3D() = delete;

	Object3D(std::vector<Mesh> meshes);
	Object3D(std::vector<Mesh> meshes, glm::mat4 baseTransform);

	// Simple accessors.
	const glm::vec3& getPosition() const;
	const glm::vec3& getOrientation() const;
	const glm::vec3& getScale() const;
	const glm::vec3& getCenter() const;
	const std::string& getName() const;
	const glm::vec4& getMaterial() const;

	// Child management.
	size_t numberOfChildren() const;
	const Object3D& getChild(size_t index) const;
	Object3D& getChild(size_t index);


	// Simple mutators.
	void setPosition(glm::vec3 position);
	void setOrientation(glm::vec3 orientation);
	void setScale(glm::vec3 scale);
	void setCenter(glm::vec3 center);
	void setName(std::string name);
	void setMaterial(glm::vec4 material);

	// Transformations.
	void move(const glm::vec3& offset);
	void rotate(const glm::vec3& rotation);
	void grow(const glm::vec3& growth);
	void addChild(Object3D child);

	// Rendering.
	void render(ShaderProgram& shaderProgram) const;
	void renderRecursive(ShaderProgram& shaderProgram, const glm::mat4& parentMatrix) const;
};