#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <algorithm>

#ifdef _WIN64
#include <Windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glew.h>

#include "ogldev_util.h"
#include "ogldev_basic_glfw_camera.h"
#include "ogldev_glfw.h"

#include "demo_config.h"
#include "texture_config.h"
#include "midpoint_disp_terrain.h"

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void CursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode);

static int g_seed = 4428;
unsigned int m_numMainBodyIndices;
unsigned int m_numTailIndices;
extern int gShowPoints;

// Simplified PlayerCube class
class CubeTechnique
{
public:
    CubeTechnique() : m_shaderProg(0) {}

    ~CubeTechnique() {
        if (m_shaderProg != 0) {
            glDeleteProgram(m_shaderProg);
        }
    }

    bool Init() {
        // Vertex shader source
        const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 gVP;
        uniform mat4 gModel;
        
        void main()
        {
            gl_Position = gVP * gModel * vec4(aPos, 1.0);
        }
        )";

        // Fragment shader source
        const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec3 gColor;
        
        void main()
        {
            FragColor = vec4(gColor, 1.0);
        }
        )";

        // Create and compile vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        // Check for vertex shader compile errors
        GLint success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            printf("Vertex shader compilation failed: %s\n", infoLog);
            return false;
        }

        // Create and compile fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // Check for fragment shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            printf("Fragment shader compilation failed: %s\n", infoLog);
            return false;
        }

        // Create shader program
        m_shaderProg = glCreateProgram();
        glAttachShader(m_shaderProg, vertexShader);
        glAttachShader(m_shaderProg, fragmentShader);
        glLinkProgram(m_shaderProg);

        // Check for linking errors
        glGetProgramiv(m_shaderProg, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(m_shaderProg, 512, NULL, infoLog);
            printf("Shader program linking failed: %s\n", infoLog);
            return false;
        }

        // Clean up shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Get uniform locations
        m_vpLoc = glGetUniformLocation(m_shaderProg, "gVP");
        m_modelLoc = glGetUniformLocation(m_shaderProg, "gModel");
        m_colorLoc = glGetUniformLocation(m_shaderProg, "gColor");

        return true;
    }

    void Enable() {
        glUseProgram(m_shaderProg);
    }

    void SetVP(const Matrix4f& VP) {
        glUniformMatrix4fv(m_vpLoc, 1, GL_TRUE, (const GLfloat*)VP.m);
    }

    void SetModel(const Matrix4f& Model) {
        glUniformMatrix4fv(m_modelLoc, 1, GL_TRUE, (const GLfloat*)Model.m);
    }

    void SetColor(const Vector3f& Color) {
        glUniform3f(m_colorLoc, Color.x, Color.y, Color.z);
    }

    void SetColor(float r, float g, float b) {
        glUniform3f(m_colorLoc, r, g, b);
    }

private:
    GLuint m_shaderProg;
    GLint m_vpLoc;
    GLint m_modelLoc;
    GLint m_colorLoc;
};

// Modern PlayerCube class
class Bird
{
public:
    Bird() : m_position(0.0f, 0.0f, 0.0f), m_velocity(0.0f, 0.0f, 0.0f), m_wingPhase(0.0f), m_minHeight(250.0f) {}
    Bird(const Vector3f& pos, const Vector3f& vel)
        : m_position(pos), m_velocity(vel), m_wingPhase((float)(rand() % 360)), m_minHeight(pos.y) {
        // Zapisz startową wysokość jako minimalną
        if (m_minHeight < 250.0f) m_minHeight = 250.0f; // Minimum 200 jednostek
    }

    void Update(float deltaTime, float worldSize)
    {
        // Aktualizuj pozycję
        m_position = m_position + (m_velocity * deltaTime);

        // Aktualizuj fazę skrzydeł dla animacji
        m_wingPhase += 200.0f * deltaTime; // prędkość machania skrzydłami
        if (m_wingPhase > 360.0f) m_wingPhase -= 360.0f;

        // Sprawdź granice świata i odbij ptaka
        if (m_position.x <= 50.0f || m_position.x >= worldSize - 50.0f) {
            m_velocity.x = -m_velocity.x;
            m_position.x = (m_position.x <= 50.0f) ? 60.0f : worldSize - 60.0f;
        }
        if (m_position.z <= 50.0f || m_position.z >= worldSize - 50.0f) {
            m_velocity.z = -m_velocity.z;
            m_position.z = (m_position.z <= 50.0f) ? 60.0f : worldSize - 60.0f;
        }

        // Utrzymuj wysokość - NIE MOŻE zejść poniżej startowej wysokości
        if (m_position.y < m_minHeight) {
            m_velocity.y = abs(m_velocity.y); // skieruj w górę
            m_position.y = m_minHeight;
        }
        if (m_position.y > 600.0f) {
            m_velocity.y = -abs(m_velocity.y); // skieruj w dół
            m_position.y = 600.0f;
        }

        // Dodaj losowe zmiany w locie dla bardziej naturalnego ruchu
        if (rand() % 100 < 2) { // 2% szansy na zmianę co klatkę
            m_velocity.x += (-5.0f + (float)(rand() % 10));
            m_velocity.z += (-5.0f + (float)(rand() % 10));
            m_velocity.y += (-2.0f + (float)(rand() % 4));

            // Ograniczenia prędkości
            if (m_velocity.x > 40.0f) m_velocity.x = 40.0f;
            if (m_velocity.x < -40.0f) m_velocity.x = -40.0f;
            if (m_velocity.z > 40.0f) m_velocity.z = 40.0f;
            if (m_velocity.z < -40.0f) m_velocity.z = -40.0f;
            if (m_velocity.y > 10.0f) m_velocity.y = 10.0f;
            if (m_velocity.y < -10.0f) m_velocity.y = -10.0f;
        }
    }

    void Render(const Matrix4f& VP, CubeTechnique& technique)
    {
        // Oblicz kąt obrotu na podstawie kierunku lotu
        float yaw = atan2(m_velocity.x, m_velocity.z) * 180.0f / M_PI;

        // Oblicz nachylenie skrzydeł - większa amplituda dla lepszej widoczności
        float wingAngle = sin(m_wingPhase * M_PI / 180.0f) * 25.0f; // ±25 stopni

        // KORPUS - większy rozmiar
        RenderBirdPart(VP, technique, m_position, Vector3f(2.0f, 1.0f, 3.0f), yaw, 0.0f, 0.0f);

        // LEWE SKRZYDŁO - większe i bardziej widoczne
        Vector3f leftWingPos = m_position + Vector3f(-2.5f, 0.2f, 0.0f);
        RenderBirdPart(VP, technique, leftWingPos, Vector3f(4.0f, 0.3f, 1.5f), yaw, wingAngle, 0.0f);

        // PRAWE SKRZYDŁO - większe i bardziej widoczne
        Vector3f rightWingPos = m_position + Vector3f(2.5f, 0.2f, 0.0f);
        RenderBirdPart(VP, technique, rightWingPos, Vector3f(4.0f, 0.3f, 1.5f), yaw, -wingAngle, 0.0f);

        // OGON - większy
        Vector3f tailPos = m_position + Vector3f(0.0f, 0.0f, -2.0f);
        RenderBirdPart(VP, technique, tailPos, Vector3f(1.2f, 0.3f, 2.0f), yaw, 0.0f, 0.0f);

        // GŁOWA - dodajmy głowę dla lepszego wyglądu
        Vector3f headPos = m_position + Vector3f(0.0f, 0.3f, 1.8f);
        RenderBirdPart(VP, technique, headPos, Vector3f(0.8f, 0.8f, 0.8f), yaw, 0.0f, 0.0f);
    }

private:
    void RenderBirdPart(const Matrix4f& VP, CubeTechnique& technique, const Vector3f& pos,
        const Vector3f& scale, float yaw, float pitch, float roll)
    {
        Matrix4f translation;
        translation.InitTranslationTransform(pos.x, pos.y, pos.z);

        Matrix4f rotationY;
        rotationY.InitRotateTransform(0.0f, yaw, 0.0f);

        Matrix4f rotationX;
        rotationX.InitRotateTransform(pitch, 0.0f, 0.0f);

        Matrix4f rotationZ;
        rotationZ.InitRotateTransform(0.0f, 0.0f, roll);

        Matrix4f scaleMatrix;
        scaleMatrix.InitScaleTransform(scale.x, scale.y, scale.z);

        Matrix4f modelMatrix = translation * rotationY * rotationX * rotationZ * scaleMatrix;
        technique.SetModel(modelMatrix);

        // Ciemnoszary/czarny kolor dla lepszej widoczności
        technique.SetColor(0.2f, 0.2f, 0.2f);

        // Renderuj prosty sześcian
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    Vector3f m_position;
    Vector3f m_velocity;
    float m_wingPhase;
    float m_minHeight; // NOWE: minimalna wysokość (startowa wysokość ptaka)
};




class PlayerCube
{
public:
    PlayerCube() : m_position(0.0f, 0.0f, 0.0f), m_size(25.0f), m_VAO(0), m_VBO(0), m_EBO(0), m_rotation(0.0f), m_rotationX(0.0f), m_rotationZ(0.0f)
    {
        InitBuffers();
        m_cubeTech.Init();
    }

    ~PlayerCube() {
        CleanupBuffers();
    }

    void SetPosition(const Vector3f& pos) { m_position = pos; }
    Vector3f GetPosition() const { return m_position; }
    void SetSize(float size) { m_size = size; }
    float GetSize() const { return m_size; }

    void SetRotation(float rotation) { m_rotation = rotation; }
    float GetRotation() const { return m_rotation; }

    void Rotate(float deltaAngle) {
        m_rotation += deltaAngle;
        while (m_rotation >= 360.0f) m_rotation -= 360.0f;
        while (m_rotation < 0.0f) m_rotation += 360.0f;
    }

    void SetRotationX(float rotation) { m_rotationX = rotation; }
    float GetRotationX() const { return m_rotationX; }
    void SetRotationZ(float rotation) { m_rotationZ = rotation; }
    float GetRotationZ() const { return m_rotationZ; }

    void RotateX(float deltaAngle) {
        m_rotationX += deltaAngle;
        while (m_rotationX >= 360.0f) m_rotationX -= 360.0f;
        while (m_rotationX < 0.0f) m_rotationX += 360.0f;
    }

    void RotateZ(float deltaAngle) {
        m_rotationZ += deltaAngle;
        while (m_rotationZ >= 360.0f) m_rotationZ -= 360.0f;
        while (m_rotationZ < 0.0f) m_rotationZ += 360.0f;
    }

    void Render(const Matrix4f& VP)
    {
        Matrix4f translation;
        translation.InitTranslationTransform(m_position.x, m_position.y, m_position.z);

        Matrix4f rotationY;
        rotationY.InitRotateTransform(0.0f, m_rotation + 180.0f, 0.0f); // Dodaj +180 stopni aby obrócić samolot

        Matrix4f rotationX;
        rotationX.InitRotateTransform(m_rotationX, 0.0f, 0.0f);

        Matrix4f rotationZ;
        rotationZ.InitRotateTransform(0.0f, 0.0f, m_rotationZ);

        Matrix4f scale;
        scale.InitScaleTransform(m_size, m_size, m_size);

        Matrix4f modelMatrix = translation * rotationY * rotationX * rotationZ * scale;

        m_cubeTech.Enable();
        m_cubeTech.SetVP(VP);
        m_cubeTech.SetModel(modelMatrix);

        glBindVertexArray(m_VAO);

        // Fuselage - główny korpus samolotu
        m_cubeTech.SetColor(0.8f, 0.8f, 0.9f);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Right Wing - =
        m_cubeTech.SetColor(0.3f, 0.5f, 0.9f);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)(36 * sizeof(unsigned int)));

        // Left Wing - =
        m_cubeTech.SetColor(0.3f, 0.5f, 0.9f);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)(72 * sizeof(unsigned int)));

        // Horizontal Tail - =
        m_cubeTech.SetColor(0.8f, 0.8f, 0.9f);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)(108 * sizeof(unsigned int)));

        // Vertical Tail - =
        m_cubeTech.SetColor(0.3f, 0.5f, 0.9);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)(144 * sizeof(unsigned int)));

        // Nose/Dziób - =
        m_cubeTech.SetColor(0.8f, 0.8f, 0.9f);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)(180 * sizeof(unsigned int))); // Nowy element

        glBindVertexArray(0);
        glUseProgram(0);
    }

private:
    void InitBuffers()
    {
        // Enhanced Airplane Model Dimensions (unit scale) - Larger and longer
        // Fuselage - Made longer and more rounded
        const float f_len = 3.0f;  // było 1.8f
        const float f_wid = 0.45f; // było 0.35f
        const float f_hgt = 0.45f; // było 0.35f

        const float hf_len = f_len / 2.0f;
        const float hf_wid = f_wid / 2.0f;
        const float hf_hgt = f_hgt / 2.0f;

        // Wings - Made bigger with more separation
        const float w_span_each = 1.2f; // było 0.9f
        const float w_chord = 0.9f;     // było 0.7f
        const float w_thick = 0.2f;     // było 0.15f
        const float hw_chord = w_chord / 2.0f;
        const float hw_thick = w_thick / 2.0f;
        const float wing_z_offset = 0.1f;

        

        // Horizontal Tail - Increased separation and size
        const float ht_span_each = 0.6f;  // było 0.45f
        const float ht_chord = 0.5f;      // było 0.4f
        const float ht_thick = 0.15f;     // było 0.12f
        const float ht_z_pos = -hf_len - ht_chord / 2.0f + 0.15f; // było + 0.05f
        const float ht_y_pos = -hf_hgt + ht_thick / 2.0f;

        // Vertical Tail - Increased separation and size
        const float vt_height = 0.8f;     // było 0.6f
        const float vt_width = 0.15f;     // było 0.12f
        const float vt_chord = 0.5f;      // było 0.4f
        const float vt_z_pos = -hf_len - vt_chord / 2.0f + 0.15f; // było + 0.05f
        const float vt_y_pos = hf_hgt - 0.02f;

        // Nose/Dziób - Połączony z fuselage
        const float nose_length = 0.5f;
        const float nose_width = f_wid * 0.8f; // Proporcjonalny do fuselage
        const float nose_height = f_hgt * 0.8f;
        const float nose_z_pos = hf_len + nose_length / 2.0f - 0.05f; // Lekko nakładający się

        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        // Helper function to add a rounded cuboid (approximated with multiple segments)
        auto AddRoundedCuboid = [&](const Vector3f& center, const Vector3f& half_dims, int segments = 8) {
            unsigned int currentVertexOffset = vertices.size() / 3;

            // For rounded fuselage, we'll create an elliptical cross-section
            if (segments > 4) {
                // Create vertices for rounded shape
                for (int i = 0; i <= segments; ++i) {
                    float angle = (float)i / segments * 2.0f * M_PI;
                    float x_mult = cos(angle) * 0.8f; // Slightly flattened
                    float y_mult = sin(angle) * 0.9f;

                    // Front vertices
                    vertices.push_back(center.x + x_mult * half_dims.x);
                    vertices.push_back(center.y + y_mult * half_dims.y);
                    vertices.push_back(center.z + half_dims.z);

                    // Back vertices
                    vertices.push_back(center.x + x_mult * half_dims.x);
                    vertices.push_back(center.y + y_mult * half_dims.y);
                    vertices.push_back(center.z - half_dims.z);
                }

                // Create triangular faces
                for (int i = 0; i < segments; ++i) {
                    int front1 = currentVertexOffset + i * 2;
                    int back1 = front1 + 1;
                    int front2 = currentVertexOffset + ((i + 1) % (segments + 1)) * 2;
                    int back2 = front2 + 1;

                    // Side quad (2 triangles)
                    indices.push_back(front1);
                    indices.push_back(back1);
                    indices.push_back(front2);

                    indices.push_back(front2);
                    indices.push_back(back1);
                    indices.push_back(back2);
                }

            }
            else {
                // Standard cuboid for non-fuselage parts
                // Define local vertices for a cuboid centered at (0,0,0) with given half dimensions
                float local_cuboid_vertices[] = {
                    -half_dims.x, -half_dims.y, half_dims.z,  // 0: front-bottom-left
                     half_dims.x, -half_dims.y, half_dims.z,  // 1: front-bottom-right
                     half_dims.x,  half_dims.y, half_dims.z,  // 2: front-top-right
                    -half_dims.x,  half_dims.y, half_dims.z,  // 3: front-top-left
                    -half_dims.x, -half_dims.y, -half_dims.z, // 4: back-bottom-left
                    -half_dims.x,  half_dims.y, -half_dims.z, // 5: back-top-left
                     half_dims.x,  half_dims.y, -half_dims.z, // 6: back-top-right
                     half_dims.x, -half_dims.y, -half_dims.z  // 7: back-bottom-right
                };

                for (int i = 0; i < 8 * 3; i += 3) {
                    vertices.push_back(local_cuboid_vertices[i] + center.x);
                    vertices.push_back(local_cuboid_vertices[i + 1] + center.y);
                    vertices.push_back(local_cuboid_vertices[i + 2] + center.z);
                }

                // Indices for a standard cuboid (36 indices)
                unsigned int base_cuboid_indices[] = {
                    // Front face
                    0, 1, 2, 2, 3, 0,
                    // Back face
                    4, 5, 6, 6, 7, 4,
                    // Left face
                    4, 0, 3, 3, 5, 4,
                    // Right face
                    1, 7, 6, 6, 2, 1,
                    // Top face
                    3, 2, 6, 6, 5, 3,
                    // Bottom face
                    4, 7, 1, 1, 0, 4
                };

                for (int i = 0; i < 36; ++i) {
                    indices.push_back(base_cuboid_indices[i] + currentVertexOffset);
                }
            }
            };

        // Standard cuboid helper for wings and tail
        auto AddCuboid = [&](const Vector3f& center, const Vector3f& half_dims) {
            AddRoundedCuboid(center, half_dims, 4); // Use standard cuboid
            };

        // Fuselage - Rounded
        AddRoundedCuboid(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(hf_wid, hf_hgt, hf_len), 8);

        // Right Wing
        AddCuboid(Vector3f(hf_wid + w_span_each / 2.0f - 0.05f, 0.0f, wing_z_offset),
            Vector3f(w_span_each / 2.0f, hw_thick, hw_chord));

        // Left Wing
        AddCuboid(Vector3f(-(hf_wid + w_span_each / 2.0f - 0.05f), 0.0f, wing_z_offset),
            Vector3f(w_span_each / 2.0f, hw_thick, hw_chord));

        // Horizontal Tail
        AddCuboid(Vector3f(0.0f, ht_y_pos, ht_z_pos),
            Vector3f(ht_span_each, ht_thick / 2.0f, ht_chord / 2.0f));

        // Vertical Tail
        AddCuboid(Vector3f(0.0f, vt_y_pos + vt_height / 2.0f - 0.05f, vt_z_pos),
            Vector3f(vt_width / 2.0f, vt_height / 2.0f, vt_chord / 2.0f));

        // Dziób/Nose samolotu - ostry dziób z przodu
       
        AddCuboid(Vector3f(0.0f, 0.0f, nose_z_pos),
            Vector3f(nose_width / 2.0f, nose_height / 2.0f, nose_length / 2.0f));

        m_numTotalIndices = indices.size();

        // Generate and bind VAO, VBO, EBO
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &m_EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void CleanupBuffers()
    {
        if (m_VAO != 0) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
        if (m_VBO != 0) {
            glDeleteBuffers(1, &m_VBO);
            m_VBO = 0;
        }
        if (m_EBO != 0) {
            glDeleteBuffers(1, &m_EBO);
            m_EBO = 0;
        }
    }

    Vector3f m_position;
    float m_size;
    float m_rotation;
    float m_rotationX;
    float m_rotationZ;
    GLuint m_VAO, m_VBO, m_EBO;
    CubeTechnique m_cubeTech;
    unsigned int m_numTotalIndices;
};

class TerrainDemo12
{
public:

    TerrainDemo12()
    {
        m_lastFrameTime = glfwGetTime();
        m_cubeFollowsCamera = true;
    }

    virtual ~TerrainDemo12()
    {
        SAFE_DELETE(m_pGameCamera);
        SAFE_DELETE(m_pPlayerCube);
        if (m_birdVAO != 0) {
            glDeleteVertexArrays(1, &m_birdVAO);
        }
        if (m_birdVBO != 0) {
            glDeleteBuffers(1, &m_birdVBO);
        }
        if (m_birdEBO != 0) {
            glDeleteBuffers(1, &m_birdEBO);
        }
    }
    void InitBirds()
    {
        // Inicjalizuj shader dla ptaków
        m_birdTechnique.Init();

        // Stwórz podstawowy sześcian dla ptaków
        CreateBirdGeometry();

        // Pobierz początkową pozycję kamery
        Vector3f cameraStartPos = m_pGameCamera->GetPos();

        // Stwórz ptaki nad pozycją startową kamery
        int numBirds = 25;

        for (int i = 0; i < numBirds; i++) {
            // Pozycja w okolicy kamery startowej (w promieniu ~200 jednostek)
            Vector3f pos(
                cameraStartPos.x + (-100.0f + (float)(rand() % 200)), // ±100 od kamery w X
                cameraStartPos.y + 50.0f + (float)(rand() % 200),     // 50-250 nad kamerą w Y
                cameraStartPos.z + (-100.0f + (float)(rand() % 200))  // ±100 od kamery w Z
            );

            // Upewnij się, że ptaki są wysoko nad terenem
            if (pos.y < 200.0f) {
                pos.y = 200.0f + (float)(rand() % 200); // 200-400 wysokości
            }

            // Losowa prędkość - ale bardziej umiarkowana
            Vector3f vel(
                -15.0f + (float)(rand() % 30), // -15 do +15
                -2.0f + (float)(rand() % 4),   // lekkie ruchy w górę/dół
                -15.0f + (float)(rand() % 30)  // -15 do +15
            );

            // Ograniczenia do granic terenu
            float worldSize = m_terrain.GetWorldSize();
            if (pos.x < 0.0f) pos.x = 50.0f;
            if (pos.x > worldSize) pos.x = worldSize - 50.0f;
            if (pos.z < 0.0f) pos.z = 50.0f;
            if (pos.z > worldSize) pos.z = worldSize - 50.0f;

            m_birds.push_back(Bird(pos, vel));
        }

        printf("Spawned %d birds around camera start position (%.1f, %.1f, %.1f)\n",
            numBirds, cameraStartPos.x, cameraStartPos.y, cameraStartPos.z);
    }

    void Init()
    {
        CreateWindow_();
        InitCallbacks();
        InitTerrain();
        InitCamera();
        InitPlayerCube();
		InitBirds();
        InitGUI();
    }

    

    void CreateBirdGeometry()
    {
        // Proste wierzchołki sześcianu
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f, -0.5f,  0.5f
        };

        unsigned int indices[] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            4, 0, 3, 3, 5, 4,
            1, 7, 6, 6, 2, 1,
            3, 2, 6, 6, 5, 3,
            4, 7, 1, 1, 0, 4
        };

        glGenVertexArrays(1, &m_birdVAO);
        glBindVertexArray(m_birdVAO);

        glGenBuffers(1, &m_birdVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_birdVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenBuffers(1, &m_birdEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_birdEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void UpdateBirds()
    {
        float worldSize = m_terrain.GetWorldSize();
        for (auto& bird : m_birds) {
            bird.Update(m_deltaTime, worldSize);
        }
    }

    void RenderBirds()
    {
        Matrix4f VP = m_pGameCamera->GetViewProjMatrix();

        m_birdTechnique.Enable();
        m_birdTechnique.SetVP(VP);

        glBindVertexArray(m_birdVAO);

        for (auto& bird : m_birds) {
            bird.Render(VP, m_birdTechnique);
        }

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void UpdateCubePosition()
    {
        if (!m_pPlayerCube) return;

        Vector3f cameraPos = m_pGameCamera->GetPos();
        Vector3f cubePos;

        switch (m_cubeFollowMode) {
        case FOLLOW_CAMERA_XYZ:
            // Kostka podąża za kamerą w pełni 3D - kopiuje dokładnie pozycję kamery
            cubePos = cameraPos;
            break;

        case FOLLOW_CAMERA_XZ:
            // Kostka podąża za kamerą w XZ, ale stoi na terenie
        {
            float terrainHeight = m_terrain.GetWorldHeight(cameraPos.x, cameraPos.z);
            cubePos = Vector3f(cameraPos.x, terrainHeight + m_pPlayerCube->GetSize() / 2.0f, cameraPos.z);
        }
        break;

        case FOLLOW_TERRAIN_HEIGHT:
            // Kostka podąża za kamerą w XZ, utrzymuje stałą wysokość nad terenem
        {
            float terrainHeight = m_terrain.GetWorldHeight(cameraPos.x, cameraPos.z);
            cubePos = Vector3f(cameraPos.x, terrainHeight + m_cubeHeightOffset + m_pPlayerCube->GetSize() / 2.0f, cameraPos.z);
        }
        break;
        }

        // Ograniczenia do granic terenu (tylko X i Z)
        float worldSize = m_terrain.GetWorldSize();
        float cubeSize = m_pPlayerCube->GetSize();

        if (cubePos.x < cubeSize / 2.0f) cubePos.x = cubeSize / 2.0f;
        if (cubePos.x > worldSize - cubeSize / 2.0f) cubePos.x = worldSize - cubeSize / 2.0f;
        if (cubePos.z < cubeSize / 2.0f) cubePos.z = cubeSize / 2.0f;
        if (cubePos.z > worldSize - cubeSize / 2.0f) cubePos.z = worldSize - cubeSize / 2.0f;

        // Dla trybu FOLLOW_CAMERA_XYZ, również ograniczamy Y do rozsądnych wartości
        if (m_cubeFollowMode == FOLLOW_CAMERA_XYZ) {
            // Nie pozwól kostce zejść poniżej terenu
            float terrainHeight = m_terrain.GetWorldHeight(cubePos.x, cubePos.z);
            if (cubePos.y < terrainHeight + cubeSize / 2.0f) {
                cubePos.y = terrainHeight + cubeSize / 2.0f;
            }

            // Nie pozwól kostce wejść zbyt wysoko
            if (cubePos.y > m_maxHeight + 200.0f) {
                cubePos.y = m_maxHeight + 200.0f;
            }
        }

        m_pPlayerCube->SetPosition(cubePos);
    }

    void SetCameraBehindCubeFixed()
    {
        if (!m_pPlayerCube || !m_pGameCamera) return;

        // Stałe wartości dla lepszej widoczności kostki
        m_cameraDistance = 150.0f;       // Odległość od kostki
        m_cameraHeightOffset = 50.0f;    // Wysokość nad kostką

        // Kamera powinna być za kostką względem jej orientacji
        UpdateCameraPosition();

        m_cameraFixed = true;
        m_cubeControlMode = true;

        printf("Camera positioned behind cube and FIXED with cube rotation control.\n");
        printf("Use WASD to move cube: W=Forward, S=Backward, A=Left, D=Right\n");
        printf("Use LEFT/RIGHT arrows to rotate CUBE (camera will follow)\n");
        printf("Use +/- for up/down, T again to unlock camera\n");
    }

    void UpdateCameraPosition()
    {
        if (!m_pPlayerCube) return;

        Vector3f cubePos = m_pPlayerCube->GetPosition();

        // Pobierz wszystkie obroty kostki
        float cubeRotationY = m_pPlayerCube->GetRotation();
        float cubeRotationX = m_pPlayerCube->GetRotationX();
        float cubeRotationZ = m_pPlayerCube->GetRotationZ();

        // Początkowy offset kamery (za kostką)
        Vector3f baseOffset(0.0f, m_cameraHeightOffset, m_cameraDistance);

        // Zastosuj wszystkie obroty do offsetu kamery w tej samej kolejności co w Render()

        // 1. Obrót wokół osi Y
        float radiansY = -cubeRotationY * M_PI / 180.0f;
        Vector3f offsetAfterY;
        offsetAfterY.x = baseOffset.x * cos(radiansY) + baseOffset.z * sin(radiansY);
        offsetAfterY.y = baseOffset.y;
        offsetAfterY.z = -baseOffset.x * sin(radiansY) + baseOffset.z * cos(radiansY);

        // 2. Obrót wokół osi X
        float radiansX = cubeRotationX * M_PI / 180.0f;
        Vector3f offsetAfterX;
        offsetAfterX.x = offsetAfterY.x;
        offsetAfterX.y = offsetAfterY.y * cos(radiansX) - offsetAfterY.z * sin(radiansX);
        offsetAfterX.z = offsetAfterY.y * sin(radiansX) + offsetAfterY.z * cos(radiansX);

        // 3. Obrót wokół osi Z
        float radiansZ = cubeRotationZ * M_PI / 180.0f;
        Vector3f finalOffset;
        finalOffset.x = offsetAfterX.x * cos(radiansZ) - offsetAfterX.y * sin(radiansZ);
        finalOffset.y = offsetAfterX.x * sin(radiansZ) + offsetAfterX.y * cos(radiansZ);
        finalOffset.z = offsetAfterX.z;

        // Pozycja kamery = pozycja kostki + obrócony offset
        m_fixedCameraPos = cubePos + finalOffset;

        // Sprawdź czy kamera nie wejdzie pod teren
        float terrainHeight = m_terrain.GetWorldHeight(m_fixedCameraPos.x, m_fixedCameraPos.z);
        if (m_fixedCameraPos.y < terrainHeight + 15.0f) {
            m_fixedCameraPos.y = terrainHeight + 15.0f;
        }

        // Ograniczenia do granic terenu
        float worldSize = m_terrain.GetWorldSize();
        if (m_fixedCameraPos.x < 5.0f) m_fixedCameraPos.x = 5.0f;
        if (m_fixedCameraPos.x > worldSize - 5.0f) m_fixedCameraPos.x = worldSize - 5.0f;
        if (m_fixedCameraPos.z < 5.0f) m_fixedCameraPos.z = 5.0f;
        if (m_fixedCameraPos.z > worldSize - 5.0f) m_fixedCameraPos.z = worldSize - 5.0f;

        // Kierunek patrzenia - zawsze na kostkę
        m_fixedCameraTarget = (cubePos - m_fixedCameraPos).Normalize();

        // Aktualizuj kamerę
        m_pGameCamera->SetPosition(m_fixedCameraPos);
        m_pGameCamera->SetTarget(m_fixedCameraTarget);
    }

    void RotateCube(float deltaAngle)
    {
        if (!m_cameraFixed || !m_pPlayerCube) return;

        m_pPlayerCube->Rotate(deltaAngle);

        // Automatycznie aktualizuj pozycję kamery
        UpdateCameraPosition();

        printf("Cube rotation: %.1f degrees\n", m_pPlayerCube->GetRotation());
    }

    // 3. DODAJ nową metodę MoveCube():
    void MoveCube(Vector3f direction)
    {
        if (!m_pPlayerCube || !m_cubeControlMode) return;

        Vector3f currentPos = m_pPlayerCube->GetPosition();

        // Transformuj kierunek ruchu względem orientacji KOSTKI (nie kamery)
        float cubeRotation = m_pPlayerCube->GetRotation();
        float radians = cubeRotation * M_PI / 180.0f;

        // Obrót kierunku ruchu o kąt kostki
        Vector3f transformedDirection;
        transformedDirection.x = direction.x * cos(radians) - direction.z * sin(radians);
        transformedDirection.z = direction.x * sin(radians) + direction.z * cos(radians);
        transformedDirection.y = direction.y; // Y pozostaje bez zmian

        Vector3f newPos = currentPos + (transformedDirection * m_cubeSpeed * m_deltaTime);

        // Ograniczenia do granic terenu
        float worldSize = m_terrain.GetWorldSize();
        float cubeSize = m_pPlayerCube->GetSize();

        if (newPos.x < cubeSize / 2.0f) newPos.x = cubeSize / 2.0f;
        if (newPos.x > worldSize - cubeSize / 2.0f) newPos.x = worldSize - cubeSize / 2.0f;
        if (newPos.z < cubeSize / 2.0f) newPos.z = cubeSize / 2.0f;
        if (newPos.z > worldSize - cubeSize / 2.0f) newPos.z = worldSize - cubeSize / 2.0f;

        // Sprawdź czy kostka nie wejdzie pod teren
        float terrainHeight = m_terrain.GetWorldHeight(newPos.x, newPos.z);
        if (newPos.y < terrainHeight + cubeSize / 2.0f) {
            newPos.y = terrainHeight + cubeSize / 2.0f;
        }

        // Nie pozwól kostce wejść zbyt wysoko
        if (newPos.y > m_maxHeight + 300.0f) {
            newPos.y = m_maxHeight + 300.0f;
        }

        m_pPlayerCube->SetPosition(newPos);

        // Aktualizuj pozycję kamery aby podążała za kostką
        if (m_cameraFixed) {
            UpdateCameraPosition();
        }
    }
    //nidfnasidifnsif
        //dsdaadadasasdasdasdasdsd

    void Run()
    {
        while (!glfwWindowShouldClose(window)) {
            double currentTime = glfwGetTime();
            m_deltaTime = currentTime - m_lastFrameTime;
            m_lastFrameTime = currentTime;

            glfwPollEvents();

            if (m_cubeControlMode) {
                Vector3f moveDirection(0.0f, 0.0f, 0.0f);

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    moveDirection.z -= 1.0f; // Forward
                }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    moveDirection.z += 1.0f; // Backward
                }
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    moveDirection.x -= 1.0f; // Left
                }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    moveDirection.x += 1.0f; // Right
                }
                if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) { // '+'
                    moveDirection.y += 1.0f; // Up
                }
                if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
                    moveDirection.y -= 1.0f; // Down
                }
                if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
                    m_pPlayerCube->RotateZ(m_cameraRotationSpeed * m_deltaTime);
                    if (m_cameraFixed) UpdateCameraPosition();
                }
                if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
                    m_pPlayerCube->RotateZ(-m_cameraRotationSpeed * m_deltaTime);
                    if (m_cameraFixed) UpdateCameraPosition();
                }
                if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
                    m_pPlayerCube->Rotate(m_cameraRotationSpeed * m_deltaTime);
                    if (m_cameraFixed) UpdateCameraPosition();
                }
                if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
                    m_pPlayerCube->Rotate(-m_cameraRotationSpeed * m_deltaTime);
                    if (m_cameraFixed) UpdateCameraPosition();
                }
                if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
                    m_pPlayerCube->RotateX(-m_cameraRotationSpeed * m_deltaTime);
                    if (m_cameraFixed) UpdateCameraPosition();
                }
                if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
                    m_pPlayerCube->RotateX(m_cameraRotationSpeed * m_deltaTime);
                    if (m_cameraFixed) UpdateCameraPosition();
                }

                // Normalize diagonal movement
                if (moveDirection.Length() > 0.0f) {
                    moveDirection.Normalize();
                    MoveCube(moveDirection);
                }
            }

            // Update cube position to follow camera (if enabled)
            if (m_cubeFollowsCamera) {
                UpdateCubePosition();
            }
            UpdateBirds();

            if (m_showGui) {
                // Start the Dear ImGui frame
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::Begin("Terrain Demo 12 - Cube Follows Camera");

                ImGui::SliderFloat("Max height", &this->m_maxHeight, 0.0f, 1000.0f);
                ImGui::SliderFloat("Terrain roughness", &this->m_roughness, 0.0f, 5.0f);

                static float Height0 = 64.0f;
                static float Height1 = 128.0f;
                static float Height2 = 192.0f;
                static float Height3 = 256.0f;

                ImGui::SliderFloat("Height0", &Height0, 0.0f, 64.0f);
                ImGui::SliderFloat("Height1", &Height1, 64.0f, 128.0f);
                ImGui::SliderFloat("Height2", &Height2, 128.0f, 192.0f);
                ImGui::SliderFloat("Height3", &Height3, 192.0f, 256.0f);

                if (ImGui::Button("Generate")) {
                    m_terrain.Destroy();
                    srand(g_seed);
                    m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize, m_roughness, m_minHeight, m_maxHeight);
                    m_terrain.SetTextureHeights(Height0, Height1, Height2, Height3);
                }

                Vector3f cubePos = m_pPlayerCube->GetPosition();
                Vector3f cameraPos = m_pGameCamera->GetPos();
                ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", cameraPos.x, cameraPos.y, cameraPos.z);
                ImGui::Text("Cube Position: (%.1f, %.1f, %.1f)", cubePos.x, cubePos.y, cubePos.z);

                ImGui::Separator();
                ImGui::Checkbox("Cube Follows Camera", &m_cubeFollowsCamera);
                if (ImGui::Button("Update Cube Position Now")) {
                    UpdateCubePosition();
                }

                ImGui::Separator();
                ImGui::Text("Controls:");
                ImGui::Text("WASD - Move Camera");
                ImGui::Text("Mouse - Look around");
                ImGui::Text("F - Toggle cube following camera");
                ImGui::Text("U - Update cube position manually");
                ImGui::Text("C - Print Camera, R - Reset Camera Position");

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();

                // Rendering
                
                ImGui::Render();
                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            
            RenderScene();
            glfwSwapBuffers(window);
        }
    }

    void RenderScene()
    {
        if (!m_showGui) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        m_pGameCamera->OnRender();
        Matrix4f VP = m_pGameCamera->GetViewProjMatrix();

        // Render terrain first
        m_terrain.Render(*m_pGameCamera);

        // Render the cube
        if (m_pPlayerCube) {
            m_pPlayerCube->Render(VP);
        }

        RenderBirds();
    }

    void PassiveMouseCB(int x, int y)
    {
        if (!m_showGui && !m_isPaused) {
            m_pGameCamera->OnMouse(x, y);
        }
    }

    void KeyboardCB(uint key, int state)
    {
        if (state == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwDestroyWindow(window);
                glfwTerminate();
                exit(0);

            case GLFW_KEY_R:
                // Reset camera position
            {
                float CameraX = m_terrain.GetWorldSize() / 2.0f;
                float CameraZ = CameraX + 50.0f;
                Vector3f resetPos(CameraX, m_maxHeight + 20.0f, CameraZ);
                m_pGameCamera->SetPosition(resetPos);
                printf("Camera position reset\n");
            }
            break;

            case GLFW_KEY_F:
                m_cubeFollowsCamera = !m_cubeFollowsCamera;
                printf("Cube follows camera: %s\n", m_cubeFollowsCamera ? "ON" : "OFF");
                break;

            case GLFW_KEY_U:
                UpdateCubePosition();
                printf("Cube position updated manually\n");
                break;

            case GLFW_KEY_C:
                m_pGameCamera->Print();
                break;

            case GLFW_KEY_E:
                m_isWireframe = !m_isWireframe;
                if (m_isWireframe) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }
                else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
                break;

            case GLFW_KEY_T:
                if (m_cameraFixed) {
                    // Odblokuj kamerę
                    m_cameraFixed = false;
                    m_cubeControlMode = false;
                    printf("Camera unlocked. Normal camera control restored.\n");
                }
                else {
                    // Zablokuj kamerę za kostką
                    SetCameraBehindCubeFixed();
                }
                break;
           

            case GLFW_KEY_P:
                m_isPaused = !m_isPaused;
                break;

            case GLFW_KEY_SPACE:
                m_showGui = !m_showGui;
                break;

            case GLFW_KEY_0:
                gShowPoints = 0;
                break;

            case GLFW_KEY_1:
                m_cubeFollowMode = FOLLOW_CAMERA_XYZ;
               break;

            case GLFW_KEY_2:
                m_cubeFollowMode = FOLLOW_CAMERA_XZ;
                break;

            case GLFW_KEY_3:
                m_cubeFollowMode = FOLLOW_TERRAIN_HEIGHT;
                break;

            case GLFW_KEY_MINUS:
                if (!m_cubeControlMode && m_cubeFollowMode == FOLLOW_TERRAIN_HEIGHT) {
                    m_cubeHeightOffset -= 10.0f;
                    if (m_cubeHeightOffset < 0.0f) m_cubeHeightOffset = 0.0f;
                    printf("Cube height offset: %.1f\n", m_cubeHeightOffset);
                }
                // W trybie kontroli kostki obsługa jest w pętli głównej
                break;

            case GLFW_KEY_EQUAL: // klawisz '+'
                if (!m_cubeControlMode && m_cubeFollowMode == FOLLOW_TERRAIN_HEIGHT) {
                    m_cubeHeightOffset += 10.0f;
                    printf("Cube height offset: %.1f\n", m_cubeHeightOffset);
                }
                // W trybie kontroli kostki obsługa jest w pętli głównej
                break;
            }
        }

        // Handle normal camera movement
        if (!m_cubeControlMode) {
            bool CameraChangedPos = m_pGameCamera->OnKeyboard(key);

            if (m_constrainCamera && CameraChangedPos) {
                ConstrainCameraToTerrain();
            }
        }
        else {
            // W trybie kontroli kostki, blokuj przekazywanie WASD do kamery
            if (key != GLFW_KEY_W && key != GLFW_KEY_A && key != GLFW_KEY_S && key != GLFW_KEY_D) {
                // Pozwól na inne klawisze kamery (np. strzałki)
                bool CameraChangedPos = m_pGameCamera->OnKeyboard(key);

                if (m_constrainCamera && CameraChangedPos) {
                    ConstrainCameraToTerrain();
                }
            }
        }
    }

    void MouseCB(int button, int action, int x, int y)
    {
    }

private:

    enum CubeFollowMode {
        FOLLOW_CAMERA_XYZ,    // Kostka podąża za kamerą w 3D (x,y,z)
        FOLLOW_CAMERA_XZ,     // Kostka podąża za kamerą tylko w płaszczyźnie XZ, stoi na terenie
        FOLLOW_TERRAIN_HEIGHT // Kostka podąża za kamerą XZ, ale utrzymuje stałą wysokość nad terenem
    };

    void CreateWindow_()
    {
        int major_ver = 0;
        int minor_ver = 0;
        bool is_full_screen = false;
        window = glfw_init(major_ver, minor_ver, WINDOW_WIDTH, WINDOW_HEIGHT, is_full_screen, "Terrain Rendering - Demo 12 with Cube Following Camera");
        glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    }

    void InitCallbacks()
    {
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
    }

    void InitCamera()
    {
        float CameraX = m_terrain.GetWorldSize() / 2.0f;
        float CameraZ = CameraX + 50.0f; // Position camera back from center
        Vector3f Pos(CameraX, m_maxHeight + 20.0f, CameraZ); // Start above terrain
        Vector3f Target(0.0f, 0.0f, -1.0f); // Look forward towards terrain center
        Vector3f Up(0.0, 1.0f, 0.0f);

        float FOV = 60.0f;
        float zNear = 0.01f;
        float zFar = Z_FAR;
        PersProjInfo persProjInfo = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };

        m_pGameCamera = new BasicCamera(persProjInfo, Pos, Target, Up);
        m_pGameCamera->SetSpeed(2.5f);
    }

    void InitPlayerCube()
    {
        m_pPlayerCube = new PlayerCube();
        float cubeSize = 25.0f;
        m_pPlayerCube->SetSize(cubeSize);

        // Initial position will be set by UpdateCubePosition()
        Vector3f cubePos(0.0f, 0.0f, 0.0f);
        m_pPlayerCube->SetPosition(cubePos);
    }

    void InitTerrain()
    {
        float WorldScale = 20.0f;
        float TextureScale = 16.0f;
        std::vector<string> TextureFilenames;
        TextureFilenames.push_back("rock051.jpg");
        TextureFilenames.push_back("coast_sand_rocks.jpg");
        TextureFilenames.push_back("grass-verydark.png");
        TextureFilenames.push_back("water.png");

        m_terrain.InitTerrain(WorldScale, TextureScale, TextureFilenames);
        m_terrain.CreateMidpointDisplacement(m_terrainSize, m_patchSize, m_roughness, m_minHeight, m_maxHeight);

        Vector3f LightDir(0.0f, -1.0f, 0.0f);
        m_terrain.SetLightDir(LightDir);
    }

    void InitGUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        const char* glsl_version = "#version 130";
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    void ConstrainCameraToTerrain()
    {
        Vector3f NewCameraPos = m_terrain.ConstrainCameraPosToTerrain(m_pGameCamera->GetPos());
        m_pGameCamera->SetPosition(NewCameraPos);
    }

    GLFWwindow* window = NULL;
    BasicCamera* m_pGameCamera = NULL;
    PlayerCube* m_pPlayerCube = NULL;
    bool m_isWireframe = false;
    MidpointDispTerrain m_terrain;
    bool m_showGui = false;
    bool m_isPaused = false;
    int m_terrainSize = 513;
    float m_roughness = 0.4f;
    float m_minHeight = 30.0f;
    float m_maxHeight = 400.0f;
    int m_patchSize = 17;
    bool m_constrainCamera = false;
    double m_lastFrameTime;
    double m_deltaTime;
    bool m_cubeFollowsCamera;
    CubeFollowMode m_cubeFollowMode = FOLLOW_CAMERA_XYZ;
    float m_cubeHeightOffset = 0.0f;
    bool m_cubeControlMode = false;        // Tryb sterowania kostką
    bool m_cameraFixed = false;           // Czy kamera jest zablokowana
    Vector3f m_fixedCameraPos;            // Pozycja zablokowanej kamery
    Vector3f m_fixedCameraTarget;         // Cel zablokowanej kamery
    float m_cubeSpeed = 150.0f;            // Prędkość ruchu kostki
    float m_cameraDistance = 150.0f;     // Odległość kamery od kostki
    float m_cameraHeightOffset = 50.0f;
    float m_cameraRotationSpeed = 90.0f; // Prędkość obrotu kamery (stopni/sekundę)
    std::vector<Bird> m_birds;
    CubeTechnique m_birdTechnique;
    GLuint m_birdVAO, m_birdVBO, m_birdEBO;
};

TerrainDemo12* app = NULL;

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    app->KeyboardCB(key, action);
}



static void CursorPosCallback(GLFWwindow* window, double x, double y)
{
    app->PassiveMouseCB((int)x, (int)y);
}

static void MouseButtonCallback(GLFWwindow* window, int Button, int Action, int Mode)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    app->MouseCB(Button, Action, (int)x, (int)y);
}

int main(int argc, char** argv)
{
#ifdef _WIN64
    g_seed = GetCurrentProcessId();
#else
    g_seed = getpid();
#endif
    printf("random seed %d\n", g_seed);

    srand(g_seed);

    app = new TerrainDemo12();
    app->Init();

    glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 0.0f);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    app->Run();

    delete app;
    return 0;
}