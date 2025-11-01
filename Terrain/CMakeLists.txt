# Add source to this project's executable.
add_executable (Terrain "Main.cpp" "TerrainApp.h" "TerrainApp.cpp" "D3D12Renderer.h" "D3D12Renderer.cpp" "Include/d3dx12.h")

# Include D3D12
target_link_libraries(Terrain PUBLIC d3d12.lib d3dcompiler.lib dxcompiler.lib dxgi.lib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Terrain PROPERTY CXX_STANDARD 20)
endif()