# ${CMAKE_CURRENT_SOURCE_DIR}表示当前运行的CMakeLists.txt所在的路径
# cmake最低版本号要求
cmake_minimum_required(VERSION 3.10)

# 项目名称变量PROJECT_NAME，也是输出可文件的名称 
set(PNAME CM5)

# 设置可执行文件输出路径变量EXECUTABLE_OUTPUT_PATH，变量名随意  
set(OUTPUT_DIR 
${CMAKE_CURRENT_SOURCE_DIR}/Bin)

# 设置源文件名变量SRC_LIST，变量名随意 
set(SRC_LIST 
${CMAKE_CURRENT_SOURCE_DIR}/Code/StartUp/main.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Code/FUN/FUN_StringConv.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Code/FUN/ClassTest.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Code/FUN/FUN_Boost.cpp)



# 设置头文件路径名变量INC_DIR，变量名随意  
set(INC_DIR 
${CMAKE_CURRENT_SOURCE_DIR}/Code/DRI
${CMAKE_CURRENT_SOURCE_DIR}/Code/FUN
${CMAKE_CURRENT_SOURCE_DIR}/Code/StartUp
# /usr/local/include)
H:/ThirdC++Lib/boost_1_84_0)

# 设置库文件路径名变量LIB_DIR，变量名随意，目前库文件目录为空
set(LIB_DIR)

# 设置库文件名变量LIB_LIST，变量名随意，目前库文件名为空
set(LIB_LIST
# -lboost_system
# -lboost_thread
# -liconv_hook
# -lpthread)
-lWs2_32)

# 设置编译参数变量PPARA，变量名随意
set(PPARA
-Wall
-g)


# 为C++源文件设置C++标准  ---> C++14 
set(CMAKE_CXX_STANDARD 14)  
# 如果设置为 ON，CMake会在编译器不支持指定标准时发出错误
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

# 为C源文件设置C标准  ---> C11
set(CMAKE_C_STANDARD 11)  
# 如果设置为 ON，CMake会在编译器不支持指定标准时发出错误
set(CMAKE_C_STANDARD_REQUIRED ON)  


# 设置C编译器路径   
# set(CMAKE_C_COMPILER "H:/MinGW/mingw64_posix/bin/gcc.exe")    
# 设置C++编译器路径  
# set(CMAKE_CXX_COMPILER "H:/MinGW/mingw64_posix/bin/g++.exe") 



















#--------------------- 生成 ------------------------#
# 项目名称
project(${PNAME})

# 生成可执行文件
add_executable(${PNAME} ${SRC_LIST})

# 生成动态库文件
#add_library(${PNAME} SHARED ${SRC_LIST})

# 生成静态库文件
#add_library(${PNAME} STATIC ${SRC_LIST})

#--------------------- 添加编译选项 ------------------------#
# 添加头文件路径 
target_include_directories(${PNAME} PRIVATE ${INC_DIR})

# 添加库文件路径
target_link_directories(${PNAME} PRIVATE ${LIB_DIR})

# 添加编译参数  
target_compile_options(${PNAME} PRIVATE ${PPARA})

# 添加可执行文件输出目录
set_target_properties(${PNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

#---------------------- 动态链接库文件 ------------------------#
# 动态链接库文件
target_link_libraries(${PNAME} ${LIB_LIST})

#---------------------- 静态链接库文件 ------------------------# 
# 设置链接类型为静态  
# set_target_properties(${PNAME} PROPERTIES LINK_LIBRARY_DIRS "${LIB_DIR}")  # 指定库文件所在目录  
# set_target_properties(${PNAME} PROPERTIES LINK_FLAGS "-static")  # 使用静态链接
# 静态链接文件
# target_link_libraries(${PNAME} ${LIB_LIST})
