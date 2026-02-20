cmake_minimum_required(VERSION 3.16)

project(Trie LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Trie main.cpp words.cpp)

include(GNUInstallDirs)
install(TARGETS Trie
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
