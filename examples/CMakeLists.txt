# CMakeLists.txt
# adding the demos
foreach(dir demo1 demo2 demo3 demo4 demo5 hun-garr v12demo v14demo xmldemo barebones physicsdemo)
    add_subdirectory(src/${dir})
endforeach()
