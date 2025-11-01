#pragma once
#include <glm/ext.hpp>
#include <glad/glad.h>
#include <vector>

#include "Texture.h"
#include "ShaderProgram.h"

#define MAX_BONE_INFLUENCE 4

struct Vertex3D {
	float x;
	float y;
	float z;

	float nx;
	float ny;
	float nz;

	float u;
	float v;
};


class Mesh {
private:
	uint32_t m_vao;
	std::vector<Texture> m_textures;
	uint32_t m_vertexCount;
	uint32_t m_faceCount;

public:
	/**
	 * @brief Construcst a Mesh3D using existing vectors of vertices and faces.
	*/
	Mesh(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& faces);
	Mesh(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& faces, std::vector<Texture> textures);


	void addTexture(Texture texture);
	void addTextures(std::vector<Texture> textures);

	/**
	 * @brief Constructs a 1x1 square centered at the origin in world space.
	*/
	static Mesh square(std::vector<Texture> textures);
	
	/**
	 * @brief Renders the mesh to the given context.
	 * @param model the local->world model transformation matrix.
	 * @param view the world->view camera matrix.
	 * @param proj the view->clip projection matrix.
	*/
	void render(ShaderProgram& program) const;
	
};
