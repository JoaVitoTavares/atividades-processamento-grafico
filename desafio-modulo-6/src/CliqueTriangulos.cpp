// CliqueTriangulos.cpp
// OpenGL 3.3 + GLFW + GLAD + GLM
// A cada clique um vértice, a cada 3 vértices um triângulo de cor diferente.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>

// Janela
const unsigned int SCR_W = 800, SCR_H = 600;

// Paleta simples de cores
std::vector<glm::vec3> palette = {
    {1,0,0},{0,1,0},{0,0,1},{1,1,0},{1,0,1},{0,1,1}
};
int nextColor = 0;

struct Triangle {
    GLuint VAO;
    glm::vec3 color;
};
std::vector<Triangle> triangles;

std::vector<glm::vec2> pendingVerts;

GLuint makeTriangleVAO(const glm::vec2 v0,
                       const glm::vec2 v1,
                       const glm::vec2 v2)
{
    float verts[] = {
        v0.x, v0.y,
        v1.x, v1.y,
        v2.x, v2.y
    };
    GLuint VAO, VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers     (1,&VBO);
    glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER,VBO);
      glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glBindVertexArray(0);
    return VAO;
}

// ——————————————————————
// Shaders mínimos
const char* vs_src = R"(
#version 330 core
layout(location=0) in vec2 aPos;
uniform mat4 projection;
void main(){
    gl_Position = projection * vec4(aPos,0,1);
}
)";
const char* fs_src = R"(
#version 330 core
out vec4 Frag;
uniform vec3 uColor;
void main(){
    Frag = vec4(uColor,1);
}
)";

GLuint compileShader(GLenum t,const char* src){
    GLuint s=glCreateShader(t);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ char log[512]; glGetShaderInfoLog(s,512,nullptr,log);
        std::cerr<<(t==GL_VERTEX_SHADER?"VS":"FS")<<" error:\n"<<log;
    }
    return s;
}
GLuint makeProgram(){
    GLuint vs=compileShader(GL_VERTEX_SHADER,vs_src),
           fs=compileShader(GL_FRAGMENT_SHADER,fs_src),
           p=glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs);
    glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// ——————————————————————
// Callback de clique
void mouse_cb(GLFWwindow* w,int button,int action,int mods){
    if(button!=GLFW_MOUSE_BUTTON_LEFT||action!=GLFW_PRESS) return;
    double x,y; glfwGetCursorPos(w,&x,&y);
    // converte para coordenadas de mundo (origem no canto inferior esquerdo):
    y = SCR_H - y;
    pendingVerts.emplace_back((float)x,(float)y);
    if(pendingVerts.size()==3){
        // cria triângulo
        GLuint VAO = makeTriangleVAO(
            pendingVerts[0],
            pendingVerts[1],
            pendingVerts[2]
        );
        triangles.push_back({ VAO, palette[nextColor] });
        nextColor = (nextColor+1) % palette.size();
        pendingVerts.clear();
    }
}

int main(){
    // GLFW + contexto
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(SCR_W,SCR_H,
                                       "Clique→Vértice→Triângulo",nullptr,nullptr);
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // setup
    GLuint program = makeProgram();
    GLint locProj = glGetUniformLocation(program,"projection");
    GLint locColor= glGetUniformLocation(program,"uColor");
    glm::mat4 proj = glm::ortho(0.0f,(float)SCR_W,0.0f,(float)SCR_H,-1.0f,1.0f);
    glUseProgram(program);
    glUniformMatrix4fv(locProj,1,GL_FALSE,glm::value_ptr(proj));

    glfwSetMouseButtonCallback(win,mouse_cb);

    // loop
    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();
        glClearColor(0.1f,0.1f,0.1f,1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        for(auto &tri : triangles){
            glUniform3fv(locColor,1,glm::value_ptr(tri.color));
            glBindVertexArray(tri.VAO);
            glDrawArrays(GL_TRIANGLES,0,3);
        }

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
