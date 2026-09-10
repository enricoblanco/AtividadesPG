#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

const GLuint WIDTH = 800, HEIGHT = 600;

// ==========================================
// ESTRUTURAS E VARIÁVEIS GLOBAIS
// ==========================================

// Estado atual do programa (1 = Exercícios 1 e 2 | 2 = Exercício 3)
int current_mode = 1;

// --- Dados da Parte 1 ---
vector<GLuint> vaosTriangulosEstaticos;

// --- Dados da Parte 2 ---
struct Triangle {
    vec3 position;
    vec3 color;
};
vector<Triangle> trianglesDinamicos;
GLuint standardVAO;

// ==========================================
// SHADERS (Unificados para servir às duas partes)
// ==========================================

const GLchar* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
uniform mat4 model; // Usada para mover o triângulo na Parte 2 (ou mantida intacta na Parte 1)
void main() {
    gl_Position = model * vec4(position, 1.0);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 330 core
uniform vec3 uniformColor; // Cor enviada via código
out vec4 fragColor;
void main() {
    fragColor = vec4(uniformColor, 1.0);
}
)";

// ==========================================
// CALLBACKS (Inputs)
// ==========================================

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
        
    // Alterna para a Parte 1
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        current_mode = 1;
        cout << "Modo 1: Triangulos Estaticos (Parte 1)" << endl;
    }
    
    // Alterna para a Parte 2
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        current_mode = 2;
        cout << "Modo 2: Triangulos Dinamicos via Mouse (Parte 2)" << endl;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Só permite criar novos triângulos se estivermos no modo 2
    if (current_mode == 2 && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float ndc_x = (xpos / WIDTH) * 2.0f - 1.0f;
        float ndc_y = 1.0f - (ypos / HEIGHT) * 2.0f;

        Triangle newTriangle;
        newTriangle.position = vec3(ndc_x, ndc_y, 0.0f);
        
        float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        newTriangle.color = vec3(r, g, b);

        trianglesDinamicos.push_back(newTriangle);
    }
}

// ==========================================
// FUNÇÕES DE GEOMETRIA E SHADER
// ==========================================

GLuint setupShader() {
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

    glDeleteShader(vs);
    glDeleteShader(fs);
    return shaderProgram;
}

// Função solicitada no Exercício 1
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    GLfloat vertices[] = {
        x0, y0, 0.0f,
        x1, y1, 0.0f,
        x2, y2, 0.0f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

// Função auxiliar para o VAO padrão do Exercício 3
GLuint setupStandardGeometry() {
    return createTriangle(-0.1f, -0.1f, 0.1f, -0.1f, 0.0f, 0.1f);
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================

int main() {
    srand(static_cast<unsigned>(time(0)));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio Modulo 2 - Completo", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    GLuint shaderID = setupShader();
    
    // Setup Parte 1: Instancia 5 triângulos fixos
    vaosTriangulosEstaticos.push_back(createTriangle(-0.9f, -0.9f, -0.7f, -0.9f, -0.8f, -0.7f));
    vaosTriangulosEstaticos.push_back(createTriangle(-0.5f, -0.5f, -0.3f, -0.5f, -0.4f, -0.3f));
    vaosTriangulosEstaticos.push_back(createTriangle(-0.1f, -0.1f,  0.1f, -0.1f,  0.0f,  0.1f));
    vaosTriangulosEstaticos.push_back(createTriangle( 0.3f,  0.3f,  0.5f,  0.3f,  0.4f,  0.5f));
    vaosTriangulosEstaticos.push_back(createTriangle( 0.7f,  0.7f,  0.9f,  0.7f,  0.8f,  0.9f));

    // Setup Parte 2: Instancia o VAO padrão
    standardVAO = setupStandardGeometry();

    glUseProgram(shaderID);
    
    cout << "=== DESAFIO MODULO 2 ===" << endl;
    cout << "Pressione '1' para ver a Parte 1 (Estaticos)" << endl;
    cout << "Pressione '2' para ver a Parte 2 (Mouse)" << endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (current_mode == 1) {
            // === RENDERIZAÇÃO DA PARTE 1 ===
            // Matriz identidade (sem movimento) e cor fixa (ex: verde)
            mat4 model = mat4(1.0f); 
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
            glUniform3f(glGetUniformLocation(shaderID, "uniformColor"), 0.0f, 1.0f, 0.0f);

            for (GLuint vao : vaosTriangulosEstaticos) {
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
        } 
        else if (current_mode == 2) {
            // === RENDERIZAÇÃO DA PARTE 2 ===
            glBindVertexArray(standardVAO);
            for (const Triangle& tri : trianglesDinamicos) {
                mat4 model = mat4(1.0f);
                model = translate(model, tri.position);
                
                glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
                glUniform3f(glGetUniformLocation(shaderID, "uniformColor"), tri.color.r, tri.color.g, tri.color.b);

                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}