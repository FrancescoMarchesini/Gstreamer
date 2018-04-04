
file(GLOB Sources *.cpp *.cu)
file(GLOB Headers *.h )

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

cuda_add_executable(dynamic_pipeline ${Sources})
target_link_libraries(dynamic_pipeline gstreamer-1.0 gstapp-1.0 glib-2.0 gobject-2.0 Qt4::QtGui)

