#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Incluindo a implementação da stb_image direto na pasta src conforme ajustamos!
#include "stb_image.cpp" 

using namespace std;
using namespace glm;

const GLint WIDTH = 800, HEIGHT = 600;

// Variáveis de controle do adesivo
float adX = 400.0f, adY = 300.0f;
float adScale = 150.0f;
float adAngle = 0.0f;

// Variáveis de controle de Filtro e Flip
int filterType = 0; // 0 = Normal, 1 = Tons de Cinza, 2 = Invertido
bool flipH = false;
bool flipV = false;

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
uniform bool flipH;
uniform bool flipV;
uniform int filterType;

void main() {
    // 1. Aplica o Flip nas coordenadas de textura
    vec2 uv = TexCoord;
    if (flipH) uv.x = 1.0 - uv.x;
    if (flipV) uv.y = 1.0 - uv.y;

    // 2. Lê a cor da textura
    vec4 color = texture(spriteTexture, uv);

    // 3. Descarta fragmentos transparentes (Transparência do Adesivo)
    if(color.a < 0.1) {
        discard;
    }

    // 4. Aplica os Filtros de Imagem
    if (filterType == 1) {
        // Tons de cinza (Luminância)
        float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
        color = vec4(gray, gray, gray, color.a);
    } else if (filterType == 2) {
        // Inverter cores (Negativo)
        color = vec4(1.0 - color.rgb, color.a);
    }

    FragColor = color;
}
)";

GLuint loadTexture(const char* file_name) {
    GLuint tex;
    int x, y, n;
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, 4);
    if (!image_data) {
        cerr << "ERRO: Não carregou " << file_name << endl;
        return 0;
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    return tex;
}

GLuint setupQuad() {
    GLfloat vertices[] = {
        // Posicionamento centralizado (Origem no meio do adesivo para rotacionar certo)
        -0.5f,  0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  1.0f, 0.0f
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        // Movimento
        if (key == GLFW_KEY_UP) adY -= 10.0f;
        if (key == GLFW_KEY_DOWN) adY += 10.0f;
        if (key == GLFW_KEY_LEFT) adX -= 10.0f;
        if (key == GLFW_KEY_RIGHT) adX += 10.0f;
        
        // Rotação e Escala
        if (key == GLFW_KEY_Q) adAngle -= 0.1f;
        if (key == GLFW_KEY_E) adAngle += 0.1f;
        if (key == GLFW_KEY_W) adScale += 10.0f;
        if (key == GLFW_KEY_S) adScale -= 10.0f;
    }
    
    // Ações de toque único (Flips e Filtros)
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_H) flipH = !flipH;
        if (key == GLFW_KEY_V) flipV = !flipV;
        if (key == GLFW_KEY_F) {
            filterType++;
            if (filterType > 2) filterType = 0; // Alterna entre 0, 1 e 2
        }
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio: Adesivos e Filtros", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    GLuint quadVAO = setupQuad();

    GLuint texFundo = loadTexture("../assets/fundo.png");
    GLuint texAdesivo = loadTexture("../assets/personagem.png");

    mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));

        // --- DESENHA O FUNDO ---
        // O fundo não tem flip nem filtro
        glUniform1i(glGetUniformLocation(shaderProgram, "flipH"), false);
        glUniform1i(glGetUniformLocation(shaderProgram, "flipV"), false);
        glUniform1i(glGetUniformLocation(shaderProgram, "filterType"), 0);

        mat4 modelFundo = mat4(1.0f);
        modelFundo = translate(modelFundo, vec3(WIDTH / 2.0f, HEIGHT / 2.0f, 0.0f));
        modelFundo = scale(modelFundo, vec3(WIDTH, HEIGHT, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(modelFundo));
        glBindTexture(GL_TEXTURE_2D, texFundo);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // --- DESENHA O ADESIVO (Personagem) ---
        // Envia as opções escolhidas pelo usuário para o Shader do adesivo
        glUniform1i(glGetUniformLocation(shaderProgram, "flipH"), flipH);
        glUniform1i(glGetUniformLocation(shaderProgram, "flipV"), flipV);
        glUniform1i(glGetUniformLocation(shaderProgram, "filterType"), filterType);

        mat4 modelAdesivo = mat4(1.0f);
        modelAdesivo = translate(modelAdesivo, vec3(adX, adY, 0.0f));
        modelAdesivo = rotate(modelAdesivo, adAngle, vec3(0.0f, 0.0f, 1.0f));
        modelAdesivo = scale(modelAdesivo, vec3(adScale, adScale, 1.0f));
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(modelAdesivo));
        glBindTexture(GL_TEXTURE_2D, texAdesivo);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}