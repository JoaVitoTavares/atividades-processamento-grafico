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
#include <random>       // Para geração de números aleatórios
#include <chrono>       // Para seed de números aleatórios
#include <algorithm>    // Para std::find
#include <sstream>      // Para parsing de linhas
#include <map>          // Para armazenar as propriedades dos tiles

typedef unsigned int uint;
const uint SCR_W = 800, SCR_H = 600;
const int MAP_H = 15;
const int MAP_W = 15;
int mapData[MAP_H][MAP_W];
// Novo array para armazenar os IDs originais dos tiles
int originalMapData[MAP_H][MAP_W];


// Definicoes de tiles, agora com base nas propriedades carregadas
// Removendo PINK_TILE_INDEX, IMPASSABLE_TILE_DEEP_WATER, GAME_OVER_TILE
// pois serao definidos pelo arquivo de propriedades
const int PLAYER_INITIAL_TILE_ID = 0; // Exemplo: assumindo que o tile 0 é sempre caminhável e seguro para iniciar
const int HIGHLIGHT_TILE_ID = 6; // ID do tile para destacar a posição atual do jogador

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
bool hasWon = false; // Novo: Variável para controlar se o jogador venceu

// ===========================================
// Estrutura para os Itens
// ===========================================
struct GameItem {
    int gridX;
    int gridY;
    GLuint textureID; // Agora armazena o ID da textura OpenGL diretamente
    bool collected;   // Se o item foi coletado (para futura lógica)
    int type;         // Novo: Para identificar o tipo de item (pode ser usado para pontuação, etc.)
};

std::vector<GameItem> gameItems;

// IDs das texturas para os cristais individuais
GLuint darkRedCrystalTexture;
GLuint whiteCrystalTexture;
GLuint yellowCrystalTexture;
GLuint gameOverTileTexture; // Nova textura para o tile de game over

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
// Propriedades dos Tiles
// ===========================================
std::map<int, bool> tileWalkableProperties; // tileID -> isWalkable
std::map<int, bool> tileGameOverProperties; // tileID -> isGameOver

bool loadTilePropertiesFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo de propriedades dos tiles: " << filename << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        int tileID;
        int isWalkableInt;
        int isGameOverInt = 0; // Padrão: não é tile de game over

        // Formato esperado: tileID isWalkable [isGameOver]
        if (!(ss >> tileID >> isWalkableInt)) {
            std::cerr << "Erro de formato na linha " << lineNumber << " do arquivo de propriedades dos tiles. Esperado: tileID isWalkable [isGameOver]. Linha: " << line << std::endl;
            continue;
        }

        // Tenta ler isGameOver opcionalmente
        if (ss >> isGameOverInt) {
            tileGameOverProperties[tileID] = (isGameOverInt == 1);
        } else {
            tileGameOverProperties[tileID] = false; // Se não especificado, não é tile de game over
        }

        tileWalkableProperties[tileID] = (isWalkableInt == 1);
    }
    file.close();
    std::cout << "Propriedades dos tiles carregadas com sucesso de: " << filename << std::endl;
    return true;
}

// Função para verificar se um tile é caminhável
bool isTileWalkable(int tileID) {
    auto it = tileWalkableProperties.find(tileID);
    if (it != tileWalkableProperties.end()) {
        return it->second; // Retorna a propriedade de caminhabilidade definida
    }
    // Se o tileID não estiver no mapa, assume que é intransitável por padrão (segurança)
    std::cerr << "Aviso: Tile ID " << tileID << " nao encontrado nas propriedades. Assumindo como intransitavel." << std::endl;
    return false;
}

// Função para verificar se um tile é um tile de game over
bool isTileGameOver(int tileID) {
    auto it = tileGameOverProperties.find(tileID);
    if (it != tileGameOverProperties.end()) {
        return it->second;
    }
    return false; // Se não estiver definido, não é um tile de game over
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
            // Armazena o ID original do tile
            originalMapData[i][j] = mapData[i][j];

            // Garante que a posição inicial do jogador não seja um tile intransitável
            if (i == playerGridY && j == playerGridX) {
                if (!isTileWalkable(mapData[i][j]) || isTileGameOver(mapData[i][j])) {
                    std::cerr << "Erro: O tile de inicio do personagem (" << mapData[i][j]
                              << ") no centro do mapa [" << i << "][" << j
                              << "] e intransitavel ou de game over. Por favor, ajuste o mapa." << std::endl;
                    return false;
                }
            }
        }
    }

    file.close();
    std::cout << "Mapa carregado com sucesso de: " << filename << std::endl;
    return true;
}

// Nova função para carregar itens de um arquivo
bool loadItemsFromFile(const std::string& filename) {
    // Limpa os itens existentes antes de carregar novos
    gameItems.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo de itens: " << filename << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        // Ignora linhas vazias ou comentários
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        int gridX, gridY, textureType;
        // Lê as coordenadas X, Y e o tipo de textura
        if (!(ss >> gridX >> gridY >> textureType)) {
            std::cerr << "Erro de formato na linha " << lineNumber << " do arquivo de itens: " << line << std::endl;
            continue;
        }

        // Verifica se as coordenadas estão dentro dos limites do mapa
        if (gridX < 0 || gridX >= MAP_W || gridY < 0 || gridY >= MAP_H) {
            std::cerr << "Aviso: Item na linha " << lineNumber << " fora dos limites do mapa (" << gridX << ", " << gridY << "). Ignorando." << std::endl;
            continue;
        }

        // **Validação de caminhabilidade antes da posição do item**
        if (!isTileWalkable(mapData[gridY][gridX]) || isTileGameOver(mapData[gridY][gridX]) ||
            (gridX == playerGridX && gridY == playerGridY)) {
            std::cerr << "Aviso: Item na linha " << lineNumber << " em posicao intransitavel, de game over ou na posicao do jogador (" << gridX << ", " << gridY << "). Ignorando." << std::endl;
            continue;
        }

        GameItem newItem;
        newItem.gridX = gridX;
        newItem.gridY = gridY;
        newItem.collected = false;
        newItem.type = textureType; // Armazena o tipo do item, caso precise para pontuação, etc.

        // Atribui a textura com base no textureType
        switch (textureType) {
            case 0: // Exemplo: Tipo 0 para Dark Red Crystal
                newItem.textureID = darkRedCrystalTexture;
                break;
            case 1: // Exemplo: Tipo 1 para White Crystal
                newItem.textureID = whiteCrystalTexture;
                break;
            case 2: // Exemplo: Tipo 2 para Yellow Crystal
                newItem.textureID = yellowCrystalTexture;
                break;
            default:
                std::cerr << "Aviso: Tipo de textura invalido (" << textureType << ") na linha " << lineNumber << ". Usando Dark Red Crystal por padrao." << std::endl;
                newItem.textureID = darkRedCrystalTexture; // Padrão
                break;
        }
        gameItems.push_back(newItem);
    }

    file.close();
    std::cout << "Carregados " << gameItems.size() << " itens do arquivo: " << filename << std::endl;
    return true;
}

// ===========================================
// Nova função para verificar se todos os itens foram coletados
// ===========================================
bool areAllItemsCollected() {
    for (const auto& item : gameItems) {
        if (!item.collected) {
            return false; // If an uncollected item is found, return false
        }
    }
    return true; // All items have been collected
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

    // 1. Carrega as propriedades dos tiles PRIMEIRO
    if (!loadTilePropertiesFromFile("tile_properties.txt")) {
        return -1;
    }

    // 2. Carrega o mapa (que agora usa as propriedades dos tiles)
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
    gameOverTileTexture = loadTexture("resources/game_over_tile.png"); // Carregue sua textura de game over aqui

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
    if (gameOverTileTexture == 0) {
        std::cerr << "Aviso: Nao foi possivel carregar a textura do tile de Game Over (game_over_tile.png). O jogo continuara sem essa visualizacao especifica." << std::endl;
    }


    // 3. Carrega os itens do arquivo. A validação de posição agora usa as propriedades carregadas.
    if (!loadItemsFromFile("items.txt")) {
        // Se o carregamento do arquivo falhar, você pode, opcionalmente, lidar com isso aqui.
        // Por exemplo, gerar itens aleatoriamente se o arquivo não for encontrado ou estiver vazio.
        std::cerr << "Nao foi possivel carregar os itens do arquivo. Certifique-se de que 'items.txt' existe e esta formatado corretamente." << std::endl;
        // return -1; // Comente para permitir continuar sem itens ou com itens gerados aleatoriamente
    }

    // Armazena a posição anterior do jogador para restaurar o tile
    int lastPlayerGridX = playerGridX;
    int lastPlayerGridY = playerGridY;

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

    // This is your main game loop. All game logic and rendering should happen inside this one loop.
    while (!glfwWindowShouldClose(win))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - g_lastFrameTime;
        g_lastFrameTime = currentTime;

        glfwPollEvents();

        // Game Logic: Only process input and update game state if the game is not over and not won
        if (!isGameOver && !hasWon) {
            int newPlayerGridX = playerGridX;
            int newPlayerGridY = playerGridY;
            bool playerAttemptedMove = false;
            int newPlayerAnimY = playerAnimationFrameY;

            // Player movement input handling
            if (g_keysPressed[GLFW_KEY_W] && !g_keysHandled[GLFW_KEY_W]) { // North-East
                newPlayerGridY++; newPlayerGridX++;
                newPlayerAnimY = 2; // Adjust animation row for this direction
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_W] = true;
            }
            if (g_keysPressed[GLFW_KEY_E] && !g_keysHandled[GLFW_KEY_E]) { // North-East (alternative for WASD-like feel)
                newPlayerGridY++;
                newPlayerAnimY = 2;
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_E] = true;
            } else if (g_keysPressed[GLFW_KEY_S] && !g_keysHandled[GLFW_KEY_S]) { // South-West (Forward)
                newPlayerGridY--; newPlayerGridX--;
                newPlayerAnimY = 3; // Adjust animation row for this direction
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_S] = true;
            } else if (g_keysPressed[GLFW_KEY_A] && !g_keysHandled[GLFW_KEY_A]) { // South-East
                newPlayerGridY--; newPlayerGridX++;
                newPlayerAnimY = 1; // Adjust animation row for this direction
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_A] = true;
            } else if (g_keysPressed[GLFW_KEY_Q] && !g_keysHandled[GLFW_KEY_Q]) { // East (diagonal movement helper)
                newPlayerGridX++; // This might need review for isometric movement with Q, E, Z, X
                newPlayerAnimY = 1;
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_Q] = true;
            }
            else if (g_keysPressed[GLFW_KEY_Z] && !g_keysHandled[GLFW_KEY_Z]) { // North-West (diagonal movement helper)
                newPlayerGridY--; // This might need review for isometric movement with Q, E, Z, X
                newPlayerAnimY = 1; // Assuming same animation row as 'A' or 'Q' for a similar direction
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_Z] = true;
            }
            else if (g_keysPressed[GLFW_KEY_X] && !g_keysHandled[GLFW_KEY_X]) { // South-West (diagonal movement helper)
                newPlayerGridX--; // This might need review for isometric movement with Q, E, Z, X
                newPlayerAnimY = 3; // Assuming same animation row as 'S' or 'A' for a similar direction
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_X] = true;
            }
            else if (g_keysPressed[GLFW_KEY_D] && !g_keysHandled[GLFW_KEY_D]) { // North-West
                newPlayerGridY++; newPlayerGridX--;
                newPlayerAnimY = 0; // Adjust animation row for this direction
                playerAttemptedMove = true;
                g_keysHandled[GLFW_KEY_D] = true;
            }

            if (playerAttemptedMove) {
                // Check map boundaries and impassable tiles
                if (newPlayerGridX >= 0 && newPlayerGridX < MAP_W &&
                    newPlayerGridY >= 0 && newPlayerGridY < MAP_H)
                {
                    // Usando a nova função isTileWalkable
                    if (isTileWalkable(mapData[newPlayerGridY][newPlayerGridX]))
                    {
                        // Se o jogador se moveu, restaure o tile anterior
                        if (playerGridX != newPlayerGridX || playerGridY != newPlayerGridY) {
                            mapData[lastPlayerGridY][lastPlayerGridX] = originalMapData[lastPlayerGridY][lastPlayerGridX];
                        }

                        // Atualiza a posição do jogador
                        playerGridX = newPlayerGridX;
                        playerGridY = newPlayerGridY;
                        playerAnimationFrameY = newPlayerAnimY;

                        // Armazena a nova posição como a "última" para o próximo frame
                        lastPlayerGridX = playerGridX;
                        lastPlayerGridY = playerGridY;

                        // Mude o tile atual para o HIGHLIGHT_TILE_ID (6)
                        mapData[playerGridY][playerGridX] = HIGHLIGHT_TILE_ID;

                        // Usando a nova função isTileGameOver
                        if (isTileGameOver(originalMapData[playerGridY][playerGridX])) { // Use originalMapData para verificar se é um tile de game over, já que mapData é alterado
                            isGameOver = true;
                            std::cout << "Game Over! Voce tocou em um tile de game over!" << std::endl;
                        }

                        // Item collection logic
                        for (auto& item : gameItems) {
                            if (!item.collected && item.gridX == playerGridX && item.gridY == playerGridY) {
                                item.collected = true;
                                std::cout << "Item coletado! Tipo: " << item.type << std::endl;
                                // Check for win condition immediately after collecting an item
                                if (areAllItemsCollected()) {
                                    hasWon = true;
                                    std::cout << "Parabens! Voce coletou todos os itens e venceu o jogo!" << std::endl;
                                }
                            }
                        }
                    } else {
                        // Se o movimento for bloqueado, ainda atualiza a direção da animação
                        playerAnimationFrameY = newPlayerAnimY;
                    }
                } else {
                    // Se o movimento for fora dos limites, ainda atualiza a direção da animação
                    playerAnimationFrameY = newPlayerAnimY;
                }
                playerAnimationFrameX = (int)(currentTime / g_animationSpeed) % PLAYER_SPRITE_RUN_FRAMES;
            } else {
                playerAnimationFrameX = 0; // Reset para o frame ocioso se não houver movimento
                // Garante que o tile atual do jogador ainda esteja destacado, mesmo se não houver movimento
                mapData[playerGridY][playerGridX] = HIGHLIGHT_TILE_ID;
            }
        } else { // Se o jogo estiver encerrado (game over ou vitória)
            // Garante que o tile onde o jogador parou retorne ao original ou mostre o tile de game over
            if (isGameOver) {
                mapData[playerGridY][playerGridX] = originalMapData[playerGridY][playerGridX];
                if (isTileGameOver(originalMapData[playerGridY][playerGridX])) {
                    // Opcional: Se houver um tile visual específico para "Game Over", você pode defini-lo aqui
                    // mapData[playerGridY][playerGridX] = ID_DO_TILE_DE_GAME_OVER;
                }
            } else if (hasWon) {
                mapData[playerGridY][playerGridX] = originalMapData[playerGridY][playerGridX];
            }
        }


        // Rendering code
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

        // Rendering Map Tiles
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

        // Rendering Items
        glUniform2f(locTS, 1.0f, 1.0f);
        glUniform2f(locTO, 0.0f, 0.0f);

        for (const auto& item : gameItems) {
            if (!item.collected) { // Render only if not collected
                float itemWorldX = (item.gridY - item.gridX) * halfW + mapOriginOffset.x;
                float itemWorldY = (item.gridY + item.gridX) * halfH + mapOriginOffset.y + (tileH * 0.5f); // Adjust to sit on top of the tile
                float itemZ = (item.gridY + item.gridX) * 0.001f + 0.2f; // Z-order: higher than tile, lower than player

                glm::mat4 M_item = glm::translate(glm::mat4(1.0f), glm::vec3(itemWorldX, itemWorldY, itemZ))
                                   * glm::scale(glm::mat4(1.0f), glm::vec3(ITEM_SINGLE_SPRITE_W, ITEM_SINGLE_SPRITE_H, 1));
                glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(M_item));

                glBindTexture(GL_TEXTURE_2D, item.textureID); // Bind the specific item texture
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }


        // Rendering Player Character
        if (!isGameOver && !hasWon) { // Only render player if game is still active
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

        // Rendering Player Tile Outline
        if (!isGameOver && !hasWon) { // Only show outline if game is active
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glUniform1i(locOL, 1);
            glUniform4f(locCLR, 1, 1, 1, 1);
            glLineWidth(2.0f);

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