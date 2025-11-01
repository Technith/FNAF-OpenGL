#pragma once
#include "Object3D.h"
#include <assimp/scene.h>
#include <unordered_map>
#include <filesystem>
#include <string>

Object3D assimpLoad(const std::string& path, bool flipUVCoords);
Object3D processAssimpNode(
	const aiNode* node, 
	const aiScene* scene,
	const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures);