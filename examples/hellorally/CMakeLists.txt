add_executable(hellorally main.cc helloscript.cc)

target_link_libraries(hellorally PRIVATE rally)

add_custom_target(hellorally_assets assetexporter $<TARGET_FILE_DIR:hellorally> WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets)
add_dependencies(hellorally hellorally_assets)