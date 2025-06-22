// ParallaxScrolling.cpp
// Fundo construído em camadas (“layers”) e parallax scrolling.
// OpenGL 3.3 + GLFW + GLAD + GLM + stb_image.

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

//-----------------------------------------------------------------------------

static const unsigned int SCR_W = 800;
static const unsigned int SCR_H = 600;

GLuint loadTexture(const char* path) {
    stbi_set_flip_vertically_on_load(true);
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (!data) {
        std::cerr << "Erro ao carregar textura: " << path << std::endl;
        return 0;
    }
    GLenum format = (n == 3 ? GL_RGB : GL_RGBA);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
      glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}

//-----------------------------------------------------------------------------
// Quad unitário (–0.5..+0.5) com posição + UV (0..1)
GLuint quadVAO = 0;
void initQuad() {
    float verts[] = {
        // pos.x  pos.y    u     v
        -0.5f,  0.5f,     0.0f, 1.0f,
         0.5f, -0.5f,     1.0f, 0.0f,
        -0.5f, -0.5f,     0.0f, 0.0f,

        -0.5f,  0.5f,     0.0f, 1.0f,
         0.5f,  0.5f,     1.0f, 1.0f,
         0.5f, -0.5f,     1.0f, 0.0f
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

//-----------------------------------------------------------------------------
// Shaders (suporte a sub-UV + flag de outline)
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 projection;
uniform mat4 model;
uniform vec2 texScale;
uniform vec2 texOffset;

out vec2 UV;
void main(){
    UV = aUV * texScale + texOffset;
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
in vec2 UV;
out vec4 Frag;

uniform sampler2D spriteTex;
uniform bool      u_outline;
uniform vec4      u_outlineColor;

void main(){
    if (u_outline) {
        // Desenha em cor de contorno
        Frag = u_outlineColor;
    } else {
        // Textura normal
        Frag = texture(spriteTex, UV);
    }
}
)glsl";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; char log[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << (type == GL_VERTEX_SHADER ? "VS" : "FS") << " compile error:\n"
                  << log << std::endl;
    }
    return s;
}

static GLuint createShaderProgram() {
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
        std::cerr << "Program link error:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

struct Sprite {
    GLuint   tex;
    glm::vec2 pos;
    glm::vec2 scale;
    float     rot;

    Sprite(GLuint _tex = 0) : tex(_tex), pos(0,0), scale(1,1), rot(0) {}

    // Configura a transformação e desenha o quad
    void Draw(GLuint prog) {
        // Monta a matriz modelo = T * R * S
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pos, 0.0f));
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, 1.0f));

        // Passa para o shader
        glUniformMatrix4fv(glGetUniformLocation(prog, "model"),
                           1, GL_FALSE, glm::value_ptr(model));

        // Para um atlas 1×1 (no caso não usamos sub-UV), texScale=1, texOffset=0
        glUniform2f(glGetUniformLocation(prog, "texScale"), 1.0f, 1.0f);
        glUniform2f(glGetUniformLocation(prog, "texOffset"), 0.0f, 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
};

struct SpriteAnim {
    GLuint   tex;
    int      nRows, nCols;
    float    frameDur, acc = 0.0f;
    int      currentFrame = 0;
    int      currentRow   = 0;
    glm::vec2 pos;
    glm::vec2 scale;
    float     rot;

    SpriteAnim(GLuint _tex, int rows, int cols, float dur)
      : tex(_tex), nRows(rows), nCols(cols), frameDur(dur),
        pos(0,0), scale(1,1), rot(0) {}

    void setAnimation(int row) {
        if (row < 0 || row >= nRows) return;
        if (row != currentRow) {
            currentRow   = row;
            currentFrame = 0;
            acc = 0.0f;
        }
    }

    void Update(float dt) {
        acc += dt;
        if (acc >= frameDur) {
            currentFrame = (currentFrame + 1) % nCols;
            acc -= frameDur;
        }
    }

    void Draw(GLuint prog) {
        // model = T * R * S
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pos, 0.0f));
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(prog, "model"),
                           1, GL_FALSE, glm::value_ptr(model));

        // Cálculo de sub-UV:
        float cellW = 1.0f / float(nCols);
        float cellH = 1.0f / float(nRows);
        glm::vec2 texScale(cellW, cellH);
        glm::vec2 texOffset(currentFrame * cellW,
                            (nRows - 1 - currentRow) * cellH);

        glUniform2fv(glGetUniformLocation(prog, "texScale"), 1, &texScale.x);
        glUniform2fv(glGetUniformLocation(prog, "texOffset"), 1, &texOffset.x);

        // Desenha
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
};

int main() {
    // 1) Inicialização GLFW + GLAD
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW." << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Parallax Scrolling", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar janela GLFW." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD." << std::endl;
        glfwTerminate();
        return -1;
    }

    // 2) Configurações iniciais
    glViewport(0, 0, SCR_W, SCR_H);
    GLuint shader = createShaderProgram();
    glUseProgram(shader);

    // Projeção ortográfica (0..800 em X, 0..600 em Y)
    glm::mat4 projection = glm::ortho(0.0f, float(SCR_W),
                                      0.0f, float(SCR_H),
                                      -1.0f, 1.0f);
    GLint locProj = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));

    // Uniform do sampler2D
    glUniform1i(glGetUniformLocation(shader, "spriteTex"), 0);

    // Cache de locais de uniform
    GLint locModel        = glGetUniformLocation(shader, "model");
    GLint locOutline      = glGetUniformLocation(shader, "u_outline");
    GLint locOutlineColor = glGetUniformLocation(shader, "u_outlineColor");

    // Inicializa o quad
    initQuad();

    // Outline VAO (retângulo 1×1, apenas posição, para desenhar bordas)
    GLuint outlineVAO, outlineVBO;
    {
        float coords[] = {
            -0.5f,  0.5f,
             0.5f,  0.5f,
             0.5f, -0.5f,
            -0.5f, -0.5f
        };
        glGenVertexArrays(1, &outlineVAO);
        glGenBuffers(1, &outlineVBO);
        glBindVertexArray(outlineVAO);
          glBindBuffer(GL_ARRAY_BUFFER, outlineVBO);
          glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);
          glEnableVertexAttribArray(0);
          glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    //-----------------------------------------------------------------------------  
    // 3) Carrega cada camada de fundo como Sprite(1x1)
    Sprite layer_sky     ( loadTexture("resources/background_layers/sky.png") );
    Sprite layer_clouds1 ( loadTexture("resources/background_layers/clouds_1.png") );
    Sprite layer_clouds2 ( loadTexture("resources/background_layers/clouds_2.png") );
    Sprite layer_ground1 ( loadTexture("resources/background_layers/ground_1.png") );
    Sprite layer_ground2 ( loadTexture("resources/background_layers/ground_2.png") );
    Sprite layer_ground3 ( loadTexture("resources/background_layers/ground_3.png") );
    Sprite layer_rocks   ( loadTexture("resources/background_layers/rocks.png") );
    Sprite layer_plant   ( loadTexture("resources/background_layers/plant.png") );

    // Faz com que cada camada cubra exatamente a janela (800×600)
    layer_sky.pos   = { float(SCR_W)/2.0f, float(SCR_H)/2.0f };
    layer_sky.scale = { float(SCR_W), float(SCR_H) };

    layer_clouds1.pos = layer_sky.pos;    layer_clouds1.scale = layer_sky.scale;
    layer_clouds2.pos = layer_sky.pos;    layer_clouds2.scale = layer_sky.scale;
    layer_ground1.pos = layer_sky.pos;    layer_ground1.scale = layer_sky.scale;
    layer_ground2.pos = layer_sky.pos;    layer_ground2.scale = layer_sky.scale;
    layer_ground3.pos = layer_sky.pos;    layer_ground3.scale = layer_sky.scale;
    layer_rocks.pos   = layer_sky.pos;    layer_rocks.scale   = layer_sky.scale;
    layer_plant.pos   = layer_sky.pos;    layer_plant.scale   = layer_sky.scale;

    SpriteAnim playerIdle ( loadTexture("resources/Gangsters/Idle.png"), 1, 7,  0.12f );
    SpriteAnim playerWalk ( loadTexture("resources/Gangsters/Walk.png"), 1, 10, 0.10f );

    playerIdle.pos   = { float(SCR_W)/2.0f, float(SCR_H)/2.0f };
    playerIdle.scale = { 64.0f, 64.0f };   // 64×64 px no mundo
    playerWalk.pos   = playerIdle.pos;
    playerWalk.scale = playerIdle.scale;

    SpriteAnim* player = &playerIdle;

    //-----------------------------------------------------------------------------  
    // 5) Variável global de “camera X” (offset do mundo) para o parallax
    float cameraX = 0.0f;

    // Velocidade de movimento (pixels por segundo)
    const float MOVE_SPEED = 200.0f;

    // Habilita blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Temporizador para animações
    float lastTime = static_cast<float>(glfwGetTime());

    //-----------------------------------------------------------------------------  
    // 6) Loop principal
    while (!glfwWindowShouldClose(window)) {
        // ——> 6.1) Processa eventos do GLFW <——
        glfwPollEvents();  
        // Sem este glfwPollEvents(), a janela não chega a “aparecer” porque o
        // GLFW não processa internamente o pedido de criar/contextualizar a área
        // de renderização, nem captura os eventos de sistema.

        // Calcula delta-time
        float now = static_cast<float>(glfwGetTime());
        float dt  = now - lastTime;
        lastTime  = now;

        // — Entrada do usuário → atualiza cameraX e troca de animação do player —
        bool keyLeft  = glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS
                      || glfwGetKey(window, GLFW_KEY_A)     == GLFW_PRESS;
        bool keyRight = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS
                      || glfwGetKey(window, GLFW_KEY_D)     == GLFW_PRESS;
        bool keyUp    = glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS
                      || glfwGetKey(window, GLFW_KEY_W)     == GLFW_PRESS;
        bool keyDown  = glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS
                      || glfwGetKey(window, GLFW_KEY_S)     == GLFW_PRESS;

        if (keyLeft) {
            cameraX -= MOVE_SPEED * dt;
            player = &playerWalk;
        }
        else if (keyRight) {
            cameraX += MOVE_SPEED * dt;
            player = &playerWalk;
        }
        else if (keyUp || keyDown) {
            player = &playerWalk;
            // não alteramos cameraX verticalmente, afinal nosso parallax é apenas horizontal.
        }
        else {
            player = &playerIdle;
        }

        player->Update(dt);

        //-----------------------------------------------------------------------------  
        // 6.2) Limpa a tela
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // PASS 1: desenha as camadas em modo FILL
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(locOutline, 0);

        // === 1) Layer “sky” (fator = 0.10f) ===
        {
            float fator   = 0.10f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;   // centro do primeiro tile
            float x2 = x1 + float(SCR_W);            // segundo tile à direita

            layer_sky.scale = { float(SCR_W), float(SCR_H) };

            layer_sky.pos = { x1, float(SCR_H)*0.5f };
            layer_sky.Draw(shader);

            layer_sky.pos = { x2, float(SCR_H)*0.5f };
            layer_sky.Draw(shader);
        }

        // === 2) Layer “clouds_1” (fator = 0.20f) ===
        {
            float fator   = 0.20f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;
            float x2 = x1 + float(SCR_W);

            layer_clouds1.scale = { float(SCR_W), float(SCR_H) };

            layer_clouds1.pos = { x1, float(SCR_H)*0.5f };
            layer_clouds1.Draw(shader);

            layer_clouds1.pos = { x2, float(SCR_H)*0.5f };
            layer_clouds1.Draw(shader);
        }

        // === 3) Layer “clouds_2” (fator = 0.30f) ===
        {
            float fator   = 0.30f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;
            float x2 = x1 + float(SCR_W);

            layer_clouds2.scale = { float(SCR_W), float(SCR_H) };

            layer_clouds2.pos = { x1, float(SCR_H)*0.5f };
            layer_clouds2.Draw(shader);

            layer_clouds2.pos = { x2, float(SCR_H)*0.5f };
            layer_clouds2.Draw(shader);
        }

        // === 4) Layer “ground_1” (fator = 0.50f) ===
        {
            float fator   = 0.50f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;
            float x2 = x1 + float(SCR_W);

            layer_ground1.scale = { float(SCR_W), float(SCR_H) };

            layer_ground1.pos = { x1, float(SCR_H)*0.5f };
            layer_ground1.Draw(shader);

            layer_ground1.pos = { x2, float(SCR_H)*0.5f };
            layer_ground1.Draw(shader);
        }

        // === 5) Layer “ground_2” (fator = 0.70f) ===
        {
            float fator   = 0.70f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;
            float x2 = x1 + float(SCR_W);

            layer_ground2.scale = { float(SCR_W), float(SCR_H) };

            layer_ground2.pos = { x1, float(SCR_H)*0.5f };
            layer_ground2.Draw(shader);

            layer_ground2.pos = { x2, float(SCR_H)*0.5f };
            layer_ground2.Draw(shader);
        }

        // === 6) Layer “ground_3” (fator = 0.90f) ===
        {
            float fator   = 0.90f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;
            float x2 = x1 + float(SCR_W);

            layer_ground3.scale = { float(SCR_W), float(SCR_H) };

            layer_ground3.pos = { x1, float(SCR_H)*0.5f };
            layer_ground3.Draw(shader);

            layer_ground3.pos = { x2, float(SCR_H)*0.5f };
            layer_ground3.Draw(shader);
        }

        // === 7) Layer “rocks” (fator = 1.10f) ===
        {
            float fator   = 1.10f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;
            float x2 = x1 + float(SCR_W);

            layer_rocks.scale = { float(SCR_W), float(SCR_H) };

            layer_rocks.pos = { x1, float(SCR_H)*0.5f };
            layer_rocks.Draw(shader);

            layer_rocks.pos = { x2, float(SCR_H)*0.5f };
            layer_rocks.Draw(shader);
        }

        // === 8) Layer “plant” (fator = 1.30f) ===
        {
            float fator   = 1.30f;
            float desloc  = fmodf(cameraX * fator, float(SCR_W));
            if (desloc < 0.0f) desloc += float(SCR_W);

            float x1 = float(SCR_W)*0.5f - desloc;
            float x2 = x1 + float(SCR_W);

            layer_plant.scale = { float(SCR_W), float(SCR_H) };

            layer_plant.pos = { x1, float(SCR_H)*0.5f };
            layer_plant.Draw(shader);

            layer_plant.pos = { x2, float(SCR_H)*0.5f };
            layer_plant.Draw(shader);
        }

        //---- Desenha o personagem (sempre no centro da tela) ----
        player->Draw(shader);

        // PASS 2: Desenha o contorno (wireframe) do player
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform1i(locOutline, 1);
        glUniform4f(locOutlineColor, 1.0f, 1.0f, 1.0f, 1.0f);
        glLineWidth(2.0f);

        glBindVertexArray(outlineVAO);
        {
            glm::mat4 M = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(player->pos, 0.0f))
                        * glm::scale   (glm::mat4(1.0f),
                                       glm::vec3(player->scale, 1.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(M));
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }

        // Restaura fill e “desliga” outline
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(locOutline, 0);

        //---- Troca buffers ----
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
