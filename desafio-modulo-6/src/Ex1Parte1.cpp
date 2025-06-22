// Ex1Parte1.cpp
// Este arquivo implementa os exercícios propostos para a PARTE 1,
// onde:
// - Exercício 1: Cria uma função "createTriangle" que gera um VAO contendo 
//   a geometria de um triângulo com os vértices passados por parâmetro.
// - Exercício 2: Instancia 5 triângulos na tela, armazenando os VAOs em um vector
//   e desenhando-os no loop de renderização.

#include <iostream>
#include <vector>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

using namespace std;

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex e Fragment shaders mínimos (versão 330 core)
const GLchar* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 position;
    void main()
    {
        gl_Position = vec4(position, 1.0);
    }
)";

const GLchar* fragmentShaderSource = R"(
    #version 330 core
    out vec4 color;
    void main()
    {
        color = vec4(0.2, 0.6, 1.0, 1.0); // cor fixa azulada
    }
)";

// Função para compilar os shaders e criar o shader program
GLuint setupShaderProgram()
{
    GLint success;
    GLchar infoLog[512];

    // Compila o Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::VERTEX_SHADER_COMPILATION_FAILED\n" << infoLog << endl;
    }

    // Compila o Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::FRAGMENT_SHADER_COMPILATION_FAILED\n" << infoLog << endl;
    }

    // Cria o shader program e linka os shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER_PROGRAM_LINKING_FAILED\n" << infoLog << endl;
    }

    // Deleta os shaders já linkados
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Exercício 1 - Função que cria um triângulo e retorna seu VAO
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
    // Array de vértices: 3 vértices com 3 componentes (x, y, z), aqui definimos z = 0
    GLfloat vertices[] = {
        x0, y0, 0.0f,  // v0
        x1, y1, 0.0f,  // v1
        x2, y2, 0.0f   // v2
    };

    GLuint VAO, VBO;
    
    // Gera o VAO e o VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    // Vincula o VAO
    glBindVertexArray(VAO);
    
    // Vincula o VBO, envia os dados e configura os atributos
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Configura o atributo de vértice (localização 0): 3 componentes, do tipo float, sem normalização
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    
    // Desvincula o VBO e o VAO para garantir que não haja alterações acidentais
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

int main()
{
    // Inicializa a GLFW
    if (!glfwInit()) {
        cout << "Erro ao inicializar GLFW" << endl;
        return -1;
    }
    // Define as configurações da GLFW para usar OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // Opcional
    
    // Cria uma janela
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Exercícios com OpenGL 3.3+ - Parte 1", nullptr, nullptr);
    if (!window) {
        cout << "Erro ao criar a janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Inicializa o GLAD para carregar funções do OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }
    
    // Define as dimensões da viewport
    glViewport(0, 0, WIDTH, HEIGHT);
    
    // Compila o shader program
    GLuint shaderProgram = setupShaderProgram();
    
    // Exercício 2: Instancie 5 triângulos na tela usando a função createTriangle
    // Armazene os VAOs em um vector
    vector<GLuint> triangleVAOs;
    
    // Exemplos de coordenadas para 5 triângulos (coordenadas em NDC, variando de -1 a 1)
    triangleVAOs.push_back( createTriangle(-0.8f, -0.8f, -0.6f, -0.2f, -0.4f, -0.8f) );
    triangleVAOs.push_back( createTriangle(-0.2f, -0.8f,  0.0f, -0.2f,  0.2f, -0.8f) );
    triangleVAOs.push_back( createTriangle( 0.4f, -0.8f,  0.6f, -0.2f,  0.8f, -0.8f) );
    triangleVAOs.push_back( createTriangle(-0.8f,  0.2f, -0.6f,  0.8f, -0.4f,  0.2f) );
    triangleVAOs.push_back( createTriangle( 0.4f,  0.2f,  0.6f,  0.8f,  0.8f,  0.2f) );
    
    // Use o shader program
    glUseProgram(shaderProgram);
    
    // Loop principal
    while (!glfwWindowShouldClose(window))
    {
        // Processa eventos
        glfwPollEvents();
        
        // Limpa a tela com uma cor de fundo
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Para cada triângulo, vincule o VAO e desenhe
        for (GLuint vao : triangleVAOs) {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindVertexArray(0);
        
        // Troca os buffers para exibir o desenho
        glfwSwapBuffers(window);
    }
    
    // Finaliza a GLFW
    glfwTerminate();
    return 0;
}
