// src/main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // width e o height estão sendo declarados na biblioteca importada.
    glViewport(0, 0, width, height);
}

int main()
{
    unsigned int VAO, VBO; // vao guada o estado de configuração dos atributos do vértices
    // VAO → Vertex Array Object (guarda como os dados devem ser interpretados).
    // informa tipo, bytes por linha, byte de começo

    // VBO → Vertex Buffer Object (guarda os dados dos vértices).
    // informa coordenadas
    glGenVertexArrays(1, &VAO); // VAO guarda o estado de configuração dos atributos de vértices.
    glGenBuffers(1, &VBO);      // 1 indica a quantidade

    glBindVertexArray(VAO); // essa função diz ao openGL: A partir de agora, qualquer configuração de atributos de vértice será salva nesse VAO.

    float vertices[] = {
        0.0f, 0.5f,   // topo
        -0.5f, -0.5f, // canto esquerdo
        0.5f, -0.5f   // canto direito
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Associa o buffer VBO como sendo o "buffer de vértices ativo" (GL_ARRAY_BUFFER).
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // posição dos atributos (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
        )";

    if (!glfwInit())
    {
        std::cerr << "Erro ao inicializar GLFW\n";
        return -1;
    }

    // Versão 3.3 do OpenGL (Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Janela OpenGL", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Erro ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD\n";
        return -1;
    }

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(1.2f, 0.3f, 0.4f, 0.2f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 2.3f, 0.4f, 0.2f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
