cmake_minimum_required(VERSION 3.10)
project(MeuProjetoOpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Incluir headers do glad
include_directories(include)

# Adiciona os arquivos fonte
add_executable(main
        src/main.cpp
        src/glad.c
        )

# Linka com as bibliotecas do sistema
target_link_libraries(main
        glfw
        GL
        dl
        )
