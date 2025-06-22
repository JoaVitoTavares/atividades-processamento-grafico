// CustomTextureMapping.cpp
// Fundo texturizado e um personagem com várias animações por spritesheet.
// OpenGL 3.3 + GLFW + GLAD + GLM + stb_image.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

// dimensão da janela
const unsigned int SCR_W = 800, SCR_H = 600;

GLuint loadTexture(const char* path){
    stbi_set_flip_vertically_on_load(true);
    int w,h,n;
    unsigned char* data = stbi_load(path,&w,&h,&n,0);
    if(!data){ std::cerr<<"Erro ao carregar "<<path<<"\n"; return 0; }
    GLenum fmt = (n==3?GL_RGB:GL_RGBA);
    GLuint t; glGenTextures(1,&t);
    glBindTexture(GL_TEXTURE_2D,t);
      glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,data);
      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(data);
    return t;
}

// quad unitário com pos+uv
GLuint quadVAO = 0;
void initQuad(){
    float V[] = {
        // pos      // uv
        -0.5f,  0.5f,   0.0f,1.0f,
         0.5f, -0.5f,   1.0f,0.0f,
        -0.5f, -0.5f,   0.0f,0.0f,
        -0.5f,  0.5f,   0.0f,1.0f,
         0.5f,  0.5f,   1.0f,1.0f,
         0.5f, -0.5f,   1.0f,0.0f
    };
    GLuint VBO;
    glGenVertexArrays(1,&quadVAO);
    glGenBuffers(1,&VBO);
    glBindVertexArray(quadVAO);
      glBindBuffer(GL_ARRAY_BUFFER,VBO);
      glBufferData(GL_ARRAY_BUFFER,sizeof(V),V,GL_STATIC_DRAW);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

// shaders com suporte a sub-UV + outline flag
const char* vsSrc = R"glsl(
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
    gl_Position = projection * model * vec4(aPos,0,1);
}
)glsl";

const char* fsSrc = R"glsl(
#version 330 core
in vec2 UV;
out vec4 Frag;
uniform sampler2D spriteTex;
uniform bool   u_outline;
uniform vec4   u_outlineColor;
void main(){
    if(u_outline)      Frag = u_outlineColor;
    else               Frag = texture(spriteTex, UV);
}
)glsl";

GLuint compileShader(GLenum t,const char*src){
    GLuint s=glCreateShader(t);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok; char log[512];
    glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){
        glGetShaderInfoLog(s,512,nullptr,log);
        std::cerr<< (t==GL_VERTEX_SHADER?"VS":"FS") <<" error:\n"<<log;
    }
    return s;
}
GLuint createProgram(){
    GLuint vs=compileShader(GL_VERTEX_SHADER,vsSrc);
    GLuint fs=compileShader(GL_FRAGMENT_SHADER,fsSrc);
    GLuint p=glCreateProgram();
    glAttachShader(p,vs);
    glAttachShader(p,fs);
    glLinkProgram(p);
    GLint ok; char log[512];
    glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){
        glGetProgramInfoLog(p,512,nullptr,log);
        std::cerr<<"Link error:\n"<<log;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

struct Sprite {
    GLuint   tex;
    int      nRows,nCols;
    float    frameDur,acc=0;
    int      frame=0,anim=0;

    Sprite(GLuint t,int rows,int cols,float dur)
      : tex(t),nRows(rows),nCols(cols),frameDur(dur){}

    void setAnimation(int row){
        if(row<0||row>=nRows) return;
        if(anim!=row){ anim=row; frame=0; acc=0; }
    }

    void Update(float dt){
        acc+=dt;
        if(acc>=frameDur){
            frame=(frame+1)%nCols;
            acc-=frameDur;
        }
    }

    void Draw(GLuint prog){
        // calcula sub-UV
        glm::vec2 ds(1.0f/nCols,1.0f/nRows);
        glm::vec2 off(frame*ds.x,(nRows-1-anim)*ds.y);
        glUniform2fv(glGetUniformLocation(prog,"texScale"),1,glm::value_ptr(ds));
        glUniform2fv(glGetUniformLocation(prog,"texOffset"),1,glm::value_ptr(off));
        // draw
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,tex);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES,0,6);
    }
};

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(SCR_W,SCR_H,"Sprite Control",nullptr,nullptr);
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glViewport(0,0,SCR_W,SCR_H);
    GLuint shader = createProgram();
    glUseProgram(shader);

    GLint locProj    = glGetUniformLocation(shader,"projection");
    GLint locSprite  = glGetUniformLocation(shader,"spriteTex");
    glUniformMatrix4fv(locProj,1,GL_FALSE,glm::value_ptr(
      glm::ortho(0.0f,(float)SCR_W,0.0f,(float)SCR_H,-1.0f,1.0f)));
    glUniform1i(locSprite,0);

    GLint locModel        = glGetUniformLocation(shader,"model");
    GLint locOutline      = glGetUniformLocation(shader,"u_outline");
    GLint locOutlineColor = glGetUniformLocation(shader,"u_outlineColor");

    initQuad();
    GLuint outlineVAO, vboO;
    {
        float C[] = { -0.5f,0.5f,  0.5f,0.5f,  0.5f,-0.5f, -0.5f,-0.5f };
        glGenVertexArrays(1,&outlineVAO);
        glGenBuffers(1,&vboO);
        glBindVertexArray(outlineVAO);
          glBindBuffer(GL_ARRAY_BUFFER,vboO);
          glBufferData(GL_ARRAY_BUFFER,sizeof(C),C,GL_STATIC_DRAW);
          glEnableVertexAttribArray(0);
          glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
        glBindVertexArray(0);
    }

    Sprite bg   ( loadTexture("resources/background.png"),   1, 1, 1.0f );
    Sprite idle ( loadTexture("resources/Gangsters/Idle.png"),   1, 7, 0.12f );
    Sprite walk ( loadTexture("resources/Gangsters/Walk.png"),   1,10, 0.10f );

    glm::vec2 bgPos   = { SCR_W * 0.5f, SCR_H * 0.5f };
    glm::vec2 bgScale = { (float)SCR_W,  (float)SCR_H   };
    glm::vec2 playerPos   = { 400.0f, 300.0f };
    glm::vec2 playerScale = {  64.0f,  64.0f   };
    Sprite*   player      = &idle;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    float lastT = (float)glfwGetTime();
    const float speed = 200.0f;

    while(!glfwWindowShouldClose(win)){
        float now = (float)glfwGetTime();
        float dt  = now - lastT; lastT = now;

        glfwPollEvents();
        if(glfwGetKey(win,GLFW_KEY_ESCAPE)==GLFW_PRESS) break;

        bool up    = glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS;
        bool down  = glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS;
        bool left  = glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS;
        bool right = glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS;

        if(up||down||left||right){
            player = &walk;
            if(up)    playerPos.y += speed * dt;
            if(down)  playerPos.y -= speed * dt;
            if(left)  playerPos.x -= speed * dt;
            if(right) playerPos.x += speed * dt;
        } else {
            player = &idle;
        }
        player->Update(dt);

        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        glUniform1i(locOutline,0);

        {
          glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(bgPos,0.0f))
                      * glm::scale   (glm::mat4(1.0f), glm::vec3(bgScale,1.0f));
          glUniformMatrix4fv(locModel,1,GL_FALSE,glm::value_ptr(M));
          bg.Draw(shader);
        }
        {
          glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(playerPos,0.0f))
                      * glm::scale   (glm::mat4(1.0f), glm::vec3(playerScale,1.0f));
          glUniformMatrix4fv(locModel,1,GL_FALSE,glm::value_ptr(M));
          player->Draw(shader);
        }

        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        glUniform1i(locOutline,1);
        glUniform4f(locOutlineColor,1,1,1,1);
        glLineWidth(2.0f);
        glBindVertexArray(outlineVAO);

        {
          glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(bgPos,0.0f))
                      * glm::scale   (glm::mat4(1.0f), glm::vec3(bgScale,1.0f));
          glUniformMatrix4fv(locModel,1,GL_FALSE,glm::value_ptr(M));
          glDrawArrays(GL_LINE_LOOP,0,4);
        }
        {
          glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(playerPos,0.0f))
                      * glm::scale   (glm::mat4(1.0f), glm::vec3(playerScale,1.0f));
          glUniformMatrix4fv(locModel,1,GL_FALSE,glm::value_ptr(M));
          glDrawArrays(GL_LINE_LOOP,0,4);
        }

        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        glUniform1i(locOutline,0);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
