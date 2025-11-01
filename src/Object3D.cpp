#include "Object3D.h"
#include "ShaderProgram.h"
#include <glm/ext.hpp>

glm::mat4 Object3D::buildModelMatrix() const {
	auto m = glm::translate(glm::mat4{ 1 }, m_position);
	m = glm::translate(m, m_center * m_scale);
	m = glm::rotate(m, m_orientation[2], glm::vec3{ 0, 0, 1 });
	m = glm::rotate(m, m_orientation[0], glm::vec3{ 1, 0, 0 });
	m = glm::rotate(m, m_orientation[1], glm::vec3{ 0, 1, 0 });
	m = glm::scale(m, m_scale);
	m = glm::translate(m, -m_center);
	m = m * m_baseTransform;
	return m;
}

Object3D::Object3D(std::vector<Mesh> meshes)
	: Object3D{ std::move(meshes), glm::mat4 {1} } {
}

Object3D::Object3D(std::vector<Mesh> meshes, glm::mat4 baseTransform)
	: m_meshes{ std::move(meshes) }, m_baseTransform{ std::move(baseTransform) }
{
}

const glm::vec3& Object3D::getPosition() const {
	return m_position;
}

const glm::vec3& Object3D::getOrientation() const {
	return m_orientation;
}

const glm::vec3& Object3D::getScale() const {
	return m_scale;
}

/**
 * @brief Gets the center of the object's rotation.
 */
const glm::vec3& Object3D::getCenter() const {
	return m_center;
}

const std::string& Object3D::getName() const {
	return m_name;
}

const glm::vec4& Object3D::getMaterial() const {
	return m_material;
}

size_t Object3D::numberOfChildren() const {
	return m_children.size();
}

const Object3D& Object3D::getChild(size_t index) const {
	return m_children[index];
}

Object3D& Object3D::getChild(size_t index) {
	return m_children[index];
}

void Object3D::setPosition(glm::vec3 position) {
	m_position = position;
}

void Object3D::setOrientation(glm::vec3 orientation) {
	m_orientation = orientation;
}

void Object3D::setScale(glm::vec3 scale) {
	m_scale = scale;
}

/**
 * @brief Sets the center point of the object's rotation, which is otherwise a rotation around
   the origin in local space..
 */
void Object3D::setCenter(glm::vec3 center)
{
	m_center = center;
}

void Object3D::setName(std::string name) {
	m_name = name;
}

void Object3D::setMaterial(glm::vec4 material) {
	m_material = material;
	for (auto& c : m_children) {
		c.setMaterial(material);
	}
}

void Object3D::move(const glm::vec3& offset) {
	m_position = m_position + offset;
}

void Object3D::rotate(const glm::vec3& rotation) {
	m_orientation = m_orientation + rotation;
}

void Object3D::grow(const glm::vec3& growth) {
	m_scale = m_scale * growth;
}

void Object3D::addChild(Object3D child) {
	m_children.emplace_back(std::move(child));
}

void Object3D::render(ShaderProgram& shaderProgram) const {
	renderRecursive(shaderProgram, glm::mat4{ 1 });
}

/**
 * @brief Renders the object and its children, recursively.
 * @param parentMatrix the model matrix of this object's parent in the model hierarchy.
 */
void Object3D::renderRecursive(ShaderProgram& shaderProgram, const glm::mat4& parentModel) const {
	// Build the local model matrix, which is relative to the parent model matrix.
	glm::mat4 localModel{ buildModelMatrix() };

	// TODO: to render a hierarchical mesh, we must build the "true" model matrix for htis object,
	// which is the combination of its parent's model matrix and the local model matrix.
	// At the moment, we assume that localModel == trueModel, which is only true for the root 
	// of a hierarchy. So this code only draws a mesh correctly if it has no children at all.

	glm::mat4 trueModel{ parentModel * localModel };
	// TODO: you must change the calculation above, so that parentModel is combined with localModel to 
	// make trueModel. We want localModel's transformations to happen BEFORE parentModel's.



	shaderProgram.setUniform("model", trueModel);
	shaderProgram.setUniform("material", m_material);
	// Render each *mesh* in the object.
	for (auto& mesh : m_meshes) {
		mesh.render(shaderProgram);
	}

	// TODO: to render the rest of the hierarchy, you must loop through each element of "m_children",
	// and have them render themselves recursively. The parent model matrix for your children is your own
	// true model matrix.
	for (auto& child : m_children) {
		child.renderRecursive(shaderProgram, trueModel);
	}
}
