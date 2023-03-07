set(MBGL_WITH_QT ON CACHE BOOL "Build Maplibre Qt version" FORCE)
set(MBGL_QT_INSIDE_PLUGIN ON CACHE BOOL "Build all libs as OBJECT libraries." FORCE)
add_subdirectory(
    ../../../3rdparty/maplibre-gl-native # Source directory
    ../../../3rdparty/maplibre-gl-native # Binary directory
    EXCLUDE_FROM_ALL
)
if(IOS)
    target_compile_options(mbgl-core PUBLIC -fembed-bitcode)
    target_link_libraries(mbgl-core PUBLIC -fembed-bitcode)
    target_compile_options(mbgl-vendor-parsedate PUBLIC -fembed-bitcode)
    target_link_libraries(mbgl-vendor-parsedate PUBLIC -fembed-bitcode)
    target_compile_options(mbgl-vendor-nunicode PUBLIC -fembed-bitcode)
    target_link_libraries(mbgl-vendor-nunicode PUBLIC -fembed-bitcode)
    target_compile_options(mbgl-vendor-csscolorparser PUBLIC -fembed-bitcode)
    target_link_libraries(mbgl-vendor-csscolorparser PUBLIC -fembed-bitcode)
elseif(MSVC)
    target_compile_options(mbgl-core PUBLIC "/EHsc")
    target_compile_definitions(mbgl-core PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    target_compile_definitions(qmaplibregl PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()


qt_internal_add_plugin(QGeoServiceProviderFactoryMaplibreGLPlugin
    OUTPUT_NAME qtgeoservices_maplibregl
    CLASS_NAME QGeoServiceProviderFactoryMaplibreGL
    PLUGIN_TYPE geoservices
    SOURCES
        qgeoserviceproviderpluginmaplibregl.cpp qgeoserviceproviderpluginmaplibregl.h
        qgeomappingmanagerenginemaplibregl.cpp qgeomappingmanagerenginemaplibregl.h
        qgeomapmaplibregl.cpp qgeomapmaplibregl.h qgeomapmaplibregl_p.h
        qmaplibreglstylechange.cpp qmaplibreglstylechange_p.h
        qsgmaplibreglnode.cpp qsgmaplibreglnode.h
    INCLUDE_DIRECTORIES
        ../../../3rdparty/maplibre-gl-native/platform/qt/include
    DEFINES
        QT_MAPBOXGL_STATIC
    LIBRARIES
        Qt::OpenGL
        Qt::Network
        Qt::LocationPrivate
        "$<BUILD_INTERFACE:qmaplibregl>"
        "$<BUILD_INTERFACE:mbgl-core>"
        "$<BUILD_INTERFACE:mbgl-vendor-parsedate>"
        "$<BUILD_INTERFACE:mbgl-vendor-nunicode>"
        "$<BUILD_INTERFACE:mbgl-vendor-csscolorparser>"
)

qt_add_resources(plugin_resource_files maplibregl.qrc)

qt_internal_extend_target(QGeoServiceProviderFactoryMaplibreGLPlugin
    SOURCES
        ${plugin_resource_files}
)
