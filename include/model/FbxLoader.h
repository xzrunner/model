#pragma once

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
    static bool Load(Model& model, const std::string& filepath);
    static bool LoadBlendShape(Model& model, const std::string& filepath);

private:
    static void InitializeSdkObjects(fbxsdk::FbxManager*& pManager, fbxsdk::FbxScene*& pScene);
    static bool LoadScene(fbxsdk::FbxManager* pManager, fbxsdk::FbxDocument* pScene, const char* pFilename);
    static void LoadNode(fbxsdk::FbxNode* node, Model& model);

    static void LoadContent(fbxsdk::FbxNode* node, Model& model);
    static void LoadMesh(fbxsdk::FbxNode* node, Model& model);

}; // FbxLoader

}
