#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

// GLAD (Padrão da disciplina)
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Variáveis globais para acesso dentro do callback do mouse
GLuint VAO, VBO;
vector<GLfloat> vertices;
vec3 current_color(1.0f, 0.0f, 0.0f);

// Código fonte do Vertex Shader (Simples: Posição + Cor)
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 projection;
out vec3 vertexColor;
void main()
{
    gl_Position = projection * vec4(position, 1.0);
    vertexColor = color;
}
)";

// Código fonte do Fragment Shader (Simples: Pinta com a cor do vértice)
const GLchar *fragmentShaderSource = R"(
#version 400
in vec3 vertexColor;
out vec4 fragColor;
void main()
{
    fragColor = vec4(vertexColor, 1.0);
}
)";

// Protótipos
int setupShader();
void setupGeometry();
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

int main()
{
    srand(static_cast<unsigned>(time(0))); // Semente para cores aleatórias

    glfwInit();
    
    // Configurações base do GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Tarefa: Triangulos com Mouse", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Registra a função de clique do mouse
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Inicializa o GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    // Configura Shaders e Buffers
    GLuint shaderID = setupShader();
    setupGeometry();

    glUseProgram(shaderID);

    // Projeção Ortográfica (Mapeia mundo para a tela em pixels)
    mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    // Game Loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        
        // Desenha os triângulos se houver vértices (6 floats por vértice)
        if (vertices.size() > 0) {
            glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// Callback executado a cada clique do mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Verifica se é o início de um novo triângulo para sortear a cor
        int num_vertices_atuais = vertices.size() / 6;
        if (num_vertices_atuais % 3 == 0) {
            float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            current_color = vec3(r, g, b);
        }

        // Adiciona posição (x, y, z)
        vertices.push_back((float)xpos);
        vertices.push_back((float)ypos);
        vertices.push_back(0.0f);

        // Adiciona cor (r, g, b)
        vertices.push_back(current_color.r);
        vertices.push_back(current_color.g);
        vertices.push_back(current_color.b);

        // Atualiza a GPU com os novos dados
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);
    }
}

// Prepara um VBO/VAO vazio mas pronto para receber dados dinâmicos
void setupGeometry()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    // GL_DYNAMIC_DRAW é usado pois os dados mudarão a cada clique
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Atributo 0: Posição (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: Cor (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Compila os shaders
int setupShader()
{
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

    return shaderProgram;
}