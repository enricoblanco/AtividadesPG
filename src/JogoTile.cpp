#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// A biblioteca stb_image implementada direto da pasta Common
#include "stb_image.cpp"

using namespace std;
using namespace glm;

const GLint WIDTH = 800, HEIGHT = 600;
const int TILE_SIZE = 50;
const int MAP_ROWS = 12;
const int MAP_COLS = 16;

// --- ESTRUTURA DO TILEMAP ---
// 0 = Chão Livre, 1 = Parede/Obstáculo
int tileMap[MAP_ROWS][MAP_COLS] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1,1,1,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1},
    {1,0,1,0,1,1,1,1,1,0,1,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,1,0,1,0,1,0,1,0,1,1,1,1,0,1},
    {1,0,1,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// --- VARIÁVEIS DO JOGADOR ---
float playerX = 1 * TILE_SIZE;
float playerY = 1 * TILE_SIZE;
float playerSpeed = 3.0f;

// --- VARIÁVEIS DO INIMIGO ---
float enemyX = 14 * TILE_SIZE;
float enemyY = 1 * TILE_SIZE;
float enemySpeed = 2.0f;
int enemyDirection = -1; // -1 = Esquerda, 1 = Direita

// --- VARIÁVEIS DO ITEM E JOGO ---
bool itemColetado = false;
int estadoJogo = 0; // 0 = Jogando, 1 = Venceu, 2 = Perdeu

// --- SHADERS BÁSICOS ---
const GLchar* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 projection;
uniform mat4 model;
out vec2 TexCoord;
void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const GLchar* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D spriteTexture;
void main() {
    vec4 texColor = texture(spriteTexture, TexCoord);
    if(texColor.a < 0.1) discard; // Garante transparência
    FragColor = texColor;
}
)";

// --- FUNÇÕES DE SUPORTE (TEXTURA E QUAD) ---
GLuint loadTexture(const char* file_name) {
    GLuint tex;
    int x, y, n;
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, 4);
    if (!image_data) {
        cerr << "ERRO: Textura " << file_name << " nao encontrada!" << endl;
        return 0;
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    return tex;
}

GLuint setupQuad() {
    GLfloat vertices[] = {
        0.0f, 1.0f,  0.0f, 1.0f, 
        1.0f, 0.0f,  1.0f, 0.0f, 
        0.0f, 0.0f,  0.0f, 0.0f, 
        0.0f, 1.0f,  0.0f, 1.0f, 
        1.0f, 1.0f,  1.0f, 1.0f, 
        1.0f, 0.0f,  1.0f, 0.0f  
    };
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    return VAO;
}

void drawSprite(GLuint shader, GLuint quadVAO, GLuint texture, float x, float y, float w, float h) {
    mat4 model = mat4(1.0f);
    model = translate(model, vec3(x, y, 0.0f));
    model = scale(model, vec3(w, h, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(model));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// --- FUNÇÕES DE COLISÃO ---
bool checkWallCollision(float x, float y, float size) {
    int leftTile = x / TILE_SIZE;
    int rightTile = (x + size - 1) / TILE_SIZE;
    int topTile = y / TILE_SIZE;
    int bottomTile = (y + size - 1) / TILE_SIZE;

    if (tileMap[topTile][leftTile] == 1 || tileMap[topTile][rightTile] == 1 ||
        tileMap[bottomTile][leftTile] == 1 || tileMap[bottomTile][rightTile] == 1) {
        return true; 
    }
    return false;
}

bool checkAABB(float x1, float y1, float s1, float x2, float y2, float s2) {
    return x1 < x2 + s2 && x1 + s1 > x2 && y1 < y2 + s2 && y1 + s1 > y2;
}

// --- FUNÇÃO PRINCIPAL ---
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio: Jogo Tilemap 2D", nullptr, nullptr);
    if (!window) {
        cout << "Falha ao criar a janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Falha ao inicializar o GLAD" << endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint quadVAO = setupQuad();

    GLuint texChao   = loadTexture("../assets/tile_chao.png");
    GLuint texParede = loadTexture("../assets/tile_parede.png");
    GLuint texPlayer = loadTexture("../assets/personagem.png");
    GLuint texItem   = loadTexture("../assets/item.png");
    GLuint texEnemy  = loadTexture("../assets/inimigo.png"); // Lembre-se de baixar essa imagem!

    mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));

        if (estadoJogo == 0) {
            // --- MOVIMENTO DO JOGADOR COM COLISÃO ---
            float nextX = playerX;
            float nextY = playerY;

            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) nextX += playerSpeed;
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  nextX -= playerSpeed;
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    nextY -= playerSpeed;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  nextY += playerSpeed;

            if (!checkWallCollision(nextX, playerY, 40.0f)) playerX = nextX;
            if (!checkWallCollision(playerX, nextY, 40.0f)) playerY = nextY;

            // --- MOVIMENTO DO INIMIGO ---
            float nextEnemyX = enemyX + (enemySpeed * enemyDirection);
            if (checkWallCollision(nextEnemyX, enemyY, 40.0f)) {
                enemyDirection *= -1; 
            } else {
                enemyX = nextEnemyX;
            }

            // --- LÓGICA DE VITÓRIA E DERROTA ---
            float itemX = 14 * TILE_SIZE;
            float itemY = 9 * TILE_SIZE;

            if (!itemColetado && checkAABB(playerX, playerY, 40.0f, itemX, itemY, 40.0f)) {
                itemColetado = true;
                estadoJogo = 1; 
                cout << "Evidencia coletada! VOCE VENCEU!" << endl;
            }

            if (checkAABB(playerX, playerY, 40.0f, enemyX, enemyY, 40.0f)) {
                estadoJogo = 2; 
                cout << "O inimigo te pegou! FIM DE JOGO!" << endl;
            }
        }

        // --- RENDERIZAÇÃO DO TILEMAP ---
        for (int row = 0; row < MAP_ROWS; row++) {
            for (int col = 0; col < MAP_COLS; col++) {
                float posX = col * TILE_SIZE;
                float posY = row * TILE_SIZE;

                if (tileMap[row][col] == 1) {
                    drawSprite(shaderProgram, quadVAO, texParede, posX, posY, TILE_SIZE, TILE_SIZE);
                } else {
                    drawSprite(shaderProgram, quadVAO, texChao, posX, posY, TILE_SIZE, TILE_SIZE);
                }
            }
        }

        // --- RENDERIZAÇÃO DAS ENTIDADES ---
        if (!itemColetado) {
            drawSprite(shaderProgram, quadVAO, texItem, 14 * TILE_SIZE, 9 * TILE_SIZE, 40.0f, 40.0f);
        }
        
        drawSprite(shaderProgram, quadVAO, texEnemy, enemyX, enemyY, 40.0f, 40.0f);
        drawSprite(shaderProgram, quadVAO, texPlayer, playerX, playerY, 40.0f, 40.0f);

        glfwSwapBuffers(window);
    }
    
    glfwTerminate();
    return 0;
}