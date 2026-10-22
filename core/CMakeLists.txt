add_library(core STATIC
    src/Soil.cpp
    src/IGrowthPolicy.cpp
    src/Crop.cpp
    src/CropFactory.cpp
    src/Tile.cpp
    src/Farm.cpp
    src/FarmGridBuilder.cpp
    src/IBiomePolicy.cpp
    src/WeatherModel.cpp
    src/Hazard.cpp
    src/Simulation.cpp
    src/Treasury.cpp
    src/Inventory.cpp
    src/IPricingStrategy.cpp
    src/Market.cpp
    src/JsonSaveStore.cpp
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)