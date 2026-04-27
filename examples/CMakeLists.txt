add_executable(mhda_basic basic.cpp)
target_link_libraries(mhda_basic PRIVATE mhda)

add_executable(mhda_by_key by_key.cpp)
target_link_libraries(mhda_by_key PRIVATE mhda)
