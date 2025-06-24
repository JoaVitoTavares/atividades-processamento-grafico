#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <fstream>

typedef unsigned int uint;
const uint SCR_W = 800, SCR_H = 600;
const int MAP_H = 15;
const int MAP_W = 15;
int mapData[MAP_H][MAP_W];

const int PINK_TILE_INDEX = 6; // Ainda pode ser útil para debug ou outros propósitos
const int IMPASSABLE_TILE_DEEP_WATER = 5;

bool g_keysPressed[GLFW_KEY_LAST + 1] = {false};

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key >= 0 && key <= GLFW_KEY_LAST)
    {
        if (action == GLFW_PRESS)
        {
            g_keysPressed[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            g_keysPressed[key] = false;
        }
    }
}

GLuint loadTexture(const char *path)
{
    stbi_set_flip_vertically_on_load(true);
    int w, h, n;
    unsigned char *data = stbi_load(path, &w, &h, &n, 0);
    if (!data)
    {
        std::cerr << "Failed to load " << path << "\n";
        return 0;
    }
    GLenum fmt = (n == 3 ? GL_RGB : GL_RGBA); // Assume RGB se 3 canais, RGBA se 4 canais
    if (n == 4) fmt = GL_RGBA; // Explicitamente definir para RGBA se houver canal alfa
    else if (n == 3) fmt = GL_RGB;
    else {
        std::cerr << "Formato de imagem nao suportado (canais: " << n << ") para " << path << "\n";
        stbi_image_free(data);
        return 0;
    }

    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return t;
}

GLuint quadVAO;
void initQuad()
{
    float V[] = {
            // Posição (x,y), Coordenadas de Textura (u,v)
            -0.5f,  0.5f,  0.0f, 1.0f, // Top-left
            0.5f, -0.5f,  1.0f, 0.0f, // Bottom-right
            -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left

            -0.5f,  0.5f,  0.0f, 1.0f, // Top-left
            0.5f,  0.5f,  1.0f, 1.0f, // Top-right
            0.5f, -0.5f,  1.0f, 0.0f  // Bottom-right
    };
    GLuint VBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(V), V, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
}

const char *vsSrc = R"glsl(
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

const char *fsSrc = R"glsl(
#version 330 core
in vec2 UV;
out vec4 Frag;
uniform sampler2D spriteTex;
uniform bool u_outline;
uniform vec4 u_outlineColor;
void main(){
    // Para sprites com transparência, use discard para pixels totalmente transparentes
    vec4 texColor = texture(spriteTex, UV);
    if (texColor.a < 0.1) discard; // Descartar pixels quase transparentes

    if(u_outline) Frag = u_outlineColor;
    else          Frag = texColor;
}
)glsl";

static GLuint compileShader(GLenum t, const char *src)
{
    GLuint s = glCreateShader(t);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    char log[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << (t == GL_VERTEX_SHADER ? "VS" : "FS") << " error:\n"
                  << log;
    }
    return s;
}

static GLuint createProgram()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    char log[512];
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        glGetProgramInfoLog(p, 512, nullptr, log);
        std::cerr << "Link error:\n"
                  << log;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

GLuint outlineVAO;
void initOutline()
{
    float C[] = {
            -0.5f, 0.5f,
            0.5f, 0.5f,
            0.5f, -0.5f,
            -0.5f, -0.5f};
    GLuint vbo;
    glGenVertexArrays(1, &outlineVAO);
    glGenBuffers(1, &vbo);
    glBindVertexArray(outlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(C), C, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glBindVertexArray(0);
}

bool loadMapFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo do mapa: " << filename << std::endl;
        return false;
    }

    int fileMapH, fileMapW;
    file >> fileMapH >> fileMapW;

    if (fileMapH != MAP_H || fileMapW != MAP_W) {
        std::cerr << "Aviso: Dimensoes do mapa no arquivo (" << fileMapH << "x" << fileMapW
                  << ") nao correspondem as dimensoes esperadas (" << MAP_H << "x" << MAP_W << ")." << std::endl;
    }

    const int centerX = MAP_H / 2;
    const int centerY = MAP_W / 2;

    for (int i = 0; i < MAP_H; ++i) {
        for (int j = 0; j < MAP_W; ++j) {
            if (!(file >> mapData[i][j])) {
                std::cerr << "Erro: Falha ao ler o tile em [" << i << "][" << j << "] do arquivo." << std::endl;
                return false;
            }

            if (i == centerX && j == centerY && mapData[i][j] == IMPASSABLE_TILE_DEEP_WATER) {
                std::cerr << "Erro: A agua profunda (tile " << IMPASSABLE_TILE_DEEP_WATER
                          << ") nao pode ficar no centro do mapa [" << i << "][" << j
                          << "], pois é onde o personagem da spawn." << std::endl;
                return false;
            }
        }
    }

    file.close();
    std::cout << "Mapa carregado com sucesso de: " << filename << std::endl;
    return true;
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *win = glfwCreateWindow(SCR_W, SCR_H, "IsometricTilemap", nullptr, nullptr);
    if (!win)
    {
        std::cerr << "Failed to create GLFW window\n";
        return -1;
    }
    glfwMakeContextCurrent(win);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glfwSetKeyCallback(win, key_callback);

    if (!loadMapFromFile("map.txt")) {
        return -1;
    }

    glViewport(0, 0, SCR_W, SCR_H);
    GLuint shader = createProgram();
    glUseProgram(shader);

    glm::mat4 proj = glm::ortho(0.0f, float(SCR_W), 0.0f, float(SCR_H), -1.0f, 1.0f);
    GLint locP = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(locP, 1, GL_FALSE, glm::value_ptr(proj));

    glUniform1i(glGetUniformLocation(shader, "spriteTex"), 0);

    initQuad();
    initOutline();

    GLuint tileset = loadTexture("resources/tileset.png"); // Textura dos tiles do mapa
    // AQUI É A LINHA QUE MUDOU:
    GLuint playerSpriteSheet = loadTexture("resources/pngwing.com.png"); // Sua spritesheet do boneco

    // Dimensões de um único sprite na spritesheet do jogador
    // Baseado na imagem fornecida: 17 colunas, 7 linhas, sprite de ~24x32 pixels
    const int playerSpriteCols = 17;
    const int playerSpriteRows = 7;
    const float playerSpriteW = 24.0f; // Largura de um único sprite
    const float playerSpriteH = 32.0f; // Altura de um único sprite

    const float tileW = 128.0f, tileH = 64.0f; // Dimensões dos tiles do mapa

    float halfW = tileW * 0.5f;
    float halfH = tileH * 0.5f;

    glm::vec2 mapOriginOffset(0.0f, 0.0f);

    GLint locM = glGetUniformLocation(shader, "model");
    GLint locTS = glGetUniformLocation(shader, "texScale");
    GLint locTO = glGetUniformLocation(shader, "texOffset");
    GLint locOL = glGetUniformLocation(shader, "u_outline");
    GLint locCLR = glGetUniformLocation(shader, "u_outlineColor");

    int ci = MAP_H / 2;
    int cj = MAP_W / 2;

    bool prevKeys[GLFW_KEY_LAST + 1] = {false};

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    const int nCols = 7;
    const int nRows = 1;

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        int ni = ci;
        int nj = cj;

        if (g_keysPressed[GLFW_KEY_S] && !prevKeys[GLFW_KEY_S]) { ni--; nj--; }
        if (g_keysPressed[GLFW_KEY_W] && !prevKeys[GLFW_KEY_W]) { ni++; nj++; }
        if (g_keysPressed[GLFW_KEY_D] && !prevKeys[GLFW_KEY_D]) { ni++; nj--; }
        if (g_keysPressed[GLFW_KEY_A] && !prevKeys[GLFW_KEY_A]) { ni--; nj++; }

        if (ni != ci || nj != cj)
        {
            if (ni >= 0 && ni < MAP_H && nj >= 0 && nj < MAP_W)
            {
                if (mapData[ni][nj] != IMPASSABLE_TILE_DEEP_WATER)
                {
                    ci = ni;
                    cj = nj;
                }
            }
        }

        for (int i = 0; i <= GLFW_KEY_LAST; ++i)
        {
            prevKeys[i] = g_keysPressed[i];
        }

        float targetX = (ci - cj) * halfW + mapOriginOffset.x;
        float targetY = (ci + cj) * halfH + mapOriginOffset.y;

        float offsetX = (SCR_W * 0.5f) - targetX;
        float offsetY = (SCR_H * 0.5f) - targetY;

        proj = glm::ortho(0.0f - offsetX, float(SCR_W) - offsetX, 0.0f - offsetY, float(SCR_H) - offsetY, -1.0f, 1.0f);
        glUniformMatrix4fv(locP, 1, GL_FALSE, glm::value_ptr(proj));

        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(locOL, 0);

        float dsx_tile = 1.0f / float(nCols);
        float dsy_tile = 1.0f / float(nRows);

        for (int i = 0; i < MAP_H; ++i)
        {
            for (int j = 0; j < MAP_W; ++j)
            {
                int idx = mapData[i][j];
                float offx = idx * dsx_tile;

                float x = (i - j) * halfW + mapOriginOffset.x;
                float y = (i + j) * halfH + mapOriginOffset.y;

                glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(tileW, tileH, 1));
                glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M));
                glUniform2f(locTS, dsx_tile, dsy_tile);
                glUniform2f(locTO, offx, 0.0f);

                glBindTexture(GL_TEXTURE_2D, tileset);
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        float playerX = (ci - cj) * halfW + mapOriginOffset.x;
        float playerY = (ci + cj) * halfH + mapOriginOffset.y + (tileH * 0.25f);

        int playerSpriteIndexX = 0;
        int playerSpriteIndexY = 0;

        float dsx_player = 1.0f / float(playerSpriteCols);
        float dsy_player = 1.0f / float(playerSpriteRows);
        float offx_player = playerSpriteIndexX * dsx_player;
        float offy_player = playerSpriteIndexY * dsy_player;

        glm::mat4 M_player = glm::translate(glm::mat4(1.0f), glm::vec3(playerX, playerY, 0.1f))
                             * glm::scale(glm::mat4(1.0f), glm::vec3(playerSpriteW, playerSpriteH, 1));
        glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_player));
        glUniform2f(locTS, dsx_player, dsy_player);
        glUniform2f(locTO, offx_player, offy_player);

        glBindTexture(GL_TEXTURE_2D, playerSpriteSheet);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);


        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform1i(locOL, 1);
        glUniform4f(locCLR, 1, 1, 1, 1);
        glLineWidth(2.0f);

        glBindVertexArray(outlineVAO);
        {
            float x_outline = (ci - cj) * halfW + mapOriginOffset.x;
            float y_outline = (ci + cj) * halfH + mapOriginOffset.y;
            glm::mat4 M_outline = glm::translate(glm::mat4(1.0f), glm::vec3(x_outline, y_outline, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(tileW, tileH, 1));
            glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_outline));
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }
        glUniform1i(locOL, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}