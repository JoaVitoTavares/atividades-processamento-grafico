// GameColorMatch.cpp
// Jogo de “Color Match”: o usuário clica em um retângulo para escolher sua cor,
// e todos os retângulos cuja cor seja similar (distância Euclidiana em RGB ≤ limiar)
// são removidos. Cada clique conta como uma tentativa; pontos = número de retângulos removidos.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <random>
#include <iostream>

// --- Configurações da janela e da grade ---
const int WINDOW_W = 800;
const int WINDOW_H = 600;
const int COLS     = 8;
const int ROWS     = 6;
const float RECT_W = WINDOW_W  / float(COLS);
const float RECT_H = WINDOW_H / float(ROWS);

// --- Parâmetros do jogo ---
const int   MAX_ATTEMPTS    = 10;
const float COLOR_THRESHOLD = 0.25f; // Distância máxima em RGB para considerar “similar”

struct Rect {
    glm::vec2 pos;
    glm::vec3 color;
    bool      alive = true;
};

std::vector<Rect> grid;
int score    = 0;
int attempts = 0;

// Gera cores aleatórias e inicializa a grade
void initGrid() {
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(0.0f,1.0f);

    grid.clear();
    for(int y=0; y<ROWS; ++y) {
        for(int x=0; x<COLS; ++x) {
            Rect r;
            r.pos   = { x*RECT_W, y*RECT_H };
            r.color = { dist(rng), dist(rng), dist(rng) };
            r.alive = true;
            grid.push_back(r);
        }
    }
    score = 0;
    attempts = 0;
}

// Distância Euclidiana entre cores
float colorDistance(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(a - b);
}

// Shaders GLSL 330 core
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(position, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 fragColor;
uniform vec4 inputColor;
void main() {
    fragColor = inputColor;
}
)";

// Compila e linka shaders, retorna o programa
GLuint setupShaderProgram() {
    GLint success;
    GLchar infoLog[512];

    // Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertexShaderSource,nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader,GL_COMPILE_STATUS,&success);
    if(!success) {
        glGetShaderInfoLog(vertexShader,512,nullptr,infoLog);
        std::cerr<<"Vertex Shader Error:\n"<<infoLog<<std::endl;
    }

    // Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragmentShaderSource,nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader,GL_COMPILE_STATUS,&success);
    if(!success) {
        glGetShaderInfoLog(fragmentShader,512,nullptr,infoLog);
        std::cerr<<"Fragment Shader Error:\n"<<infoLog<<std::endl;
    }

    // Shader Program
    GLuint program = glCreateProgram();
    glAttachShader(program,vertexShader);
    glAttachShader(program,fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program,GL_LINK_STATUS,&success);
    if(!success) {
        glGetProgramInfoLog(program,512,nullptr,infoLog);
        std::cerr<<"Shader Program Link Error:\n"<<infoLog<<std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Cria um VAO com um quad (2 triângulos) de tamanho unitário [0,1]x[0,1]
GLuint createQuadVAO() {
    GLfloat verts[] = {
        // first triangle
         0.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f,
         1.0f, 1.0f, 0.0f,
        // second triangle
         0.0f, 0.0f, 0.0f,
         1.0f, 1.0f, 0.0f,
         0.0f, 1.0f, 0.0f
    };
    GLuint VAO, VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER,VBO);
      glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
      glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(void*)0);
      glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return VAO;
}

// Callback de mouse: clica em um retângulo da grade para escolher sua cor
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && attempts < MAX_ATTEMPTS) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        my = WINDOW_H - my;

        int cx = int(mx / RECT_W);
        int cy = int(my / RECT_H);
        if (cx < 0 || cx >= COLS || cy < 0 || cy >= ROWS) return;

        int idx = cy * COLS + cx;
        if (!grid[idx].alive) return;

        glm::vec3 chosen = grid[idx].color;
        std::vector<std::pair<int, float>> removedInfo;

        // verifica cada retângulo
        for (int i = 0; i < (int)grid.size(); ++i) {
            auto& r = grid[i];
            if (!r.alive) continue;
            float d = colorDistance(r.color, chosen);
            if (d <= COLOR_THRESHOLD) {
                r.alive = false;
                removedInfo.emplace_back(i, d);
            }
        }

        int removedCount = removedInfo.size();
        score    += removedCount;
        attempts += 1;

        // LOG detalhado
        std::cout << "Clique #" << attempts
                  << ": removidos " << removedCount
                  << " retângulos (limiar=" << COLOR_THRESHOLD << ")\n";
        for (auto& pr : removedInfo) {
            int   i = pr.first;
            float d = pr.second;
            auto& r = grid[i];
            std::cout << "  - índice " << i
                      << " em pos(" << r.pos.x << "," << r.pos.y << ")"
                      << " cor(" << r.color.r << "," << r.color.g << "," << r.color.b << ")"
                      << " dist=" << d
                      << "\n";
        }

        // atualiza título da janela de forma segura
        char title[128];
        std::snprintf(title, sizeof(title),
                      "Color Match - Score: %d   Attempts: %d/%d",
                      score, attempts, MAX_ATTEMPTS);
        glfwSetWindowTitle(window, title);
    }
}

// Callback de teclado: ESC para sair
void key_callback(GLFWwindow* window,int key,int scancode,int action,int mods) {
    if(key==GLFW_KEY_ESCAPE && action==GLFW_PRESS)
        glfwSetWindowShouldClose(window,true);
}

int main(){
    // 1) Inicializa GLFW
    if(!glfwInit()){
        std::cerr<<"Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    // 2) Cria janela
    GLFWwindow* window = glfwCreateWindow(WINDOW_W,WINDOW_H,"Color Match",nullptr,nullptr);
    if(!window){
        std::cerr<<"Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 3) Carrega GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr<<"Failed to init GLAD\n";
        return -1;
    }
    glViewport(0,0,WINDOW_W,WINDOW_H);

    // 4) Compila shaders e cria VAO
    GLuint shaderProgram = setupShaderProgram();
    GLuint quadVAO      = createQuadVAO();

    // 5) Configura projection
    glm::mat4 projection = glm::ortho(
        0.0f, float(WINDOW_W),
        0.0f, float(WINDOW_H),
       -1.0f,  1.0f
    );
    glUseProgram(shaderProgram);
    GLint projLoc = glGetUniformLocation(shaderProgram,"projection");
    glUniformMatrix4fv(projLoc,1,GL_FALSE,glm::value_ptr(projection));

    // 6) Inicializa jogo e callbacks
    initGrid();
    glfwSetMouseButtonCallback(window,mouse_button_callback);
    glfwSetKeyCallback(window,key_callback);

    // 7) Main loop
    while(!glfwWindowShouldClose(window)){
        // se esgotou tentativas ou todos removidos, encerra
        bool anyAlive=false;
        for(auto& r: grid) if(r.alive){ anyAlive=true; break; }
        if(!anyAlive || attempts>=MAX_ATTEMPTS) break;

        glClearColor(0.15f,0.15f,0.15f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);

        // desenha cada retângulo
        for(auto& r: grid){
            if(!r.alive) continue;
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(r.pos,0.0f));
            model = glm::scale(model, glm::vec3(RECT_W,RECT_H,1.0f));
            GLint modelLoc = glGetUniformLocation(shaderProgram,"model");
            glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));

            GLint colorLoc = glGetUniformLocation(shaderProgram,"inputColor");
            glUniform4f(colorLoc, r.color.r, r.color.g, r.color.b, 1.0f);

            glDrawArrays(GL_TRIANGLES,0,6);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 8) Final
    std::cout<<"\n=== Game Over ===\n"
             <<"Final Score: "<<score<<"\n"
             <<"Attempts Used: "<<attempts<<" / "<<MAX_ATTEMPTS<<"\n";

    glfwTerminate();
    return 0;
}
