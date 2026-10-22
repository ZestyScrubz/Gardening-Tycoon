set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(gardening_tycoon_app
    main.cpp
    FarmController.h
    FarmController.cpp
    MainWindow.h
    MainWindow.cpp
)

target_link_libraries(gardening_tycoon_app PRIVATE core Qt6::Widgets)