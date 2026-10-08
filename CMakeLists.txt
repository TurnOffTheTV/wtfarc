cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 23)

project(wtfarc)

file(GLOB SRCS src/*.cpp)
file(GLOB INCLUDES include/*.hpp)

add_library(wtfarc STATIC ${SRCS})
set_target_properties(wtfarc PROPERTIES PREFIX "")
target_include_directories(wtfarc PUBLIC include)

find_package(OpenSSL REQUIRED)
target_link_libraries(wtfarc PRIVATE OpenSSL::Crypto)

include(GNUInstallDirs)

install(TARGETS wtfarc DESTINATION lib/wtfarc)
install(FILES ${INCLUDES} DESTINATION include/wtfarc)