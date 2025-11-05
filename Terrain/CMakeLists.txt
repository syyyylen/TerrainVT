# Shader files
set( SHADER_FILES
    Shaders/Vertex.hlsl
    Shaders/Pixel.hlsl
    Shaders/Hull.hlsl
    Shaders/Domain.hlsl
)

source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

# Set shader properties for Visual Studio
set_source_files_properties( Shaders/Vertex.hlsl PROPERTIES 
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 5.1
    VS_SHADER_OBJECT_FILE_NAME ""
    VS_TOOL_OVERRIDE "None"
)
set_source_files_properties( Shaders/Pixel.hlsl PROPERTIES 
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.1
    VS_SHADER_OBJECT_FILE_NAME ""
    VS_TOOL_OVERRIDE "None"
)

set_source_files_properties( Shaders/Hull.hlsl PROPERTIES 
    VS_SHADER_TYPE Hull
    VS_SHADER_MODEL 5.1
    VS_SHADER_OBJECT_FILE_NAME ""
    VS_TOOL_OVERRIDE "None"
)

set_source_files_properties( Shaders/Domain.hlsl PROPERTIES 
    VS_SHADER_TYPE Domain
    VS_SHADER_MODEL 5.1
    VS_SHADER_OBJECT_FILE_NAME ""
    VS_TOOL_OVERRIDE "None"
)

# Add source to this project's executable
add_executable(Terrain 
"Main.cpp" 
"TerrainApp.h" "TerrainApp.cpp" 
"Include/d3dx12.h" 
${SHADER_FILES} 
"Camera.cpp" "Camera.h" 
"Include/stb/stb.cpp" "Include/stb/stb_image.h" "Include/stb/stb_image_write.h" 
"Include/ImGui/imconfig.h" "Include/ImGui/imgui.cpp" "Include/ImGui/imgui.h" 
"Include/ImGui/ImGuizmo.cpp" "Include/ImGui/ImGuizmo.h" "Include/ImGui/imgui_demo.cpp" 
"Include/ImGui/imgui_draw.cpp" "Include/ImGui/imgui_impl_dx12.cpp" "Include/ImGui/imgui_impl_dx12.h" 
"Include/ImGui/imgui_impl_win32.cpp" "Include/ImGui/imgui_impl_win32.h" "Include/ImGui/imgui_internal.h" 
"Include/ImGui/imgui_tables.cpp" "Include/ImGui/imgui_widgets.cpp" "Include/ImGui/imstb_rectpack.h" 
"Include/ImGui/imstb_textedit.h" "Include/ImGui/imstb_truetype.h")

# Include D3D12
target_link_libraries(Terrain PUBLIC d3d12.lib d3dcompiler.lib dxcompiler.lib dxgi.lib)

# Copy shader files to output directory
add_custom_command(TARGET Terrain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
        $<TARGET_FILE_DIR:Terrain>/Shaders
    COMMENT "Copying shader files to output directory"
)

# Copy assets to output directory
add_custom_command(TARGET Terrain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:Terrain>/Assets
    COMMENT "Copying assets to output directory"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Terrain PROPERTY CXX_STANDARD 20)
endif()