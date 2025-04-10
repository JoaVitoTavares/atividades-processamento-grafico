# run.sh
#!/bin/bash

g++ src/main.cpp src/glad.c -Iinclude -lglfw -ldl -lGL -o main

if [ $? -eq 0 ]; then
    echo "Executando..."
    ./main
else
    echo "Erro na compilação."
fi
