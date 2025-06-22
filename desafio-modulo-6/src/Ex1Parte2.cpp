// Ex1Parte2.cpp
// PARTE 2 - Exercício 3: Instanciação dinâmica de triângulos com matriz de transformação
// Utiliza um único VAO para um triângulo padrão e cria novos triângulos via clique do mouse,
// com cores aleatórias.

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM para transformações
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

// Defina as dimensões da janela
const GLuint WIDTH = 800;
const GLuint HEIGHT = 600;

// Estrutura que define um triângulo instanciado via mouse
struct Triangle {
    glm::vec2 position;  // Posição (x, y)
    glm::vec3 color;     // Cor (RGB)
};

// Vetor global para armazenar os triângulos criados
vector<Triangle> triangleInstances;

// Identificador do VAO único para o triângulo padrão
GLuint defaultTriangleVAO = 0;

// Janela global (para usar no callback)
GLFWwindow* window = nullptr;

// Função que cria o VAO para o triângulo padrão
// Vértices: v0 = (-0.1, -0.1), v1 = (0.1, -0.1), v2 = (0.0, 0.1)
GLuint createDefaultTriangle()
{
    // Define os vértices do triângulo padrão; z = 0.
    GLfloat vertices[] = {
        -0.1f, -0.1f, 0.0f,
         0.1f, -0.1f, 0.0f,
         0.0f,  0.1f, 0.0f
    };
    
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // O shader espera que o atributo "position" esteja na localização 0.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return VAO;
}

// Shaders (GLSL 330 core para OpenGL 3.3+)
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main()
{
    gl_Position = projection * model * vec4(position, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 fragColor;
uniform vec4 inputColor;
void main()
{
    fragColor = inputColor;
}
)";

// Função que compila e cria o shader program
GLuint setupShaderProgram()
{
    GLint success;
    GLchar infoLog[512];
    
    // Compila o Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cout << "ERROR::VERTEX_SHADER_COMPILATION_FAILED\n" << infoLog << endl;
    }
    
    // Compila o Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cout << "ERROR::FRAGMENT_SHADER_COMPILATION_FAILED\n" << infoLog << endl;
    }
    
    // Cria o Shader Program e linka os shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cout << "ERROR::SHADER_PROGRAM_LINKING_FAILED\n" << infoLog << endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

// Callback do mouse: cria um novo triângulo na posição do clique com cor aleatória.
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // Converte as coordenadas de clique:
        int winWidth, winHeight;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        ypos = winHeight - ypos; // Inverte o y, pois GLFW entrega com origem no topo
        
        // Cria uma instância de Triangle com posição e cor aleatória
        Triangle tri;
        tri.position = glm::vec2(xpos, ypos);
        float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        tri.color = glm::vec3(r, g, b);
        
        triangleInstances.push_back(tri);
        
        // (Opcional) Imprime as coordenadas para depuração
        cout << "Clique em: " << xpos << ", " << ypos << endl;
    }
}

// Callback de teclado: fecha a janela ao pressionar ESC.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // Inicializa GLFW
    if (!glfwInit()) {
        cout << "Falha ao inicializar GLFW" << endl;
        return -1;
    }
    // Configura GLFW para OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Cria a janela
    window = glfwCreateWindow(WIDTH, HEIGHT, "Exercício 3 - Transformações com GLM", nullptr, nullptr);
    if (!window) {
        cout << "Falha ao criar a janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Registra os callbacks
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    
    // Inicializa o GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }
    
    glViewport(0, 0, WIDTH, HEIGHT);
    
    // Compila o shader program
    GLuint shaderProgram = setupShaderProgram();
    
    // Cria o VAO único para o triângulo padrão
    defaultTriangleVAO = createDefaultTriangle();
    
    // Configura a projeção ortográfica: (0,0) no canto inferior esquerdo e (WIDTH, HEIGHT) no canto superior direito
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT, -1.0f, 1.0f);
    glUseProgram(shaderProgram);
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Define a semente para geração de cores aleatórias
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Loop de renderização
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Para cada triângulo instanciado via mouse, aplica a transformação e desenha
        for (const Triangle& tri : triangleInstances)
        {
            // Constrói a matriz de modelo: traduza o triângulo padrão para a posição desejada
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(tri.position, 0.0f));

            model = glm::scale(model, glm::vec3(300.0f, 300.0f, 1.0f));
            
            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            GLint colorLoc = glGetUniformLocation(shaderProgram, "inputColor");
            glUniform4f(colorLoc, tri.color.r, tri.color.g, tri.color.b, 1.0f);
            
            // Desenha o triângulo padrão usando o VAO único
            glBindVertexArray(defaultTriangleVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindVertexArray(0);
        
        glfwSwapBuffers(window);
    }
    
    glfwTerminate();
    return 0;
}
