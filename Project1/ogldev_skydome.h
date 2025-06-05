#ifndef OGLDEV_SKYDOME_H
#define OGLDEV_SKYDOME_H

#include <glew.h>

#include "ogldev_skydome_technique.h"
#include "ogldev_basic_glfw_camera.h"
#include "ogldev_texture.h"

class Skydome
{
public:
    Skydome(int NumPitchStripes, int NumHeadingStripes, float Radius, const char* pTextureFilanem, GLenum TextureUnit, int TextureUnitIndex);

    void Render(const BasicCamera& Camera);

private:

    struct Vertex {
        Vector3f Pos;
        Vector2f Tex;

        Vertex() {}

        Vertex(const Vector3f& p);
    };

    void CreateGLState();

    void PopulateBuffers(int NumRows, int NumCols, float Radius);

    void LoadTexture(const char* pTextureFilanem);

    int m_numVertices = 0;
    GLuint m_vao;
    GLuint m_vb;
    Texture m_texture;
    SkydomeTechnique m_skydomeTech;
    int m_textureUnit = 0;
    GLenum m_textureUnitIndex = 0;
};


#endif