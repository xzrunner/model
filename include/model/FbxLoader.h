#pragma once

#include "model/Model.h"
#include "model/BlendShapeLoader.h"

#include <string>

namespace fbxsdk {
    class FbxManager;
    class FbxScene;
    class FbxDocument;
    class FbxNode;
}

namespace model
{

struct Model;

class FbxLoader
{
public:
    static bool Load(Model& model, const std::string& filepath, float scale = 1.0f);

    static bool LoadBlendShapeMeshes(std::vector<std::unique_ptr<BlendShapeLoader::MeshData>>& meshes,
        const std::string& filepath);

private:
    static void InitializeSdkObjects(fbxsdk::FbxManager*& pManager, fbxsdk::FbxScene*& pScene);
    static bool LoadScene(fbxsdk::FbxManager* pManager, fbxsdk::FbxScene* pScene, const char* pFilename);

    static void LoadBlendShapesRecursive(std::vector<std::unique_ptr<BlendShapeLoader::MeshData>>& meshes, fbxsdk::FbxNode* node);

    static void LoadContent(fbxsdk::FbxNode* node, Model& model);
    static void LoadMesh(fbxsdk::FbxNode* node, Model& model);

    static void LoadMaterialsRecursive(fbxsdk::FbxNode* node, Model& model);
    static void LoadMeshesRecursive(fbxsdk::FbxNode* node, Model& model, sm::cube& aabb, float scale = 1.0f);
    static void LoadMesh(Model::Mesh& dst, fbxsdk::FbxNode& src, sm::cube& aabb, float scale = 1.0f);
    static int LoadNodesRecursive(fbxsdk::FbxNode* node, Model& model,
        std::vector<std::unique_ptr<SkeletalAnim::Node>>& nodes, const sm::mat4& mat);

}; // FbxLoader

}
