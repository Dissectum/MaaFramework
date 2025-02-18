file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp ../include)

add_library(MaaToolKit SHARED ${maa_toolkit_src})

file(GLOB MaaToolKit_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaToolKit/*.h)
set(MaaToolKit_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaToolKit)

target_include_directories(MaaToolKit INTERFACE ../../include PRIVATE . ../include ../../include)
set_target_properties(MaaToolKit PROPERTIES
    PUBLIC_HEADER "${MaaToolKit_PUBLIC_HEADERS}"
)
target_compile_definitions(MaaToolKit PRIVATE MAA_TOOLKIT_EXPORTS)

target_link_libraries(MaaToolKit MaaFramework MaaUtils MaaControlUnit HeaderOnlyLibraries Boost::system Boost::url)

add_dependencies(MaaToolKit MaaFramework MaaUtils MaaControlUnit)

install(TARGETS MaaToolKit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    PUBLIC_HEADER DESTINATION include/MaaToolKit
    ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_toolkit_src})
