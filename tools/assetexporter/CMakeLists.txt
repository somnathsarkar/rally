add_executable(assetexporter main.cc)
target_link_libraries(assetexporter PRIVATE assimp rally)
target_include_directories(assetexporter PRIVATE "../../external/assimp/include")