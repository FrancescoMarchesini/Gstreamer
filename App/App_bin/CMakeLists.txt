cmake_minimum_required(VERSION 3.5)
file(GLOB AppSources *.cpp)
file(GLOB AppIncludes *.h )

cuda_add_executable(2_App_bin ${AppSources})
target_link_libraries(2_App_bin gstreamer-1.0 gstapp-1.0 glib-2.0 gobject-2.0 nvcaffe_parser nvinfer)
