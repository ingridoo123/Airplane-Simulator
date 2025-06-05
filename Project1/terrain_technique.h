#ifndef TERRAIN_TECHNIQUE_H
#define TERRAIN_TECHNIQUE_H

#include "technique.h"
#include "ogldev_math_3d.h"

class TerrainTechnique : public Technique
{
public:

    TerrainTechnique();

    virtual bool Init();

    void SetVP(const Matrix4f& VP);

    void SetMinMaxHeight(float Min, float Max);

    void SetTextureHeights(float Tex0Height, float Tex1Height, float Tex2Height, float Tex3Height);

    void SetLightDir(const Vector3f& Dir);

    void SetSecondLightDir(const Vector3f& Dir);

    // New methods for light intensities
    void SetMainLightIntensity(float Intensity);
    void SetSecondLightIntensity(float Intensity);

private:
    GLuint m_VPLoc = -1;
    GLuint m_minHeightLoc = -1;
    GLuint m_maxHeightLoc = -1;
    GLuint m_tex0HeightLoc = -1;
    GLuint m_tex1HeightLoc = -1;
    GLuint m_tex2HeightLoc = -1;
    GLuint m_tex3HeightLoc = -1;
    GLuint m_tex0UnitLoc = -1;
    GLuint m_tex1UnitLoc = -1;
    GLuint m_tex2UnitLoc = -1;
    GLuint m_tex3UnitLoc = -1;
    GLuint m_reversedLightDirLoc = -1;
    GLuint m_secondLightDirLoc = -1;
    GLuint m_mainLightIntensityLoc = -1;
    GLuint m_secondLightIntensityLoc = -1;
};

#endif  /* TERRAIN_TECHNIQUE_H */