// TextureMapping.cpp
// Fundo texturizado e dois sprites animados.
// OpenGL 3.3 + GLFW + GLAD + GLM + stb_image.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

// Dimensões da janela
const unsigned int SCR_W = 800;
const unsigned int SCR_H = 600;

// Carrega textura
GLuint loadTexture(const char* path) {
    stbi_set_flip_vertically_on_load(true);
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (!data) {
        std::cerr << "Falha ao carregar textura: " << path << std::endl;
        return 0;
    }
    GLenum fmt = (n == 3 ? GL_RGB : GL_RGBA);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}

// Quad unitário com UVs
GLuint quadVAO = 0;
void initQuad() {
    float verts[] = {
        // pos      // uv
        -0.5f,  0.5f,   0.0f, 1.0f,
         0.5f, -0.5f,   1.0f, 0.0f,
        -0.5f, -0.5f,   0.0f, 0.0f,

        -0.5f,  0.5f,   0.0f, 1.0f,
         0.5f,  0.5f,   1.0f, 1.0f,
         0.5f, -0.5f,   1.0f, 0.0f,
    };
    GLuint VBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

GLuint outlineVAO = 0;
void initOutline() {
    float corners[] = {
        -0.5f,  0.5f,
         0.5f,  0.5f,
         0.5f, -0.5f,
        -0.5f, -0.5f
    };
    GLuint vbo;
    glGenVertexArrays(1, &outlineVAO);
    glGenBuffers     (1, &vbo);
    glBindVertexArray(outlineVAO);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

// Shaders com suporte a sub-UV
const char* vertexShaderSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 projection;
uniform mat4 model;
uniform vec2 texScale;
uniform vec2 texOffset;
out vec2 UV;
void main(){
    UV = aUV * texScale + texOffset;
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";
const char* fragmentShaderSrc = R"(
#version 330 core
in vec2 UV;
out vec4 Frag;
uniform sampler2D spriteTex;
uniform bool      u_outline;
uniform vec4      u_outlineColor;
void main(){
    if(u_outline) {
        Frag = u_outlineColor;
    } else {
        Frag = texture(spriteTex, UV);
    }
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; char log[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << ((type == GL_VERTEX_SHADER) ? "VS" : "FS") << " error:\n" << log;
    }
    return s;
}

GLuint createShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; char log[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Link error:\n" << log;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

struct Sprite {
    GLuint tex;
    int    frameCount;
    float  frameDur, acc = 0;
    int    current = 0;
    glm::vec2 pos{0,0}, scale{1,1};
    float rot = 0;

    Sprite(GLuint t, int fc, float fd)
        : tex(t), frameCount(fc), frameDur(fd) {}

    void Update(float dt) {
        if (frameCount > 1) {
            acc += dt;
            if (acc >= frameDur) {
                current = (current + 1) % frameCount;
                acc -= frameDur;
            }
        }
    }
    void Draw(GLuint prog) {
        // Model matrix
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(pos, 0.0f));
        m = glm::rotate(m, glm::radians(rot), glm::vec3(0,0,1));
        m = glm::scale(m, glm::vec3(scale,1.0f));
        glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE,glm::value_ptr(m));
        // UV sub-range
        glm::vec2 ts(1.0f/frameCount, 1.0f);
        glm::vec2 to(current * ts.x, 0.0f);
        glUniform2fv(glGetUniformLocation(prog,"texScale"),1,glm::value_ptr(ts));
        glUniform2fv(glGetUniformLocation(prog,"texOffset"),1,glm::value_ptr(to));
        // Draw
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
};



int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(SCR_W, SCR_H, "Texture Mapping", nullptr, nullptr);
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glViewport(0,0,SCR_W,SCR_H);
    GLuint shader = createShaderProgram();
    GLint locProjection   = glGetUniformLocation(shader, "projection");
    GLint locSpriteTex    = glGetUniformLocation(shader, "spriteTex");
    GLint locModel        = glGetUniformLocation(shader, "model");
    GLint locOutline      = glGetUniformLocation(shader, "u_outline");
    GLint locOutlineColor = glGetUniformLocation(shader, "u_outlineColor");
    glm::mat4 proj = glm::ortho(0.0f, (float)SCR_W, 0.0f, (float)SCR_H, -1.0f, 1.0f);
    // glUseProgram(shader);
    // glUniformMatrix4fv(glGetUniformLocation(shader,"projection"),1,GL_FALSE,glm::value_ptr(proj));
    // glUniform1i(glGetUniformLocation(shader,"spriteTex"),0);
    glUseProgram(shader);
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i      (locSpriteTex,  0);

    initQuad();
    initOutline();
    // Carrega texturas: fundo, sprite1(6), sprite2(9)
    Sprite bg   ( loadTexture("resources/background.png"), 1, 1.0f );
    Sprite spr1 ( loadTexture("resources/sprite1.png"),     6, 0.1f );
    Sprite spr2 ( loadTexture("resources/sprite2.png"),     9, 0.1f );

    // Configura posições/escala
    bg.pos   = { SCR_W/2.0f, SCR_H/2.0f };
    bg.scale = { (float)SCR_W, (float)SCR_H };
    spr1.pos   = { 200.0f,  50.0f };
    spr1.scale = { 96.0f,   96.0f };
    spr2.pos   = { 600.0f,  50.0f };
    spr2.scale = { 96.0f,   96.0f };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float last = (float)glfwGetTime();
    while(!glfwWindowShouldClose(win)){
        float now = (float)glfwGetTime();
        float dt  = now - last; last = now;
        glfwPollEvents();
        if(glfwGetKey(win,GLFW_KEY_ESCAPE)==GLFW_PRESS) break;

        // Atualiza animações
        spr1.Update(dt);
        spr2.Update(dt);

        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(shader);
        glUniform1i(locOutline, 0);
        bg .Draw(shader);
        spr1.Draw(shader);
        spr2.Draw(shader);

        glUseProgram(shader);
        glUniform1i(locOutline, 1);
        glUniform4f(locOutlineColor, 1,1,1,1);

        {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(bg.pos, 0.0f))
                        * glm::scale   (glm::mat4(1.0f), glm::vec3(bg.scale,1.0f));
            glUniformMatrix4fv(locModel,1,GL_FALSE, glm::value_ptr(m));
            glBindVertexArray(outlineVAO);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }

        {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(spr1.pos, 0.0f))
                        * glm::scale   (glm::mat4(1.0f), glm::vec3(spr1.scale,1.0f));
            glUniformMatrix4fv(locModel,1,GL_FALSE, glm::value_ptr(m));
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }

        {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(spr2.pos, 0.0f))
                        * glm::scale   (glm::mat4(1.0f), glm::vec3(spr2.scale,1.0f));
            glUniformMatrix4fv(locModel,1,GL_FALSE, glm::value_ptr(m));
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }

        glBindVertexArray(0);
        glUniform1i(locOutline, 0);  

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}