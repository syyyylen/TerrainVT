#include "TerrainApp.h"

int main()
{
    std::cout << "Hello There !" << std::endl;

    {
        TerrainApp app;
        app.Run();
    }

    std::cout << "Exiting Terrain App" << std::endl;

    return 0;
}
