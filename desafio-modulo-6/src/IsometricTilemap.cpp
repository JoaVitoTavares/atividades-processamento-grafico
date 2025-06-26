#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>     // Para geração de números aleatórios
#include <chrono>     // Para seed de números aleatórios
#include <algorithm>  // Para std::find

typedef unsigned int uint;
const uint SCR_W = 800, SCR_H = 600;
const int MAP_H = 15;
const int MAP_W = 15;
int mapData[MAP_H][MAP_W];

const int PINK_TILE_INDEX = 6;
const int IMPASSABLE_TILE_DEEP_WATER = 5;
const int GAME_OVER_TILE = 3; // Novo: Definindo o ID do tile de game over

// ===========================================
// Variáveis Globais do Jogador
// ===========================================
int playerGridX = MAP_W / 2;
int playerGridY = MAP_H / 2;
float playerMoveSpeed = 1.0f;
int playerAnimationFrameX = 0;
int playerAnimationFrameY = 3; // Linha inicial para "para baixo"
double lastFrameTime = 0.0;

// Variáveis para controle de tempo para animação do jogador
double g_lastFrameTime = 0.0;
float g_animationSpeed = 0.15f;
int g_currentAnimationFrame = 0;
int g_animationDirection = 0;

const int PLAYER_SPRITE_RUN_FRAMES = 7;
const int PLAYER_SPRITE_ROWS = 4;

// Variável de estado do jogo
bool isGameOver = false; // Novo: Variável para controlar o estado do jogo

// ===========================================
// Estrutura para os Itens
// ===========================================
struct GameItem {
    int gridX;
    int gridY;
    GLuint textureID; // Agora armazena o ID da textura OpenGL diretamente
    bool collected;   // Se o item foi coletado (para futura lógica)
};

std::vector<GameItem> gameItems;

// IDs das texturas para os cristais individuais
GLuint darkRedCrystalTexture;
GLuint whiteCrystalTexture;
GLuint yellowCrystalTexture;

// Ajuste os tamanhos para os cristais individuais, se necessário.
// Assumindo que todos são do mesmo tamanho da spritesheet anterior para consistência inicial.
const float ITEM_SINGLE_SPRITE_W = 32.0f; // Largura de um sprite individual de item (verifique com suas imagens)
const float ITEM_SINGLE_SPRITE_H = 32.0f; // Altura de um sprite individual de item (verifique com suas imagens)


// ===========================================
// Controle de Entrada
// ===========================================
bool g_keysPressed[GLFW_KEY_LAST + 1] = {false};
bool g_keysHandled[GLFW_KEY_LAST + 1] = {false};

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
            g_keysHandled[key] = false;
        }
    }
}

// ===========================================
// Carregamento de Texturas e Inicialização de Geometria
// ===========================================
// A função loadTexture permanece a mesma, pois carrega uma única imagem por vez.
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
    GLenum fmt;
    if (n == 4) fmt = GL_RGBA;
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

// ===========================================
// Shaders
// ===========================================
const char *vsSrc = R"glsl(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 projection;
uniform mat4 model;
uniform vec2 texScale; // Mantemos para flexibilidade, mas será 1.0, 1.0 para itens individuais
uniform vec2 texOffset; // Mantemos para flexibilidade, mas será 0.0, 0.0 para itens individuais
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
    vec4 texColor = texture(spriteTex, UV);
    if (texColor.a < 0.05) discard;

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

// ===========================================
// Carregamento do Mapa e Geração de Itens
// ===========================================
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

    playerGridY = MAP_H / 2;
    playerGridX = MAP_W / 2;

    for (int i = 0; i < MAP_H; ++i) {
        for (int j = 0; j < MAP_W; ++j) {
            if (!(file >> mapData[i][j])) {
                std::cerr << "Erro: Falha ao ler o tile em [" << i << "][" << j << "] do arquivo." << std::endl;
                return false;
            }

            // Garante que a posição inicial do jogador não seja um tile intransitável
            if (i == playerGridY && j == playerGridX &&
                (mapData[i][j] == IMPASSABLE_TILE_DEEP_WATER || mapData[i][j] == GAME_OVER_TILE)) { // Modificado: Não iniciar em tile de game over
                std::cerr << "Erro: O tile de inicio do personagem (" << mapData[i][j]
                          << ") no centro do mapa [" << i << "][" << j
                          << "] e intransitavel ou de game over. Por favor, ajuste o mapa." << std::endl;
                return false;
            }
        }
    }

    file.close();
    std::cout << "Mapa carregado com sucesso de: " << filename << std::endl;
    return true;
}

void generateRandomItems() {
    std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distGridX(0, MAP_W - 1);
    std::uniform_int_distribution<int> distGridY(0, MAP_H - 1);

    // Lista de IDs de textura dos cristais
    std::vector<GLuint> crystalTextures = {
            darkRedCrystalTexture,
            whiteCrystalTexture,
            yellowCrystalTexture
    };
    std::uniform_int_distribution<int> distCrystalType(0, crystalTextures.size() - 1);


    const int numberOfItems = 10; // Número de itens a serem gerados

    for (int i = 0; i < numberOfItems; ++i) {
        int randX = distGridX(rng);
        int randY = distGridY(rng);

        // Tenta encontrar um tile que não seja intransitável e não seja a posição inicial do jogador
        int attempts = 0;
        while ((mapData[randY][randX] == IMPASSABLE_TILE_DEEP_WATER || mapData[randY][randX] == GAME_OVER_TILE ||
                (randX == playerGridX && randY == playerGridY)) && attempts < 100) { // Limita as tentativas para evitar loop infinito
            randX = distGridX(rng);
            randY = distGridY(rng);
            attempts++;
        }

        if (attempts < 100) { // Se encontrou um lugar válido
            GameItem newItem;
            newItem.gridX = randX;
            newItem.gridY = randY;
            // Atribui uma das texturas de cristal carregadas aleatoriamente
            newItem.textureID = crystalTextures[distCrystalType(rng)];
            newItem.collected = false;
            gameItems.push_back(newItem);
        } else {
            std::cerr << "Aviso: Nao foi possivel encontrar um local valido para gerar um item apos varias tentativas." << std::endl;
        }
    }
    std::cout << "Gerados " << gameItems.size() << " itens aleatorios no mapa." << std::endl;
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *win = glfwCreateWindow(SCR_W, SCR_H, "IsometricTilemap - Player Control & Items", nullptr, nullptr);
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

    GLuint tileset = loadTexture("resources/tileset.png");
    GLuint playerSpriteSheet = loadTexture("resources/Vampires2_Run_full.png");

    // CARREGANDO AS NOVAS TEXTURAS INDIVIDUAIS DOS CRISTAIS
    darkRedCrystalTexture = loadTexture("resources/Dark_red_ crystal1.png");
    whiteCrystalTexture = loadTexture("resources/White_crystal1.png");
    yellowCrystalTexture = loadTexture("resources/Yellow_crystal1.png");

    if (playerSpriteSheet == 0) {
        std::cerr << "Erro fatal: Nao foi possivel carregar a spritesheet do jogador (Vampires2_Run_full.png). Verifique o caminho e o ficheiro." << std::endl;
        glfwTerminate();
        return -1;
    }
    // Verificando se todas as texturas de cristal foram carregadas
    if (darkRedCrystalTexture == 0 || whiteCrystalTexture == 0 || yellowCrystalTexture == 0) {
        std::cerr << "Erro fatal: Nao foi possivel carregar uma ou mais texturas de cristal. Verifique os caminhos e os ficheiros." << std::endl;
        glfwTerminate();
        return -1;
    }

    const int playerSpriteSheetTotalW = 448;
    const int playerSpriteSheetTotalH = 256;

    const float playerSingleSpriteW = 64.0f;
    const float playerSingleSpriteH = 64.0f;

    const float tileW = 128.0f, tileH = 64.0f;

    float halfW = tileW * 0.5f;
    float halfH = tileH * 0.5f;

    glm::vec2 mapOriginOffset(0.0f, 0.0f);

    GLint locM = glGetUniformLocation(shader, "model");
    GLint locTS = glGetUniformLocation(shader, "texScale");
    GLint locTO = glGetUniformLocation(shader, "texOffset");
    GLint locOL = glGetUniformLocation(shader, "u_outline");
    GLint locCLR = glGetUniformLocation(shader, "u_outlineColor");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    g_lastFrameTime = glfwGetTime();

    const int nCols = 7;
    const int nRows = 1;

    generateRandomItems(); // Gera os itens após o carregamento do mapa

    while (!glfwWindowShouldClose(win))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - g_lastFrameTime;
        g_lastFrameTime = currentTime;

        glfwPollEvents();

        if (!isGameOver) {
            int newPlayerGridX = playerGridX;
            int newPlayerGridY = playerGridY;
            bool playerAttemptedMove = false;
            int newPlayerAnimY = playerAnimationFrameY;

            if (g_keysPressed[GLFW_KEY_W] && !g_keysHandled[GLFW_KEY_W]) { // Nordeste
                newPlayerGridY++; newPlayerGridX++;
                newPlayerAnimY = 2;
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_W] = true;
            } else if (g_keysPressed[GLFW_KEY_S] && !g_keysHandled[GLFW_KEY_S]) { // Sudoeste (Frente)
                newPlayerGridY--; newPlayerGridX--;
                newPlayerAnimY = 3;
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_S] = true;
            } else if (g_keysPressed[GLFW_KEY_A] && !g_keysHandled[GLFW_KEY_A]) { // Sudeste
                newPlayerGridY--; newPlayerGridX++;
                newPlayerAnimY = 1;
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_A] = true;
            } else if (g_keysPressed[GLFW_KEY_D] && !g_keysHandled[GLFW_KEY_D]) { // Noroeste (Lado)
                newPlayerGridY++; newPlayerGridX--;
                newPlayerAnimY = 0;
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_D] = true;
            }

            if (playerAttemptedMove) {
                if (newPlayerGridX >= 0 && newPlayerGridX < MAP_W &&
                    newPlayerGridY >= 0 && newPlayerGridY < MAP_H)
                {
                    if (mapData[newPlayerGridY][newPlayerGridX] != IMPASSABLE_TILE_DEEP_WATER)
                    {
                        playerGridX = newPlayerGridX;
                        playerGridY = newPlayerGridY;
                        playerAnimationFrameY = newPlayerAnimY;

                        if (mapData[playerGridY][playerGridX] == GAME_OVER_TILE) {
                            isGameOver = true;
                            std::cout << "Game Over! Voce tocou no tile " << GAME_OVER_TILE << "!" << std::endl;
                        }
                        // Lógica de coleta de itens (adicional)
                        for (auto& item : gameItems) {
                            if (!item.collected && item.gridX == playerGridX && item.gridY == playerGridY) {
                                item.collected = true;
                                std::cout << "Item coletado!" << std::endl;
                                // Adicione aqui qualquer lógica de pontuação, inventário, etc.
                            }
                        }

                    } else {
                        playerAnimationFrameY = newPlayerAnimY;
                    }
                } else {
                    playerAnimationFrameY = newPlayerAnimY;
                }
                playerAnimationFrameX = (int)(currentTime / g_animationSpeed) % PLAYER_SPRITE_RUN_FRAMES;
            } else {
                playerAnimationFrameX = 0;
            }
        }

        if (isGameOver) {
            for (int i = 0; i <= GLFW_KEY_LAST; ++i) {
                g_keysPressed[i] = false;
                g_keysHandled[i] = false;
            }
        }

        float playerWorldX = (playerGridY - playerGridX) * halfW + mapOriginOffset.x;
        float playerWorldY = (playerGridY + playerGridX) * halfH + mapOriginOffset.y;

        float cameraOffsetX = (SCR_W * 0.5f) - playerWorldX;
        float cameraOffsetY = (SCR_H * 0.5f) - playerWorldY;

        proj = glm::ortho(0.0f - cameraOffsetX, float(SCR_W) - cameraOffsetX, 0.0f - cameraOffsetY, float(SCR_H) - cameraOffsetY, -1.0f, 1.0f);
        glUniformMatrix4fv(locP, 1, GL_FALSE, glm::value_ptr(proj));

        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(locOL, 0);

        // ===========================================
        // Renderização dos Tiles do Mapa
        // ===========================================
        float dsx_tile = 1.0f / float(nCols);
        float dsy_tile = 1.0f / float(nRows);

        for (int i = 0; i < MAP_H; ++i)
        {
            for (int j = 0; j < MAP_W; ++j)
            {
                int idx = mapData[i][j];
                float offx_tile = (float)(idx % nCols) * dsx_tile;
                float offy_tile = (float)(idx / nCols) * dsy_tile;

                float x = (i - j) * halfW + mapOriginOffset.x;
                float y = (i + j) * halfH + mapOriginOffset.y;

                float tileZ = (i + j) * 0.001f;

                glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, tileZ)) * glm::scale(glm::mat4(1.0f), glm::vec3(tileW, tileH, 1));
                glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M));
                glUniform2f(locTS, dsx_tile, dsy_tile);
                glUniform2f(locTO, offx_tile, offy_tile);

                glBindTexture(GL_TEXTURE_2D, tileset);
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // ===========================================
        // Renderização dos Itens (usando texturas individuais)
        // ===========================================
        // Para texturas individuais, texScale será 1.0, 1.0 e texOffset será 0.0, 0.0
        // pois cada imagem é um sprite completo.
        glUniform2f(locTS, 1.0f, 1.0f);
        glUniform2f(locTO, 0.0f, 0.0f);

        for (const auto& item : gameItems) {
            if (!item.collected) { // Renderiza apenas se não foi coletado
                float itemWorldX = (item.gridY - item.gridX) * halfW + mapOriginOffset.x;
                float itemWorldY = (item.gridY + item.gridX) * halfH + mapOriginOffset.y + (tileH * 0.5f); // Ajuste para ficar em cima do tile
                float itemZ = (item.gridY + item.gridX) * 0.001f + 0.2f; // Z maior que o tile, menor que o jogador

                glm::mat4 M_item = glm::translate(glm::mat4(1.0f), glm::vec3(itemWorldX, itemWorldY, itemZ))
                                   * glm::scale(glm::mat4(1.0f), glm::vec3(ITEM_SINGLE_SPRITE_W, ITEM_SINGLE_SPRITE_H, 1));
                glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_item));

                glBindTexture(GL_TEXTURE_2D, item.textureID); // VINCULA A TEXTURA ESPECÍFICA DO ITEM
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }


        // ===========================================
        // Renderização do Personagem (Vampiro)
        // ===========================================
        if (!isGameOver) {
            // Reajusta texScale e texOffset para a spritesheet do jogador
            const float dsx_player = playerSingleSpriteW / playerSpriteSheetTotalW;
            const float dsy_player = playerSingleSpriteH / playerSpriteSheetTotalH;

            float offx_player = playerAnimationFrameX * dsx_player;
            float offy_player = playerAnimationFrameY * dsy_player;

            glUniform2f(locTS, dsx_player, dsy_player);
            glUniform2f(locTO, offx_player, offy_player);

            float playerRenderX = (playerGridY - playerGridX) * halfW + mapOriginOffset.x;
            float playerRenderY = (playerGridY + playerGridX) * halfH + mapOriginOffset.y + (tileH * 0.25f);
            float playerZ = (playerGridY + playerGridX) * 0.001f + 0.5f;

            glm::mat4 M_player = glm::translate(glm::mat4(1.0f), glm::vec3(playerRenderX, playerRenderY, playerZ))
                                 * glm::scale(glm::mat4(1.0f), glm::vec3(playerSingleSpriteW, playerSingleSpriteH, 1));
            glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_player));

            glBindTexture(GL_TEXTURE_2D, playerSpriteSheet);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // ===========================================
        // Renderização do Contorno do Tile do Jogador
        // ===========================================
        if (!isGameOver) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glUniform1i(locOL, 1);
            glUniform4f(locCLR, 1, 1, 1, 1);
            glLineWidth(2.0f);

            // Garante que texScale e texOffset estejam corretos para o contorno (sem textura)
            glUniform2f(locTS, 1.0f, 1.0f);
            glUniform2f(locTO, 0.0f, 0.0f);

            glBindVertexArray(outlineVAO);
            {
                float x_outline = (playerGridY - playerGridX) * halfW + mapOriginOffset.x;
                float y_outline = (playerGridY + playerGridX) * halfH + mapOriginOffset.y;
                glm::mat4 M_outline = glm::translate(glm::mat4(1.0f), glm::vec3(x_outline, y_outline, (playerGridY + playerGridX) * 0.001f + 0.1f)) * glm::scale(glm::mat4(1.0f), glm::vec3(tileW, tileH, 1));
                glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_outline));
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
            glUniform1i(locOL, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}