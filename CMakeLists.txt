cmake_minimum_required(VERSION 3.16)

project(model)

add_definitions(
    -DNO_BOOST
    -DNO_QUAKE
    -DNO_MESHLAB
)

################################################################################
# Source groups
################################################################################
set(dataset
    "include/model/MeshBuider.h"
    "include/model/MeshGeometry.h"
    "include/model/Model.h"
    "include/model/ModelInstance.h"
    "source/MeshBuider.cpp"
    "source/MeshGeometry.cpp"
    "source/Model.cpp"
    "source/ModelInstance.cpp"
)
source_group("dataset" FILES ${dataset})

set(dataset__extend
    "include/model/ModelExtend.h"
    "include/model/ModelExtendType.h"
)
source_group("dataset\\extend" FILES ${dataset__extend})

set(dataset__extend__anim
    "include/model/MorphTargetAnim.h"
    "include/model/SkeletalAnim.h"
    "source/MorphTargetAnim.cpp"
    "source/SkeletalAnim.cpp"
)
source_group("dataset\\extend\\anim" FILES ${dataset__extend__anim})

set(dataset__extend__geo
    "include/model/BrushModel.h"
    "source/BrushModel.cpp"
)
source_group("dataset\\extend\\geo" FILES ${dataset__extend__geo})

set(dataset__extend__quake
    "include/model/BspModel.h"
    "include/model/QuakeMapEntity.h"
    "source/BspModel.cpp"
    "source/QuakeMapEntity.cpp"
)
source_group("dataset\\extend\\quake" FILES ${dataset__extend__quake})

set(dataset__gltf
    "include/model/gltf/Model.h"
)
source_group("dataset\\gltf" FILES ${dataset__gltf})

set(dataset__surface
    "include/model/ParametricEquations.h"
    "include/model/ParametricSurface.h"
    "include/model/Surface.h"
    "include/model/SurfaceFactory.h"
    "source/ParametricEquations.cpp"
    "source/ParametricSurface.cpp"
    "source/SurfaceFactory.cpp"
)
source_group("dataset\\surface" FILES ${dataset__surface})

set(exportor
    "include/model/AssimpExporter.h"
    "source/AssimpExporter.cpp"
)
source_group("exportor" FILES ${exportor})

set(loader
    "include/model/AssimpHelper.h"
    "include/model/BlendShapeLoader.h"
    "include/model/FbxLoader.h"
    "include/model/GltfLoader.h"
    "include/model/M3DLoader.h"
    "include/model/MaxLoader.h"
    "include/model/MaxLoader.inl"
    "include/model/ObjLoader.h"
    "include/model/SurfaceLoader.h"
    "include/model/TextureLoader.h"
    "source/AssimpHelper.cpp"
    "source/BlendShapeLoader.cpp"
    "source/FbxLoader.cpp"
    "source/GltfLoader.cpp"
    "source/M3DLoader.cpp"
    "source/MaxLoader.cpp"
    "source/ObjLoader.cpp"
    "source/SurfaceLoader.cpp"
    "source/TextureLoader.cpp"
)
source_group("loader" FILES ${loader})

set(loader__geo
    "include/model/Adjacencies.cpp"
    "include/model/Adjacencies.h"
    "include/model/BrushBuilder.h"
    "source/BrushBuilder.cpp"
)
source_group("loader\\geo" FILES ${loader__geo})

set(loader__quake
    "include/model/BspFile.h"
    "include/model/BspLoader.h"
    "include/model/MapBuilder.h"
    "include/model/MdlLoader.h"
    "source/BspLoader.cpp"
    "source/MapBuilder.cpp"
    "source/MdlLoader.cpp"
)
source_group("loader\\quake" FILES ${loader__quake})

set(loader__todo
    "include/model/SkinnedData.h"
    "include/model/SkinnedModel.h"
    "source/SkinnedData.cpp"
)
source_group("loader\\todo" FILES ${loader__todo})

set(process
    "include/model/AnimIK.h"
    "include/model/MeshIK.h"
    "source/AnimIK.cpp"
    "source/MeshIK.cpp"
)
source_group("process" FILES ${process})

set(utility
    "include/model/GlobalClock.h"
    "include/model/NormalMap.h"
    "include/model/typedef.h"
    "source/GlobalClock.cpp"
)
source_group("utility" FILES ${utility})

set(ALL_FILES
    ${dataset}
    ${dataset__extend}
    ${dataset__extend__anim}
    ${dataset__extend__geo}
    ${dataset__extend__quake}
    ${dataset__gltf}
    ${dataset__surface}
    ${exportor}
    ${loader}
    ${loader__geo}
    ${loader__quake}
    ${loader__todo}
    ${process}
    ${utility}
)

################################################################################
# Target
################################################################################

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE include external/sm external/unirender/include external/rapidxml external/guard/include external/polymesh3/include external/halfedge/include external/assimp/include external/fbxsdk/include external/tinygltf external/tinyobjloader external/gimg)
