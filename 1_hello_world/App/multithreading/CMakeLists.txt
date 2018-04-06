
file(GLOB Sources *.cpp *.cu)
file(GLOB Headers *.h )

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

cuda_add_executable(multithreading ${Sources})
target_link_libraries(multithreading gstreamer-1.0 gstapp-1.0 glib-2.0 gobject-2.0 Qt4::QtGui)

