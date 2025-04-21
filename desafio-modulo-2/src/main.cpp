// src/main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // width e height são passados automaticamente pela GLFW quando a janela for redimensionada
    glViewport(0, 0, width, height); // Atualiza a área visível (viewport) com o novo tamanho
}

GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
    unsigned int VAO, VBO;      // vao guarda o estado de configuração dos atributos dos vértices
    glGenVertexArrays(1, &VAO); // Cria um VAO
    glGenBuffers(1, &VBO);      // Cria um VBO
                                // VAO → Vertex Array Object (guarda como os dados devem ser interpretados).
    // informa tipo, bytes por linha, byte de começo

    // VBO → Vertex Buffer Object (guarda os dados dos vértices).
    // informa coordenadas
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Ativa o VBO como buffer de vértices atual

    glBindVertexArray(VAO); // A partir de agora, qualquer configuração de atributos será associada a esse VAO

    float vertices[] = {
        x0, y0, // topo
        x1, y1, // canto esquerdo
        x2, y2, // canto direito
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // Os dados do array "vertices" são copiados para o buffer da GPU (ligados ao VBO)
    // posição dos atributos (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0); // Ativa o atributo de vértice 0
    // Explica ao OpenGL como ler os dados do buffer: location 0, dois floats por vértice, tipo float, sem normalizar, espaçamento 2 floats, sem offset
    glEnableVertexAttribArray(0); // Ativa o atributo de vértice 0

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Desativa o VBO (boa prática)
    glBindVertexArray(0);             // Desativa o VAO (boa prática)
    return VAO;
}

int main()
{
    // Inicialização da biblioteca GLFW
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

    glfwMakeContextCurrent(window);                                    // Ativa o contexto OpenGL
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // Registra callback para redimensionamento

    // Carrega funções OpenGL com GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD\n";
        return -1;
    }

    // Vertex shader → define as posições dos vértices

    GLuint vao = createTriangle(0.0, 0.5, -0.5, -0.5, 0.5, -0.5);

    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0); // z = 0.0, w = 1.0
        }
    )";

    // Fragment shader → define a cor do triângulo
    const char *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.3, 1.0, 0.2, 1.0); // verde florescente
        }
    )";

    // Compila e linka os shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    // reloca um shader já criado a um espaço na memoria de um endereçamento..
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram(); // Programa de shader unificado
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram); // Liga os shaders num programa

    glDeleteShader(vertexShader);   // Shader compilado já pode ser deletado
    glDeleteShader(fragmentShader); // idem

    // Loop principal de renderização
    while (!glfwWindowShouldClose(window))
    {
        // cor de fundo (fundo da janela)
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f); // azul-acinzentado
        glClear(GL_COLOR_BUFFER_BIT);         // limpa a tela com a cor acima

        glUseProgram(shaderProgram);      // Usa os shaders compilados
        glBindVertexArray(vao);           // Ativa os dados do triângulo
        glDrawArrays(GL_TRIANGLES, 0, 3); // Desenha 3 vértices (1 triângulo)

        glfwSwapBuffers(window); // Atualiza a janela com o conteúdo renderizado
        glfwPollEvents();        // Processa eventos de teclado, mouse etc.
    }

    // Libera recursos e fecha janela
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
