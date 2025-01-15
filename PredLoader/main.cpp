#include <iostream>
#include "Loader.h"

int main()
{
    // Create loader instance
    Loader loader;

    // Run the loader logic
    if (!loader.Init())
    {
        std::cerr << "[Loader] Initialization failed.\n";
        return 1;
    }

    // Wait for user input
    loader.RunLoop();

    std::cout << "[Loader] Exiting...\n";
    return 0;
}
