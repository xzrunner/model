#include "model/FbxLoader.h"
#include "model/MeshGeometry.h"
#include "model/Model.h"
#include "model/typedef.h"

#include <unirender2/Device.h>
#include <unirender2/VertexArray.h>
#include <unirender2/IndexBuffer.h>
#include <unirender2/VertexBuffer.h>
#include <unirender2/VertexBufferAttribute.h>

#include <fbxsdk.h>

#include <assert.h>

namespace
{

void DisplayString(const char* pHeader, const char* pValue  = "" , const char* pSuffix  = "" )
{
    FbxString lString;

    lString = pHeader;
    lString += pValue;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}

sm::mat4 trans_fbx_mat(const fbxsdk::FbxAMatrix& fbx_mat)
{
    sm::mat4 mat;
    auto& src = fbx_mat.Double44();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            mat.c[i][j] = static_cast<float>(src[i][j]);
        }
    }

    //sm::vec3 trans, rotate, scale;
    //mat.Decompose(trans, rotate, scale);

    //sm::vec3 new_trans(-trans.z, -trans.y, trans.x);
    //sm::vec3 new_rotate(rotate.z, -rotate.y, -rotate.x);
    //sm::vec3 new_scale(scale);
    ////mat = sm::mat4::Translated(trans.x, trans.y, trans.z)
    ////    * sm::mat4::Rotated(new_rotate.x, new_rotate.y, new_rotate.z)
    ////    * sm::mat4::Scaled(new_scale.x, new_scale.y, new_scale.z);
    //mat = sm::mat4::Scaled(new_scale.x, new_scale.y, new_scale.z)
    //    * sm::mat4::Rotated(new_rotate.x, new_rotate.y, new_rotate.z)
    //    * sm::mat4::Translated(new_trans.x, new_trans.y, new_trans.z);

    //auto rot_mat = sm::mat4::Rotated(rotate.x, rotate.y, rotate.z);
    //rot_mat *= sm::mat4::RotatedY(180);

    //sm::vec3 x = mat2 * sm::vec3(1, 0, 0);
    //sm::vec3 y = mat2 * sm::vec3(0, 1, 0);
    //sm::vec3 z = mat2 * sm::vec3(0, 0, 1);
    //printf("x %f %f %f, y %f %f %f, z %f %f %f\n",
    //    x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z);
    //sm::mat4 test;
    //test = mat.RotatedY(180);

    //auto trans = /*mat2 * */sm::vec3(mat.x[12], mat.x[13], mat.x[14]);
    //mat2.Translate(trans.x, trans.y, trans.z);


    //mat2.x[12] =  mat.x[14];
    //mat2.x[13] = -mat.x[13];
    //mat2.x[14] = -mat.x[12];

    //sm::vec3 x = mat2 * sm::vec3(1, 0, 0);
    //sm::vec3 y = mat2 * sm::vec3(0, 1, 0);
    //sm::vec3 z = mat2 * sm::vec3(0, 0, 1);
    //printf("x %f %f %f, y %f %f %f, z %f %f %f\n",
    //    x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z);

    return mat;
}

struct SubMesh
{
//    ofxFBXMeshMaterial* materialPtr;
    bool bRender;
    int triangleCount = 0;
    int indexOffset = 0;
    int totalIndices = 0;
};

}

namespace model
{

bool FbxLoader::Load(const ur2::Device& dev, Model& model,
                     const std::string& filepath, float scale)
{
    FbxManager* lSdkManager = NULL;
    FbxScene* lScene = NULL;

    // Prepare the FBX SDK.
    InitializeSdkObjects(lSdkManager, lScene);
    // Load the scene.

    bool lResult = LoadScene(lSdkManager, lScene, filepath.c_str());
    if (!lResult) {
        return false;
    }

    // material
    LoadMaterialsRecursive(lScene->GetRootNode(), model);

    // mesh
    sm::cube aabb;
    LoadMeshesRecursive(dev, lScene->GetRootNode(), model, aabb, scale);

    model.aabb.Combine(aabb);

//    LoadContent(lScene->GetRootNode(), model);

    // only meshes
    if (lScene->GetRootNode()->GetChildCount() == 0)
    {

    }
    // skeletal anim
    else
    {
        auto ext = std::make_unique<SkeletalAnim>();

        // nodes
        std::vector<std::unique_ptr<SkeletalAnim::Node>> nodes;
        LoadNodesRecursive(lScene->GetRootNode(), model, nodes, sm::mat4());
        ext->SetNodes(nodes);

        // bones
		auto& sk_nodes = ext->GetNodes();
		for (auto& mesh : model.meshes) {
			for (auto& bone : mesh->geometry.bones) {
				bone.node = -1;
				for (int i = 0, n = sk_nodes.size(); i < n; ++i) {
					if (sk_nodes[i]->name == bone.name) {
						bone.node = i;
						break;
					}
				}
			}
		}

        // todo animation

        model.ext = std::move(ext);
    }

    model.scale = scale;

    return true;
}

bool FbxLoader::LoadBlendShapeMeshes(std::vector<std::unique_ptr<BlendShapeLoader::MeshData>>& meshes, const std::string& filepath)
{
    FbxManager* lSdkManager = NULL;
    FbxScene* lScene = NULL;

    // Prepare the FBX SDK.
    InitializeSdkObjects(lSdkManager, lScene);
    // Load the scene.

    bool lResult = LoadScene(lSdkManager, lScene, filepath.c_str());
    if (!lResult) {
        return false;
    }

    LoadBlendShapesRecursive(meshes, lScene->GetRootNode());

    return true;
}

void FbxLoader::InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
    //The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
    pManager = FbxManager::Create();
    if( !pManager )
    {
        FBXSDK_printf("Error: Unable to create FBX Manager!\n");
        exit(1);
    }
	else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());

    //Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create(pManager, "My Scene");
	if( !pScene )
    {
        FBXSDK_printf("Error: Unable to create FBX scene!\n");
        exit(1);
    }
}

bool FbxLoader::LoadScene(FbxManager* pManager, FbxScene* pScene, const char* pFilename)
{
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor,  lSDKMinor,  lSDKRevision;
    //int lFileFormat = -1;
    int i, lAnimStackCount;
    bool lStatus;
    char lPassword[1024];

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pManager,"");

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if( !lImportStatus )
    {
        FbxString error = lImporter->GetStatus().GetErrorString();
        FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
        FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

        if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
            FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
            FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
        }

        return false;
    }

    FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

    if (lImporter->IsFBX())
    {
        FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

        // From this point, it is possible to access animation stack information without
        // the expense of loading the entire file.

        FBXSDK_printf("Animation Stack Information\n");

        lAnimStackCount = lImporter->GetAnimStackCount();

        FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
        FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
        FBXSDK_printf("\n");

        for(i = 0; i < lAnimStackCount; i++)
        {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

            FBXSDK_printf("    Animation Stack %d\n", i);
            FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
            FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

            // Change the value of the import name if the animation stack should be imported
            // under a different name.
            FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

            // Set the value of the import state to false if the animation stack should be not
            // be imported.
            FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
            FBXSDK_printf("\n");
        }

        // Set the import states. By default, the import states are always set to
        // true. The code below shows how to change these states.
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL,        true);
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE,         true);
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK,            true);
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE,           true);
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_GOBO,            true);
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_ANIMATION,       true);
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    }

    // Import the scene.
    lStatus = lImporter->Import(pScene);

    if(lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
    {
        FBXSDK_printf("Please enter password: ");

        lPassword[0] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
        scanf("%s", lPassword);
        FBXSDK_CRT_SECURE_NO_WARNING_END

        FbxString lString(lPassword);

        pManager->GetIOSettings()->SetStringProp(IMP_FBX_PASSWORD,      lString);
        pManager->GetIOSettings()->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

        lStatus = lImporter->Import(pScene);

        if(lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
        {
            FBXSDK_printf("\nPassword is wrong, import aborted.\n");
        }
    }

    // Destroy the importer.
    lImporter->Destroy();

    // Convert Axis System to what is used in this example, if needed
    FbxAxisSystem SceneAxisSystem = pScene->GetGlobalSettings().GetAxisSystem();

    int front_sign, up_sign;
    auto font = SceneAxisSystem.GetFrontVector(front_sign);
    auto up = SceneAxisSystem.GetUpVector(up_sign);
    auto sys = SceneAxisSystem.GetCoorSystem();

    //FbxAxisSystem OurAxisSystem(
    //    FbxAxisSystem::EUpVector(font * front_sign),
    //    FbxAxisSystem::EFrontVector(up * up_sign),
    //    FbxAxisSystem::eLeftHanded
    //);

    FbxAxisSystem OurAxisSystem( FbxAxisSystem::OpenGL);
    if( SceneAxisSystem != OurAxisSystem ) {
        OurAxisSystem.ConvertScene(pScene);
    }

    //for (int i = -3; i <= 3; ++i)
    //{
    //    if (i == 0) {
    //        continue;
    //    }
    //    for (int j = -2; j <= 2; ++j)
    //    {
    //        if (j == 0) {
    //            continue;
    //        }
    //        for (int k = 0; k <= 1; ++k)
    //        {
    //            //FbxAxisSystem OurAxisSystem(FbxAxisSystem::OpenGL);
    //            fbxsdk::FbxAxisSystem OurAxisSystem2(
    //                fbxsdk::FbxAxisSystem::EUpVector(i),
    //                fbxsdk::FbxAxisSystem::EFrontVector(j),
    //                k == 0 ? FbxAxisSystem::eLeftHanded : FbxAxisSystem::eRightHanded
    //            );
    //            OurAxisSystem2.ConvertScene(pScene);

    //            FbxMesh* lMesh = pScene->GetRootNode()->GetChild(0)->GetMesh();
    //            FbxSkin* lSkinDeformer = (FbxSkin *)lMesh->GetDeformer(0, FbxDeformer::eSkin);
    //            FbxCluster* src = lSkinDeformer->GetCluster(0);
    //            fbxsdk::FbxAMatrix transformLinkMatrix;
    //            src->GetTransformLinkMatrix(transformLinkMatrix);
    //            auto mat = trans_fbx_mat(transformLinkMatrix);
    //            int zz = 0;

    //            printf("%d %d %d, %f %f %f, %f %f %f\n", i, j, k,
    //                mat.c[0][0], mat.c[0][1], mat.c[0][2],
    //                mat.c[3][0], mat.c[3][1], mat.c[3][2]);
    //        }
    //    }
    //}



    //FbxAxisSystem lAxisSytemReference = pScene->GetGlobalSettings().GetAxisSystem();

    //int lUpVectorSign = 1;
    //int lFrontVectorSign = 1;

    ////get upVector and its sign.
    //FbxAxisSystem::EUpVector lUpVector = lAxisSytemReference.GetUpVector(lUpVectorSign);

    ////get FrontVector and its sign.
    //FbxAxisSystem::EFrontVector lFrontVector = lAxisSytemReference.GetFrontVector(lFrontVectorSign);

    ////get uCoorSystem.
    //FbxAxisSystem::ECoordSystem lCoorSystem = lAxisSytemReference.GetCoorSystem();

    ////The FbxAxisSystem object to reconstruct back by saved parameter
    //FbxAxisSystem lAxisSytemReconstruct(
    //    FbxAxisSystem::EUpVector(lUpVectorSign * lUpVector),
    //    FbxAxisSystem::EFrontVector(lFrontVectorSign * lFrontVector),
    //    lCoorSystem);

    // Convert Unit System to what is used in this example, if needed
    FbxSystemUnit SceneSystemUnit = pScene->GetGlobalSettings().GetSystemUnit();
    if( SceneSystemUnit.GetScaleFactor() != 1.0 ) {
        //The unit in this example is centimeter.
        FbxSystemUnit::cm.ConvertScene(pScene);
    }

//    FbxGeometryConverter lGeomConverter( lSdkManager );
//    lGeomConverter.Triangulate(lScene, /*replace*/true);

    // Split meshes per material, so that we only have one material per mesh (for VBO support)
//    if(!lGeomConverter.SplitMeshesPerMaterial(lScene, /*replace*/true)) {
//        cout << "There was an error splitting the meshes per material" << endl;
//    }
//if(!lGeomConverter.SplitMeshesPerMaterial(lScene, /*replace*/true)) {
//    int zz = 0;
//}

    // cache all of the textures by loading them from disk //
//    if(_settings.importTextures) {
//        cacheTexturesInScene( lScene );
//    }

    // cache all of the materials in the scene as ofxFBXMeshMaterial //
//    if(_settings.importMaterials) {
//        cacheMaterialsRecursive( lScene->GetRootNode() );
//    }

    return lStatus;
}

void FbxLoader::LoadBlendShapesRecursive(std::vector<std::unique_ptr<BlendShapeLoader::MeshData>>& meshes, fbxsdk::FbxNode* node)
{
    FbxMesh* lMesh = node->GetMesh();
    if (!lMesh)
    {
        for (int i = 0, n = node->GetChildCount(); i < n; ++i) {
            LoadBlendShapesRecursive(meshes, node->GetChild(i));
        }
        return;
    }

    const int lVertexCount = lMesh->GetControlPointsCount();
    const int lPolyCount = lMesh->GetPolygonCount();
    // No vertex to draw.
    if (lVertexCount == 0)
    {
        return;
    }

    std::vector<std::unique_ptr<BlendShapeLoader::VertBuf>> blendshapes;

    // If it has some defomer connection, update the vertices position
    const bool lHasVertexCache = lMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
		(static_cast<FbxVertexCacheDeformer*>(lMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
    const bool lHasShape = lMesh->GetShapeCount() > 0;
    const bool lHasSkin = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
    const bool lHasDeformation = lHasVertexCache || lHasShape || lHasSkin;
    if (lHasDeformation)
    {
        printf("node %s has deform, has vertex %d, has shape %d\n", node->GetName(), lHasVertexCache, lHasShape);
        if (lHasVertexCache)
        {
        }
        else
        {
            if (/*true || */lHasShape)
            {
                const int vert_n = lMesh->GetPolygonVertexCount();
                const int cvert_n = lMesh->GetControlPointsCount();

                FbxMesh* mesh = node->GetMesh();
                int lBlendShapeDeformerCount = mesh->GetDeformerCount(FbxDeformer::eBlendShape);
                assert(lBlendShapeDeformerCount == 1);
                for (int lBlendShapeIndex = 0; lBlendShapeIndex < lBlendShapeDeformerCount; ++lBlendShapeIndex)
                {
                    FbxBlendShape* lBlendShape = (FbxBlendShape*)mesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);
                    int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
                    for (int lChannelIndex = 0; lChannelIndex < lBlendShapeChannelCount; ++lChannelIndex)
                    {
                        FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
                        assert(lChannel);
                        int lTargetShapeCount = lChannel->GetTargetShapeCount();
                        assert(lTargetShapeCount == 1);
			            for (int lTargetShapeIndex = 0; lTargetShapeIndex < lTargetShapeCount; ++lTargetShapeIndex)
			            {
                            FbxShape* lShape = lChannel->GetTargetShape(lTargetShapeIndex);

				            int j, lControlPointsCount = lShape->GetControlPointsCount();
				            FbxVector4* lControlPoints = lShape->GetControlPoints();

                            auto blendshape = std::make_unique<BlendShapeLoader::VertBuf>();
                            blendshape->name = lShape->GetName();
                            blendshape->verts.reserve(lControlPointsCount);
				            for(j = 0; j < lControlPointsCount; j++)
				            {
                                blendshape->verts.emplace_back(
                                    (float)lControlPoints[j].mData[0],
                                    (float)lControlPoints[j].mData[1],
                                    -(float)lControlPoints[j].mData[2]
                                );
				            }
                            if (!blendshape->verts.empty()) {
                                blendshapes.push_back(std::move(blendshape));
                            }
			            }
                    }
                }
            }
            else
            {
//                printf("not has shape\n");
            }
        }
    }
    else
    {
//        printf("node %s no deform\n", node->GetName());
    }

    if (!blendshapes.empty())
    {
        auto mesh = std::make_unique<BlendShapeLoader::MeshData>();

        mesh->mesh = std::make_unique<BlendShapeLoader::VertBuf>();
        mesh->mesh->name = node->GetName();

        int controlPointCount = lMesh->GetControlPointsCount();
        mesh->mesh->verts.resize(controlPointCount);
        const FbxVector4 * lControlPoints = lMesh->GetControlPoints();
        int lPolygonCount = lMesh->GetPolygonCount();
        for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++) {
            int faceSize = lMesh->GetPolygonSize(lPolygonIndex);
            for (int lVerticeIndex = 0; lVerticeIndex < faceSize; lVerticeIndex++) {
                const int lControlPointIndex = lMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
                mesh->mesh->verts[lControlPointIndex] = sm::vec3(
                    static_cast<float>(lControlPoints[lControlPointIndex][0]),
                    static_cast<float>(lControlPoints[lControlPointIndex][1]),
                    -static_cast<float>(lControlPoints[lControlPointIndex][2])
                );
            }
        }

        mesh->blendshapes = std::move(blendshapes);

        meshes.push_back(std::move(mesh));
    }

    for (int i = 0, n = node->GetChildCount(); i < n; ++i) {
        LoadBlendShapesRecursive(meshes, node->GetChild(i));
    }
}

void FbxLoader::LoadContent(fbxsdk::FbxNode* node, Model& model)
{
    if (node->GetNodeAttribute() == NULL)
    {
        FBXSDK_printf("NULL Node Attribute\n\n");
    }
    else
    {
        //int n = node->GetNodeAttributeCount();
        //for (int i = 0; i < n; ++i)
        //{
        //    auto attr = node->GetNodeAttributeByIndex(i);
        //    if (attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
        //        int zz = 0;
        //    }
        //}

        FbxNodeAttribute::EType lAttributeType = (node->GetNodeAttribute()->GetAttributeType());

        switch (lAttributeType)
        {
        default:
            break;
        case FbxNodeAttribute::eMarker:
 //           DisplayMarker(node);
            break;

        case FbxNodeAttribute::eSkeleton:
//            DisplaySkeleton(node);
            break;

        case FbxNodeAttribute::eMesh:
            LoadMesh(node, model);
            break;

        case FbxNodeAttribute::eNurbs:
 //           DisplayNurb(node);
            break;

        case FbxNodeAttribute::ePatch:
 //           DisplayPatch(node);
            break;

        case FbxNodeAttribute::eCamera:
 //           DisplayCamera(node);
            break;

        case FbxNodeAttribute::eLight:
 //           DisplayLight(node);
            break;

        case FbxNodeAttribute::eLODGroup:
 //           DisplayLodGroup(node);
            break;
        }
    }

    for (int i = 0, n = node->GetChildCount(); i < n; ++i) {
        LoadContent(node->GetChild(i), model);
    }
}

void FbxLoader::LoadMesh(fbxsdk::FbxNode* node, Model& model)
{
    int n = node->GetNodeAttributeCount();
    for (int i = 0; i < n; ++i)
    {
        auto attr = node->GetNodeAttributeByIndex(i);
        FbxMesh* lMesh = (FbxMesh*)attr;
//        printf("mesh name %s, n_pos %d, n_poly %d\n", node->GetName(), lMesh->GetControlPointsCount(), lMesh->GetPolygonCount());
    }

    FbxMesh* lMesh = (FbxMesh*)node->GetNodeAttribute();

    int lControlPointsCount = lMesh->GetControlPointsCount();

    FbxVector4* lControlPoints = lMesh->GetControlPoints();
    for (int i = 0; i < lControlPointsCount; i++)
    {
        // lControlPoints[i]
    }

    int lPolygonCount = lMesh->GetPolygonCount();
    for (int i = 0; i < lPolygonCount; i++)
    {
        int lPolygonSize = lMesh->GetPolygonSize(i);
        assert(lPolygonSize == 3);
        for (int j = 0; j < lPolygonSize; j++)
        {
            int lControlPointIndex = lMesh->GetPolygonVertex(i, j);

        }
    }

//    printf("mesh name %s, n_pos %d, n_poly %d\n", node->GetName(), lControlPointsCount, lPolygonCount);
}

void FbxLoader::LoadMaterialsRecursive(fbxsdk::FbxNode* node, Model& model)
{
    // Bake material and hook as user data.
    const int lMaterialCount = node->GetMaterialCount();
    for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex) {
        FbxSurfaceMaterial * lMaterial = node->GetMaterial(lMaterialIndex);
        if (lMaterial && !lMaterial->GetUserDataPtr()) {
            model.materials.emplace_back(std::make_unique<Model::Material>());
        }
    }

    const int lChildCount = node->GetChildCount();
    for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex) {
        LoadMaterialsRecursive(node->GetChild(lChildIndex), model);
    }
}

void FbxLoader::LoadMeshesRecursive(const ur2::Device& dev, fbxsdk::FbxNode* pNode,
                                    Model& model, sm::cube& aabb, float scale)
{
    FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
    if (lNodeAttribute)
    {
        auto type = lNodeAttribute->GetAttributeType();
        if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh || lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNurbs || lNodeAttribute->GetAttributeType() == FbxNodeAttribute::ePatch || lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNurbsSurface)
        {
            //FbxGeometryConverter lGeomConverter(lSdkManager);
            //lGeomConverter.Triangulate(pNode->GetMesh(), true);
            if (pNode->GetMesh())
            {
                auto mesh = std::make_unique<Model::Mesh>();
                LoadMesh(dev, *mesh, *pNode, aabb, scale);
                if (!pNode->GetUserDataPtr()) {
                    pNode->SetUserDataPtr(mesh.get());
                }
                model.meshes.push_back(std::move(mesh));
            }
        }
    }

    const int lChildCount = pNode->GetChildCount();
    for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex) {
        LoadMeshesRecursive(dev, pNode->GetChild(lChildIndex), model, aabb, scale);
    }
}

void FbxLoader::LoadMesh(const ur2::Device& dev, Model::Mesh& dst,
                         fbxsdk::FbxNode& src, sm::cube& aabb, float scale)
{
    dst.name = src.GetName();
    FbxMesh* lMesh = src.GetMesh();
    if (!lMesh->GetNode()) {
        return;
    }

    // todo
    dst.material = 0;

    std::vector<SubMesh> subMeshes;

    const int lPolygonCount = lMesh->GetPolygonCount();

    auto n_vert = lMesh->GetControlPointsCount();

    // Count the polygon count of each material
    FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
    FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
    if (lMesh->GetElementMaterial()) {
        lMaterialIndice = &lMesh->GetElementMaterial()->GetIndexArray();
        lMaterialMappingMode = lMesh->GetElementMaterial()->GetMappingMode();
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon) {
            FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
            if (lMaterialIndice->GetCount() == lPolygonCount) {
                // make sure the vector is setup and we have the proper amount of materials ready //
                for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex) {
                    const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
                    if(lMaterialIndex >= 0) {
                        if(static_cast<int>(subMeshes.size()) < lMaterialIndex + 1) {
                            subMeshes.resize(lMaterialIndex + 1);
                        }
                    }
                }
            }
        }
    }

    if (subMeshes.empty()) {
        subMeshes.resize(1);
    }

    bool has_mat_tex = false;
    //if (mesh->material >= 0 && mesh->material < static_cast<int>(materials.size())) {
    //    if (materials[mesh->material]->diffuse_tex >= 0) {
    //        has_mat_tex = true;
    //    }
    //}

    int floats_per_vertex = 3;

    bool has_normal = lMesh->GetElementNormalCount() > 0 && lMesh->GetElementNormal();
    if (has_normal) {
        floats_per_vertex += 3;
    }

    bool has_texcoord = lMesh->GetElementUVCount();
    if (has_texcoord) {
        floats_per_vertex += 2;
    }

    const bool has_color = lMesh->GetElementVertexColorCount() > 0 && lMesh->GetElementVertexColor();
    if (has_color) {
        floats_per_vertex += 1;
    }

    bool has_skinned = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
	if (has_skinned) {
		floats_per_vertex += 2;
	}

    int controlPointCount = lMesh->GetControlPointsCount();

    std::vector<sm::vec3> vertices;
    vertices.resize(controlPointCount);
    std::vector<unsigned short> indices;

    const FbxVector4 * lControlPoints = lMesh->GetControlPoints();
    for( int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++ ) {
        int faceSize = lMesh->GetPolygonSize( lPolygonIndex );
        for( int lVerticeIndex = 0; lVerticeIndex < faceSize; lVerticeIndex++ ) {
            const int lControlPointIndex = lMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
            vertices[ lControlPointIndex ] = sm::vec3(
                static_cast<float>(lControlPoints[lControlPointIndex][0]),
                static_cast<float>(lControlPoints[lControlPointIndex][1]),
                static_cast<float>(lControlPoints[lControlPointIndex][2])
            );
            indices.push_back( lControlPointIndex );
        }
    }

    std::vector<sm::vec3> normals;
    if (has_normal)
    {
        const FbxGeometryElementNormal * lNormalElement     = lMesh->GetElementNormal(0);
        auto mNormalMappingMode = lNormalElement->GetMappingMode();

        normals.resize( controlPointCount );

        FbxVector4 lCurrentNormal;
        if( mNormalMappingMode == FbxGeometryElement::eByControlPoint ) {
            for(int i = 0; i < controlPointCount; i++ ) {
                int lNormalIndex = i;
                if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
                    lNormalIndex = lNormalElement->GetIndexArray().GetAt(i);
                }
                lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
                normals[i] = sm::vec3(
                    static_cast<float>(lCurrentNormal[0]),
                    static_cast<float>(lCurrentNormal[1]),
                    static_cast<float>(lCurrentNormal[2])
                );
            }
        } else if( mNormalMappingMode == FbxGeometryElement::eByPolygonVertex ) {
            for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex) {
                int faceSize = lMesh->GetPolygonSize( lPolygonIndex );
                for( int lVerticeIndex = 0; lVerticeIndex < faceSize; lVerticeIndex++ ) {
                    const int lControlPointIndex = lMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
                    if(lMesh->GetPolygonVertexNormal( lPolygonIndex, lVerticeIndex, lCurrentNormal )) {
                        normals[ lControlPointIndex ] = sm::vec3(
                            static_cast<float>(lCurrentNormal[0]),
                            static_cast<float>(lCurrentNormal[1]),
                            static_cast<float>(lCurrentNormal[2])
                        );
                    }
                }
            }
        } else {
            //ofLogVerbose() << "ofxFBXMesh :: clearing normals, only eByControlPoint and eByPolygonVertex supported.";
            //mNormalMappingMode = FbxGeometryElement::eNone;
            //mesh.clearNormals();
            assert(0);
        }
    }

    std::vector<sm::vec2> texcoords;
    if (has_texcoord)
    {
        texcoords.resize(controlPointCount);

        for (int lUVSetIndex = 0; lUVSetIndex < lMesh->GetElementUVCount(); lUVSetIndex++) {
            const FbxGeometryElementUV* lUVElement = lMesh->GetElementUV( lUVSetIndex );

            if(!lUVElement)
                continue;

            FbxGeometryElement::EMappingMode lUVMappingMode = lUVElement->GetMappingMode();
            // only support mapping mode eByPolygonVertex and eByControlPoint
            if(lUVElement->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
               lUVElement->GetMappingMode() != FbxGeometryElement::eByControlPoint )
                continue;

            //index array, where holds the index referenced to the uv data
            const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
            const int lIndexCount= (lUseIndex) ? lUVElement->GetIndexArray().GetCount() : 0;

            //iterating through the data by polygon
            const int lPolyCount = lMesh->GetPolygonCount();

            if( lUVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint ) {
                for( int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex ) {
                    // build the max index array that we need to pass into MakePoly
                    const int lPolySize = lMesh->GetPolygonSize(lPolyIndex);
                    for( int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex ) {
                        FbxVector2 lUVValue;
                        //get the index of the current vertex in control points array
                        int lPolyVertIndex = lMesh->GetPolygonVertex(lPolyIndex,lVertIndex);
                        //the UV index depends on the reference mode
                        int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;
                        lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);
                        texcoords[ lPolyVertIndex ] = sm::vec2(
                            static_cast<float>(lUVValue[0]),
                            static_cast<float>(lUVValue[1])
                        );
                    }
                }
            } else if (lUVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
                int lIndexByPolygonVertex = 0;
                FbxVector2 lUVValue;
                //Let's get normals of each polygon, since the mapping mode of normal element is by polygon-vertex.
                for(int lPolygonIndex = 0; lPolygonIndex < lMesh->GetPolygonCount(); lPolygonIndex++) {
                    //get polygon size, you know how many vertices in current polygon.
                    int lPolygonSize = lMesh->GetPolygonSize(lPolygonIndex);
                    //retrieve each vertex of current polygon.
                    for(int i = 0; i < lPolygonSize; i++) {
                        int lNormalIndex = 0;
                        //reference mode is direct, the normal index is same as lIndexByPolygonVertex.
                        if( lUVElement->GetReferenceMode() == FbxGeometryElement::eDirect )
                            lNormalIndex = lIndexByPolygonVertex;

                        //reference mode is index-to-direct, get normals by the index-to-direct
                        if( lUVElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                            lNormalIndex = lUVElement->GetIndexArray().GetAt(lIndexByPolygonVertex);

                        //Got normals of each polygon-vertex.
                        lUVValue = lUVElement->GetDirectArray().GetAt(lNormalIndex);
                        const int lControlPointIndex = lMesh->GetPolygonVertex( lPolygonIndex, i );
                        texcoords[ lControlPointIndex ] = sm::vec2(
                            static_cast<float>(lUVValue[0]),
                            static_cast<float>(lUVValue[1])
                        );

                        lIndexByPolygonVertex++;
                    }//end for i //lPolygonSize
                }//end for lPolygonIndex //PolygonCount
            }
        }
    }

    std::vector<uint32_t> colors;
    if (has_color)
    {
        colors.resize(controlPointCount);

        const FbxLayerElementVertexColor* pVertexColorElement   = lMesh->GetElementVertexColor();
        FbxGeometryElement::EMappingMode lVertexColorMappingMode = pVertexColorElement->GetMappingMode();

        FbxColor lVertexColor;
        if( lVertexColorMappingMode == FbxGeometryElement::eByControlPoint ) {
            int lVertexColorIndex = 0;
            for(int i = 0; i < controlPointCount; i++ ) {
                lVertexColorIndex = i;
                if ( pVertexColorElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
                    lVertexColorIndex = pVertexColorElement->GetIndexArray().GetAt(i);
                }
                lVertexColor = pVertexColorElement->GetDirectArray().GetAt(lVertexColorIndex);
                colors[i] =
                    (uint8_t)(lVertexColor.mAlpha * 255 + 0.5f) << 24 |
                    (uint8_t)(lVertexColor.mBlue  * 255 + 0.5f) << 16 |
                    (uint8_t)(lVertexColor.mGreen * 255 + 0.5f) <<  8 |
                    (uint8_t)(lVertexColor.mRed   * 255 + 0.5f);
            }
        } else {
            assert(0);
//            ofLogVerbose() << "ofxFBXMesh :: clearing vertex colors, only eByControlPoint supported.";
//            lVertexColorMappingMode = FbxGeometryElement::eNone;
        }
    }

    std::vector<std::vector<std::pair<int, float>>> weights_per_vertex;
    if (has_skinned)
    {
        weights_per_vertex.resize(controlPointCount);

        FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)lMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

        FbxSkin * lSkinDeformer = (FbxSkin *)lMesh->GetDeformer(0, FbxDeformer::eSkin);
        FbxSkin::EType lSkinningType = lSkinDeformer->GetSkinningType();
        assert(lSkinningType == FbxSkin::eLinear || lSkinningType == FbxSkin::eRigid);

        int lSkinCount = lMesh->GetDeformerCount(FbxDeformer::eSkin);

        for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex) {
            FbxSkin * lSkinDeformer = (FbxSkin *)lMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);

            int lClusterCount = lSkinDeformer->GetClusterCount();
            for (int lClusterIndex = 0; lClusterIndex < lClusterCount; ++lClusterIndex) {
                FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);

//                printf("bone %s\n", lCluster->GetLink()->GetName());

                if (!lCluster->GetLink())
                    continue;

                int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
                for (int k = 0; k < lVertexIndexCount; ++k) {
                    int lIndex = lCluster->GetControlPointIndices()[k];
                    double lWeight = lCluster->GetControlPointWeights()[k];
                    weights_per_vertex[lIndex].push_back({ lClusterIndex, static_cast<float>(lWeight) });
                }//For each vertex
            }//lClusterCount
        }
    }

	uint8_t* buf = new uint8_t[controlPointCount * floats_per_vertex * sizeof(float)];
	uint8_t* ptr = buf;
	for (int i = 0; i < controlPointCount; ++i)
	{
		auto& p = vertices[i];

		sm::vec3 p_trans(p.x, p.y, p.z);
		p_trans *= scale;
		memcpy(ptr, &p_trans.x, sizeof(float) * 3);
		ptr += sizeof(float) * 3;
		aabb.Combine(p_trans);

		if (has_normal)
		{
			auto& nor = normals[i];
			memcpy(ptr, &nor.x, sizeof(float) * 3);
			ptr += sizeof(float) * 3;
		}
		if (has_texcoord)
		{
			auto& t = texcoords[i];
            float x = t.x;
            if (x > 1) {
                x -= std::floor(x);
            }
			memcpy(ptr, &x, sizeof(float));
			ptr += sizeof(float);
			float y = 1 - t.y;
            if (y > 1) {
                y -= std::floor(y);
            }
			memcpy(ptr, &y, sizeof(float));
			ptr += sizeof(float);
		}
		if (has_color)
		{
			memcpy(ptr, &colors[i], sizeof(uint32_t));
			ptr += sizeof(uint32_t);
		}
		if (has_skinned)
		{
			assert(weights_per_vertex[i].size() <= 4);
			unsigned char indices[4] = { 0, 0, 0, 0 };
			unsigned char weights[4] = { 0, 0, 0, 0 };
			for (int j = 0, m = weights_per_vertex[i].size(); j < m; ++j)
			{
				indices[j] = weights_per_vertex[i][j].first;
				weights[j] = static_cast<unsigned char>(weights_per_vertex[i][j].second * 255.0f);
			}
			uint32_t indices_pack = (indices[3] << 24) | (indices[2] << 16) | (indices[1] << 8) | indices[0];
			uint32_t weights_pack = (weights[3] << 24) | (weights[2] << 16) | (weights[1] << 8) | weights[0];
			memcpy(ptr, &indices_pack, sizeof(float));
			ptr += sizeof(uint32_t);
			memcpy(ptr, &weights_pack, sizeof(float));
			ptr += sizeof(uint32_t);
		}
	}

    auto va = dev.CreateVertexArray();

    auto ibuf_sz = sizeof(uint16_t) * indices.size();
    auto ibuf = dev.CreateIndexBuffer(ur2::BufferUsageHint::StaticDraw, ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    va->SetIndexBuffer(ibuf);

    auto vbuf_sz = sizeof(float) * floats_per_vertex * controlPointCount;
    auto vbuf = dev.CreateVertexBuffer(ur2::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(buf, vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur2::VertexBufferAttribute>> vbuf_attrs;

	int stride = 0;
	// pos
	stride += 4 * 3;
	// normal
	if (has_normal) {
		stride += 4 * 3;
	}
	// texcoord
	if (has_texcoord) {
		stride += 4 * 2;
	}
	// color
	if (has_color) {
		stride += 4;
	}
	// skinned
	if (has_skinned) {
		stride += 4 + 4;
	}

	int offset = 0;
	// pos
    vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, offset, stride));
	offset += 4 * 3;
	// normal
	if (has_normal)
	{
		dst.geometry.vertex_type |= VERTEX_FLAG_NORMALS;
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::Float, 3, offset, stride));
		offset += 4 * 3;
	}
	// texcoord
	if (has_texcoord)
	{
		dst.geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::Float, 2, offset, stride));
		offset += 4 * 2;
	}
	// color
	if (has_color)
	{
		dst.geometry.vertex_type |= VERTEX_FLAG_COLOR;
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::Byte, 4, offset, stride));
		offset += 4;
	}
	// skinned
	if (has_skinned)
	{
		dst.geometry.vertex_type |= VERTEX_FLAG_SKINNED;
        // blend_indices
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::Byte, 4, offset, stride));
		offset += 4;
        // blend_weights
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::Byte, 4, offset, stride));
		offset += 4;
	}

    va->SetVertexBufferAttrs(vbuf_attrs);

    // update the meshes offsets and indices counts //
    for( size_t i = 0; i < subMeshes.size(); i++ ) {
        subMeshes[i].triangleCount = 0;
        subMeshes[i].totalIndices = 0;
    }

    // update the sub meshes with the proper amount of indices //
    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex) {
        // The material for current face.
        int lMaterialIndex = 0;
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon) {
            lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
            if(lMaterialIndex < 0) {
                lMaterialIndex = 0;
            }
        }

        subMeshes[lMaterialIndex].triangleCount += 1;
        subMeshes[lMaterialIndex].totalIndices += lMesh->GetPolygonSize( lPolygonIndex );
    }

    int toffset = 0;
    for( size_t i = 0; i < subMeshes.size(); i++ ) {
        subMeshes[i].indexOffset = toffset;
        toffset += subMeshes[i].totalIndices;
    }

    dst.geometry.vertex_array = va;
    for (auto& sub : subMeshes)
    {
        dst.geometry.sub_geometries.push_back(
            SubmeshGeometry(true, /*sub.totalIndices*/lPolygonCount * 3, sub.indexOffset)
        );
        dst.geometry.sub_geometry_materials.push_back(0);
    }
//	dst.geometry.sub_geometries.insert({ "default", SubmeshGeometry(vi.in, 0) });
//	dst.geometry.sub_geometries.push_back(SubmeshGeometry(true, vi.in, 0));

    if (has_skinned)
    {
        FbxSkin* lSkinDeformer = (FbxSkin *)lMesh->GetDeformer(0, FbxDeformer::eSkin);
        int lClusterCount = lSkinDeformer->GetClusterCount();

	    dst.geometry.bones.reserve(lClusterCount);
	    for (int i = 0; i < lClusterCount; ++i)
	    {
            FbxCluster* src = lSkinDeformer->GetCluster(i);
            assert(src->GetLink());

		    model::Bone b_dst;
            b_dst.node = -1;
            b_dst.name = src->GetLink()->GetName();

            fbxsdk::FbxAMatrix transformLinkMatrix;
//            src->GetTransformAssociateModelMatrix(transformLinkMatrix);
//            src->GetTransformLinkMatrix(transformLinkMatrix);
            b_dst.offset_trans = trans_fbx_mat(transformLinkMatrix);

		    dst.geometry.bones.push_back(b_dst);
	    }
    }

//	delete[] buf;
    dst.geometry.n_vert = controlPointCount;
    dst.geometry.n_poly = lPolygonCount;
    dst.geometry.vert_stride = stride;
    dst.geometry.vert_buf = buf;

	//if (raw_data) {
	//	dst.geometry.raw_data = LoadMeshRawData(ai_mesh, scale);
	//}
}

int FbxLoader::LoadNodesRecursive(fbxsdk::FbxNode* fbx_node, Model& model,
                                  std::vector<std::unique_ptr<SkeletalAnim::Node>>& nodes,
                                  const sm::mat4& mat)
{
    auto node = std::make_unique<SkeletalAnim::Node>();
    auto node_raw = node.get();

    node_raw->name = fbx_node->GetName();

    int node_id = nodes.size();
    nodes.push_back(std::move(node));

    //auto& local_trans = fbx_node->EvaluateLocalTransform();
    //node_raw->local_trans = trans_fbx_mat(local_trans);

    //const FbxVector4 lT = fbx_node->GetGeometricTranslation(FbxNode::eSourcePivot);
    //const FbxVector4 lR = fbx_node->GetGeometricRotation(FbxNode::eSourcePivot);
    //const FbxVector4 lS = fbx_node->GetGeometricScaling(FbxNode::eSourcePivot);
    //node_raw->local_trans = trans_fbx_mat({ lT, lR, lS });

    auto child_mat = mat * node_raw->local_trans;   // mat mul

    if (fbx_node->GetChildCount() > 0)
    {
		for (size_t i = 0, n = fbx_node->GetChildCount(); i < n; ++i)
		{
			int child = LoadNodesRecursive(fbx_node->GetChild(i), model, nodes, child_mat);
			node_raw->children.push_back(child);

			auto node = nodes[child].get();
			assert(node->parent == -1);
			node->parent = node_id;
		}
    }
    else
    {
        int idx = -1;
        auto ptr = fbx_node->GetUserDataPtr();
        if (ptr) {
            for (int i = 0, n = model.meshes.size(); i < n; ++i) {
                if (model.meshes[i].get() == ptr) {
                    idx = i;
                    break;
                }
            }
//            printf("mesh node %s\n", fbx_node->GetName());
            assert(idx >= 0);
            node_raw->meshes.push_back(idx);
        }

//		node_raw->meshes.reserve(fbx_node->getmesh);
//		for (size_t i = 0; i < ai_node->mNumMeshes; ++i)
//		{
//			auto mesh = ai_node->mMeshes[i];
//			node_raw->meshes.push_back(mesh);
//            if (model.meshes[mesh]->name.empty()) {
//                model.meshes[mesh]->name = ai_node->mName.C_Str();
//            } else {
//                if (model.meshes[mesh]->name != ai_node->mName.C_Str()) {
//                    assert(0);
//                }
//            }
////			CombineAABB(model.aabb, meshes_aabb[mesh], child_mat);
//		}
    }

    return node_id;
}

}