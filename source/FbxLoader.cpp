#include "model/FbxLoader.h"
#include "model/MeshGeometry.h"
#include "model/Model.h"

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

}

namespace model
{

bool FbxLoader::Load(Model& model, const std::string& filepath)
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

    // Convert mesh, NURBS and patch into triangle mesh
    FbxGeometryConverter lGeomConverter(lSdkManager);
    lGeomConverter.Triangulate(lScene, /*replace*/true);

    LoadContent(lScene->GetRootNode(), model);

    return true;
}

bool FbxLoader::LoadBlendShape(Model& model, const std::string& filepath)
{
    FbxManager* lSdkManager = NULL;
    FbxScene* lScene = NULL;

    // Prepare the FBX SDK.
    InitializeSdkObjects(lSdkManager, lScene);
    // Load the scene.

    bool lResult = LoadScene(lSdkManager, lScene, filepath.c_str());

    //FbxAxisSystem axis_sys(
    //    static_cast<FbxAxisSystem::EUpVector>(FbxAxisSystem::eYAxis),
    //    static_cast<FbxAxisSystem::EFrontVector>(-FbxAxisSystem::eParityOdd), 
    //    static_cast<FbxAxisSystem::ECoordSystem>(FbxAxisSystem::eLeftHanded)
    //);
    //auto& old_axis_sys = lScene->GetGlobalSettings().GetAxisSystem();
    //if (old_axis_sys != axis_sys) {
    //    axis_sys.ConvertScene(lScene);
    //}

    LoadNode(lScene->GetRootNode(), model);

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

bool FbxLoader::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
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

    return lStatus;
}

void FbxLoader::LoadNode(FbxNode* node, Model& model)
{
    FbxMesh* lMesh = node->GetMesh();
    if (!lMesh)
    {
        for (int i = 0, n = node->GetChildCount(); i < n; ++i) {
            LoadNode(node->GetChild(i), model);
        }
        return;
    }

    printf("node %s, mesh %s\n", node->GetName(), lMesh->GetName());

    const int lVertexCount = lMesh->GetControlPointsCount();
    const int lPolyCount = lMesh->GetPolygonCount();

    // No vertex to draw.
    if (lVertexCount == 0)
    {
        return;
    }

    MeshGeometry* dst_mesh = nullptr;
    for (auto& m : model.meshes) 
    {
        if (m->name == node->GetName() &&
            m->geometry.n_vert == lMesh->GetControlPointsCount() &&
            m->geometry.n_poly == lMesh->GetPolygonCount()) 
        {
            dst_mesh = &m->geometry;
        }
    }
    if (!dst_mesh) {
        return;
    }

//    const VBOMesh * lMeshCache = static_cast<const VBOMesh *>(lMesh->GetUserDataPtr());

    // If it has some defomer connection, update the vertices position
    const bool lHasVertexCache = lMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
		(static_cast<FbxVertexCacheDeformer*>(lMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
    const bool lHasShape = lMesh->GetShapeCount() > 0;
    const bool lHasSkin = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
    const bool lHasDeformation = lHasVertexCache || lHasShape || lHasSkin;

    FbxVector4* lVertexArray = NULL;
    if (/*!lMeshCache || */lHasDeformation)
    {
        lVertexArray = new FbxVector4[lVertexCount];
        memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
    }

    if (lHasDeformation)
    {
//        printf("node %s has deform, has vertex %d, has shape %d\n", node->GetName(), lHasVertexCache, lHasShape);
        if (lHasVertexCache)
        {
        }
        else
        {
            if (lHasShape)
            {
                const int vert_n = lMesh->GetPolygonVertexCount();
                const int cvert_n = lMesh->GetControlPointsCount();

                const int lPolygonCount = lMesh->GetPolygonCount();
                //for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++)
                //{
                //    const int lVerticeCount = lMesh->GetPolygonSize(lPolygonIndex);
                //    printf("%d\n", lVerticeCount);
                //}

                int tri_n = 0;
                int poly_n = lMesh->GetPolygonCount();
                for (int i_poly = 0; i_poly < poly_n; ++i_poly) {
                    int vert_n = lMesh->GetPolygonSize(i_poly);
                    tri_n += vert_n - 2;
                }


                FbxMesh* mesh = node->GetMesh();
                int lBlendShapeDeformerCount = mesh->GetDeformerCount(FbxDeformer::eBlendShape);
                assert(lBlendShapeDeformerCount == 1);
                for (int lBlendShapeIndex = 0; lBlendShapeIndex < lBlendShapeDeformerCount; ++lBlendShapeIndex)
                {
                    FbxBlendShape* lBlendShape = (FbxBlendShape*)mesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);

                    int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
//                    printf("lBlendShape %s, lBlendShapeChannelCount %d\n", lBlendShape->GetName(), lBlendShapeChannelCount);
                    for (int lChannelIndex = 0; lChannelIndex < lBlendShapeChannelCount; ++lChannelIndex)
                    {
                        FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
                        assert(lChannel);

                        if (lChannel)
                        {
                            //static int c = 0;
                            //printf("%d\n", c++);
                            //int zz = 0;

                            //// Get the percentage of influence on this channel.
                            //FbxAnimCurve* lFCurve = mesh->GetShapeChannel(lBlendShapeIndex, lChannelIndex, pAnimLayer);
                            //if (!lFCurve) continue;
                            //double lWeight = lFCurve->Evaluate(pTime);
                        }

                        int lTargetShapeCount = lChannel->GetTargetShapeCount();
                        assert(lTargetShapeCount == 1);
			            for (int lTargetShapeIndex = 0; lTargetShapeIndex < lTargetShapeCount; ++lTargetShapeIndex)
			            {
                            static int COUNT = 0;

                            FbxShape* lShape = lChannel->GetTargetShape(lTargetShapeIndex);

				            int j, lControlPointsCount = lShape->GetControlPointsCount();
				            FbxVector4* lControlPoints = lShape->GetControlPoints();
				            FbxLayerElementArrayTemplate<FbxVector4>* lNormals = NULL;
				            bool lStatus = lShape->GetNormals(&lNormals);

//                            printf("[%d] lShape %s, %d\n", COUNT++, (char*)lShape->GetName(), lControlPointsCount);

                            auto blendshape = std::make_unique<BlendShapeData>();
                            blendshape->name = lShape->GetName();
                            blendshape->vertices.reserve(lControlPointsCount);
				            for(j = 0; j < lControlPointsCount; j++)
				            {
                                blendshape->vertices.emplace_back(
                                    lControlPoints[j].mData[0], lControlPoints[j].mData[1], lControlPoints[j].mData[2]
                                );
                                //printf("Control Point %d", j);
                                //printf("Coordinates: %f %f %f %f",
                                //    lControlPoints[j].mData[0], lControlPoints[j].mData[1], lControlPoints[j].mData[2], lControlPoints[j].mData[3]);

					            //if (lStatus && lNormals && lNormals->GetCount() == lControlPointsCount)
					            //{
                 //                   auto& p = lNormals->GetAt(j);
                 //                   printf("Normal Vector: %f %f %f %f", p.mData[0], p.mData[1], p.mData[2], p.mData[3]);
					            //}
				            }
                            dst_mesh->blendshape_data.push_back(std::move(blendshape));
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

    for (int i = 0, n = node->GetChildCount(); i < n; ++i) {
        LoadNode(node->GetChild(i), model);
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
        printf("mesh name %s, n_pos %d, n_poly %d\n", node->GetName(), lMesh->GetControlPointsCount(), lMesh->GetPolygonCount());


    }

    FbxMesh* lMesh = (FbxMesh*)node->GetNodeAttribute();

    //int nbMetaData = lMesh->GetSrcObjectCount<FbxObjectMetaData>();
    //if (nbMetaData > 0)
    //    DisplayString("    MetaData connections ");

    //for (int i = 0; i < nbMetaData; i++)
    //{
    //    FbxObjectMetaData* metaData = lMesh->GetSrcObject<FbxObjectMetaData>(i);
    //    DisplayString("        Name: ", (char*)metaData->GetName());
    //}

    int lControlPointsCount = lMesh->GetControlPointsCount();

    FbxVector4* lControlPoints = lMesh->GetControlPoints();
    for (int i = 0; i < lControlPointsCount; i++)
    {
        // lControlPoints[i]
    }

    int lPolygonCount = lMesh->GetPolygonCount();
    //char header[100];

    //DisplayString("    Polygons");

    //int vertexId = 0;
    for (int i = 0; i < lPolygonCount; i++)
    {
        int lPolygonSize = lMesh->GetPolygonSize(i);
        if (lPolygonSize != 3) {
            int zz = 0;
        }

        for (int j = 0; j < lPolygonSize; j++)
        {
            int lControlPointIndex = lMesh->GetPolygonVertex(i, j);

        }
    }

//    printf("mesh name %s, n_pos %d, n_poly %d\n", node->GetName(), lControlPointsCount, lPolygonCount);
}

}