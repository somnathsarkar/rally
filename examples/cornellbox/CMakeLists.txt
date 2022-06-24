add_executable(cornellbox main.cc)

target_link_libraries(cornellbox PRIVATE rally)

add_custom_target(cornellbox_assets assetexporter $<TARGET_FILE_DIR:cornellbox> WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets)
add_dependencies(cornellbox cornellbox_assets)