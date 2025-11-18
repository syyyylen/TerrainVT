# Shader files
set( SHADER_FILES
    Shaders/Vertex.hlsl
    Shaders/Pixel.hlsl
    Shaders/Hull.hlsl
    Shaders/Domain.hlsl
    Shaders/HeightMapBakingCompute.hlsl
    Shaders/PerlinNoise.hlsli
    Shaders/PixelVT.hlsl
)

source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

# Add source to this project's executable
add_executable(Terrain 
"Main.cpp" 
"Core.h"
"TerrainApp.h" "TerrainApp.cpp" 
"Include/d3dx12.h" 
${SHADER_FILES} 
"Camera.cpp" "Camera.h" 
"Texture.h" "Texture.cpp"
"ShaderCompiler.h" "ShaderCompiler.cpp"
"VTex.h" "VTex.cpp"
"Include/stb/stb.cpp" "Include/stb/stb_image.h" "Include/stb/stb_image_write.h" 
"Include/ImGui/imconfig.h" "Include/ImGui/imgui.cpp" "Include/ImGui/imgui.h" 
"Include/ImGui/ImGuizmo.cpp" "Include/ImGui/ImGuizmo.h" "Include/ImGui/imgui_demo.cpp" 
"Include/ImGui/imgui_draw.cpp" "Include/ImGui/imgui_impl_dx12.cpp" "Include/ImGui/imgui_impl_dx12.h" 
"Include/ImGui/imgui_impl_win32.cpp" "Include/ImGui/imgui_impl_win32.h" "Include/ImGui/imgui_internal.h" 
"Include/ImGui/imgui_tables.cpp" "Include/ImGui/imgui_widgets.cpp" "Include/ImGui/imstb_rectpack.h" 
"Include/ImGui/imstb_textedit.h" "Include/ImGui/imstb_truetype.h" 
"Include/Optick/optick.config.h" "Include/Optick/optick.h" "Include/Optick/optick_capi.cpp" 
"Include/Optick/optick_capi.h" "Include/Optick/optick_common.h" "Include/Optick/optick_core.cpp" 
"Include/Optick/optick_core.freebsd.h" "Include/Optick/optick_core.h" "Include/Optick/optick_core.linux.h" 
"Include/Optick/optick_core.macos.h" "Include/Optick/optick_core.platform.h" "Include/Optick/optick_core.win.h" 
"Include/Optick/optick_gpu.cpp" "Include/Optick/optick_gpu.d3d12.cpp" "Include/Optick/optick_gpu.h" 
"Include/Optick/optick_memory.h" "Include/Optick/optick_message.cpp" 
"Include/Optick/optick_message.h" "Include/Optick/optick_miniz.cpp" "Include/Optick/optick_miniz.h" 
"Include/Optick/optick_serialization.cpp" "Include/Optick/optick_serialization.h" 
"Include/Optick/optick_server.cpp" "Include/Optick/optick_server.h")

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

# Copy DXC runtime DLLs (from local Bin/ folder) to the build output
add_custom_command(TARGET Terrain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Bin/dxcompiler.dll
        $<TARGET_FILE_DIR:Terrain>/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Bin/dxil.dll
        $<TARGET_FILE_DIR:Terrain>/dxil.dll
    COMMENT "Copying DXC runtime DLLs (dxcompiler.dll, dxil.dll) to output directory"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Terrain PROPERTY CXX_STANDARD 20)
endif()