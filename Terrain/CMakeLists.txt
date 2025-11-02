# Shader files
set( SHADER_FILES
    Shaders/Vertex.hlsl
    Shaders/Pixel.hlsl
)

source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

# Set shader properties for Visual Studio
set_source_files_properties( Shaders/Vertex.hlsl PROPERTIES 
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 5.1
    VS_SHADER_OBJECT_FILE_NAME ""  # Prevent VS from compiling to .cso
    VS_TOOL_OVERRIDE "None"         # Treat as content, not shader to compile
)
set_source_files_properties( Shaders/Pixel.hlsl PROPERTIES 
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.1
    VS_SHADER_OBJECT_FILE_NAME ""
    VS_TOOL_OVERRIDE "None"
)

# Add source to this project's executable
add_executable(Terrain "Main.cpp" "TerrainApp.h" "TerrainApp.cpp" "Include/d3dx12.h" ${SHADER_FILES})

# Include D3D12
target_link_libraries(Terrain PUBLIC d3d12.lib d3dcompiler.lib dxcompiler.lib dxgi.lib)

# Copy shader files to output directory
add_custom_command(TARGET Terrain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Vertex.hlsl
        $<TARGET_FILE_DIR:Terrain>/Shaders/Vertex.hlsl
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Pixel.hlsl
        $<TARGET_FILE_DIR:Terrain>/Shaders/Pixel.hlsl
    COMMENT "Copying shader files to output directory"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Terrain PROPERTY CXX_STANDARD 20)
endif()