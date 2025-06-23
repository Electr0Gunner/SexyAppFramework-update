# CMakeLists.txt
# adding the demos
foreach(dir v14demo barebones physicsdemo tetris)
    add_subdirectory(src/${dir})
endforeach()
