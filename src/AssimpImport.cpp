#include "AssimpImport.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <unordered_map>

std::vector<Texture> loadMaterialTextures(
	aiMaterial* mat,
	aiTextureType type,
	const std::string& typeName,
	const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures
) {
	std::vector<Texture> textures{};
	for (uint32_t i{ 0 }; i < mat->GetTextureCount(type); ++i) {
		aiString name{};
		mat->GetTexture(type, i, &name);
		std::filesystem::path texPath{ modelPath.parent_path() / name.C_Str() };
		std::cout << "loading " << texPath << std::endl;

		auto existing{ loadedTextures.find(texPath.string()) };
		if (existing != loadedTextures.end()) {
			textures.push_back(existing->second);
		}
		else {
			StbImage image{};
			image.loadFromFile(texPath.string());
			Texture tex{ Texture::loadImage(image, typeName) };
			textures.push_back(tex);
			loadedTextures.insert(std::make_pair(texPath.string(), tex));
		}
	}
	return textures;
}

Mesh fromAssimpMesh(const aiMesh* mesh, const aiScene* scene, const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures) {
	std::vector<Vertex3D> vertices;

	bool hasUVs = mesh->HasTextureCoords(0);

	for (size_t i{ 0 }; i < mesh->mNumVertices; i++) {
		auto& meshVertex{ mesh->mVertices[i] };
		auto& normal{ mesh->mNormals[i] };
		auto& texCoord{ mesh->mTextureCoords[0][i] };

		// TODO: construct a Vertex3D object using this assimp node's data, referenced
		// with the variables above. A Vertex3D is constructed with a position x/y/z, normal x/y/z, 
		// and texture u/v (x/y), in that order.
		// Then push_back the Vertex3D into the vertice list.
	

		Vertex3D vertex;
		vertex.x = meshVertex.x;
		vertex.y = meshVertex.y;
		vertex.z = meshVertex.z;
		vertex.nx = normal.x;
		vertex.ny = normal.y;
		vertex.nz = normal.z;
		if (hasUVs) {
			vertex.u = texCoord.x;
			vertex.v = texCoord.y;
		}
		else {
			vertex.u = 0;
			vertex.v = 0;
		}
		vertices.push_back(vertex);
	}

	std::vector<uint32_t> faces;
	for (size_t i{ 0 }; i < mesh->mNumFaces; ++i) {
		auto& meshFace{ mesh->mFaces[i] };
		faces.push_back(meshFace.mIndices[0]);
		faces.push_back(meshFace.mIndices[1]);
		faces.push_back(meshFace.mIndices[2]);
	}

	// Load any base textures, specular maps, and normal maps associated with the mesh.
	std::vector<Texture> textures{};
	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<Texture> diffuseMaps{
			loadMaterialTextures(material, aiTextureType_DIFFUSE, "baseTexture", modelPath, loadedTextures)
		};
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		std::vector<Texture> specularMaps{
			loadMaterialTextures(material, aiTextureType_SPECULAR, "specMap", modelPath, loadedTextures)
		};
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

		std::vector<Texture> normalMaps{
			loadMaterialTextures(material, aiTextureType_HEIGHT, "normalMap", modelPath, loadedTextures)
		};
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

		normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normalMap", modelPath, loadedTextures);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	}

	return Mesh{ vertices, faces, std::move(textures) };
}

Object3D assimpLoad(const std::string& path, bool flipTextureCoords) {
	Assimp::Importer importer{};

	auto options{ aiProcessPreset_TargetRealtime_MaxQuality };
	if (flipTextureCoords) {
		options |= aiProcess_FlipUVs;
	}
	const aiScene* scene{ importer.ReadFile(path, options) };

	// If the import failed, report it
	if (nullptr == scene) {
		std::string error{ importer.GetErrorString() };
		std::cerr << "Error loading assimp file: " + error << std::endl;
		throw std::runtime_error("Error loading assimp file: " + error);
	}
	std::vector<Mesh> meshes{};
	std::unordered_map<std::string, Texture> loadedTextures{};
	return processAssimpNode(scene->mRootNode, scene, std::filesystem::path{ path }, loadedTextures);
}

// A "Node" in assimp is an Object3D in our framework. It has one or more meshes,
// plus zero or more children.
Object3D processAssimpNode(
	const aiNode* node, 
	const aiScene* scene,
	const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures
) {
	// Load the aiNode's meshes.
	std::vector<Mesh> meshes{};
	for (size_t i{ 0 }; i < node->mNumMeshes; ++i) {
		aiMesh* mesh{ scene->mMeshes[node->mMeshes[i]] };
		meshes.emplace_back(fromAssimpMesh(mesh, scene, modelPath, loadedTextures));
	}

	// Load the node's textures.
	std::vector<Texture> textures{};
	for (auto& p : loadedTextures) {
		textures.push_back(p.second);
	}

	// Initialize the base transform of the object. (Needs to be transposed from assimp.)
	glm::mat4 baseTransform{};
	for (uint32_t i{ 0 }; i < 4; ++i) {
		for (uint32_t j{ 0 }; j < 4; ++j) {
			baseTransform[i][j] = node->mTransformation[j][i];
		}
	}

	// Initialize the object.
	Object3D parent{ std::move(meshes), std::move(baseTransform)};

	// Recursively process the children of the node and add them as child objects.
	for (size_t i{ 0 }; i < node->mNumChildren; ++i) {
		Object3D child{ processAssimpNode(node->mChildren[i], scene, modelPath, loadedTextures) };
		parent.addChild(std::move(child));
	}

	return parent;
}
