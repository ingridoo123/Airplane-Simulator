#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <string.h>

#include "terrain.h"
#include "texture_config.h"
#include "stb_image_write.h"

//#define DEBUG_PRINT

BaseTerrain::~BaseTerrain()
{
    Destroy();
}


void BaseTerrain::Destroy()
{
    m_heightMap.Destroy();
    m_geomipGrid.Destroy();
}



void BaseTerrain::InitTerrain(float WorldScale, float TextureScale, const std::vector<string>& TextureFilenames)
{
    if (!m_terrainTech.Init()) {
        printf("Error initializing tech\n");
        exit(0);
    }

    if (TextureFilenames.size() != ARRAY_SIZE_IN_ELEMENTS(m_pTextures)) {
        printf("%s:%d - number of provided textures (%lud) is not equal to the size of the texture array (%lud)\n",
            __FILE__, __LINE__, TextureFilenames.size(), ARRAY_SIZE_IN_ELEMENTS(m_pTextures));
        exit(0);
    }

    m_worldScale = WorldScale;
    m_textureScale = TextureScale;

    for (int i = 0; i < ARRAY_SIZE_IN_ELEMENTS(m_pTextures); i++) {
        m_pTextures[i] = new Texture(GL_TEXTURE_2D);
        m_pTextures[i]->Load(TextureFilenames[i]);
    }

    // Enable shader program before setting uniforms
    m_terrainTech.Enable();

    // Set up main light (sun)
    Vector3f LightDir(0.0f, -1.0f, 0.0f);
    m_terrainTech.SetLightDir(LightDir);

    // Set up second light (red sun)
    Vector3f SecondLightDir(0.5f, -0.5f, 0.5f);  // Coming from a different angle
    m_terrainTech.SetSecondLightDir(SecondLightDir);

    // Disable shader program after setting uniforms
    glUseProgram(0);

    m_pSkydome = new Skydome(8, 32, 1.0f, "kloofendal_48d_partly_cloudy_puresky_4k.jpg", COLOR_TEXTURE_UNIT_0, COLOR_TEXTURE_UNIT_INDEX_0);
}


void BaseTerrain::Finalize()
{
    m_geomipGrid.CreateGeomipGrid(m_terrainSize, m_terrainSize, m_patchSize, this);
}


float BaseTerrain::GetHeightInterpolated(float x, float z) const
{
    float X0Z0Height = GetHeight((int)x, (int)z);

    if (((int)x + 1 >= m_terrainSize) || ((int)z + 1 >= m_terrainSize)) {
        return X0Z0Height;
    }

    float X1Z0Height = GetHeight((int)x + 1, (int)z);
    float X0Z1Height = GetHeight((int)x, (int)z + 1);
    float X1Z1Height = GetHeight((int)x + 1, (int)z + 1);

    float FactorX = x - floorf(x);

    float InterpolatedBottom = (X1Z0Height - X0Z0Height) * FactorX + X0Z0Height;
    float InterpolatedTop = (X1Z1Height - X0Z1Height) * FactorX + X0Z1Height;

    float FactorZ = z - floorf(z);

    float FinalHeight = (InterpolatedTop - InterpolatedBottom) * FactorZ + InterpolatedBottom;

    return FinalHeight;
}


void BaseTerrain::LoadFromFile(const char* pFilename)
{
    LoadHeightMapFile(pFilename);

    // how do we know the patch size at this point?
    assert(0);

    m_geomipGrid.CreateGeomipGrid(m_terrainSize, m_terrainSize, m_patchSize, this);
}


void BaseTerrain::LoadHeightMapFile(const char* pFilename)
{
    int FileSize = 0;
    unsigned char* p = (unsigned char*)ReadBinaryFile(pFilename, FileSize);

    if (FileSize % sizeof(float) != 0) {
        printf("%s:%d - '%s' does not contain an whole number of floats (size %d)\n", __FILE__, __LINE__, pFilename, FileSize);
        exit(0);
    }

    m_terrainSize = (int)sqrtf((float)FileSize / (float)sizeof(float));

    printf("Terrain size %d\n", m_terrainSize);

    if ((m_terrainSize * m_terrainSize) != (FileSize / sizeof(float))) {
        printf("%s:%d - '%s' does not contain a square height map - size %d\n", __FILE__, __LINE__, pFilename, FileSize);
        exit(0);
    }

    m_heightMap.InitArray2D(m_terrainSize, m_terrainSize, (float*)p);
}


void BaseTerrain::SaveToFile(const char* pFilename)
{
    unsigned char* p = (unsigned char*)malloc(m_terrainSize * m_terrainSize);

    float* src = m_heightMap.GetBaseAddr();

    float Delta = m_maxHeight - m_minHeight;

    for (int i = 0; i < m_terrainSize * m_terrainSize; i++) {
        float f = (src[i] - m_minHeight) / Delta;
        p[i] = (unsigned char)(f * 255.0f);
    }

    stbi_write_png("heightmap.png", m_terrainSize, m_terrainSize, 1, p, m_terrainSize);

    free(p);
}


void BaseTerrain::Render(const BasicCamera& Camera)
{
    Matrix4f VP = Camera.GetViewProjMatrix();
    Matrix4f View = Camera.GetMatrix();

    m_terrainTech.Enable();
    m_terrainTech.SetVP(VP);

    for (int i = 0; i < ARRAY_SIZE_IN_ELEMENTS(m_pTextures); i++) {
        if (m_pTextures[i]) {
            m_pTextures[i]->Bind(COLOR_TEXTURE_UNIT_0 + i);
        }
    }

    m_terrainTech.SetLightDir(m_lightDir);

    m_geomipGrid.Render(Camera.GetPos(), VP);

    m_pSkydome->Render(Camera);
}


void BaseTerrain::SetMinMaxHeight(float MinHeight, float MaxHeight)
{
    m_minHeight = MinHeight;
    m_maxHeight = MaxHeight;

    m_terrainTech.Enable();
    m_terrainTech.SetMinMaxHeight(MinHeight, MaxHeight);
}


void BaseTerrain::SetTextureHeights(float Tex0Height, float Tex1Height, float Tex2Height, float Tex3Height)
{
    m_terrainTech.SetTextureHeights(Tex0Height, Tex1Height, Tex2Height, Tex3Height);
}


float BaseTerrain::GetWorldHeight(float x, float z) const
{
    float HeightMapX = x / m_worldScale;
    float HeightMapZ = z / m_worldScale;

    return GetHeightInterpolated(HeightMapX, HeightMapZ);
}


Vector3f BaseTerrain::ConstrainCameraPosToTerrain(const Vector3f& CameraPos)
{
    Vector3f NewCameraPos = CameraPos;

    // Make sure camera doesn't go outside of the terrain bounds
    if (CameraPos.x < 0.0f) {
        NewCameraPos.x = 0.0f;
    }

    if (CameraPos.z < 0.0f) {
        NewCameraPos.z = 0.0f;
    }

    if (CameraPos.x >= GetWorldSize()) {
        NewCameraPos.x = GetWorldSize() - 0.5f;
    }

    if (CameraPos.z >= GetWorldSize()) {
        NewCameraPos.z = GetWorldSize() - 0.5f;
    }

    NewCameraPos.y = GetWorldHeight(CameraPos.x, CameraPos.z) + m_cameraHeight;

    float f = sinf(CameraPos.x * 4.0f) + cosf(CameraPos.z * 4.0f);
    f /= 35.0f;

    NewCameraPos.y += f;

    return NewCameraPos;
}