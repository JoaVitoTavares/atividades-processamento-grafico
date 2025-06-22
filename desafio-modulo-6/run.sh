#!/bin/bash

g++ src/IsometricTilemap.cpp src/glad.c -Iinclude -lglfw -ldl -lGL -o main

if [ $? -eq 0 ]; then
    echo "Executando..."
    cp -r src/resources .
    ./main
else
    echo "Erro na compilação."
fi


