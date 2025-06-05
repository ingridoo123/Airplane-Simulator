#include "ogldev_util.h"
#include "terrain_technique.h"
#include "texture_config.h"


TerrainTechnique::TerrainTechnique()
{
}

bool TerrainTechnique::Init()
{
    if (!Technique::Init()) {
        return false;
    }

    if (!AddShader(GL_VERTEX_SHADER, "terrain.vs")) {
        return false;
    }

    if (!AddShader(GL_FRAGMENT_SHADER, "terrain.fs")) {
        return false;
    }

    if (!Finalize()) {
        return false;
    }

    m_VPLoc = GetUniformLocation("gVP");
    m_minHeightLoc = GetUniformLocation("gMinHeight");
    m_maxHeightLoc = GetUniformLocation("gMaxHeight");
    m_tex0HeightLoc = GetUniformLocation("gHeight0");
    m_tex1HeightLoc = GetUniformLocation("gHeight1");
    m_tex2HeightLoc = GetUniformLocation("gHeight2");
    m_tex3HeightLoc = GetUniformLocation("gHeight3");
    m_reversedLightDirLoc = GetUniformLocation("gReversedLightDir");
    m_secondLightDirLoc = GetUniformLocation("gSecondLightDir");
    m_tex0UnitLoc = GetUniformLocation("gTextureHeight0");
    m_tex1UnitLoc = GetUniformLocation("gTextureHeight1");
    m_tex2UnitLoc = GetUniformLocation("gTextureHeight2");
    m_tex3UnitLoc = GetUniformLocation("gTextureHeight3");
    m_mainLightIntensityLoc = GetUniformLocation("gMainLightIntensity");
    m_secondLightIntensityLoc = GetUniformLocation("gSecondLightIntensity");

    if (m_VPLoc == INVALID_UNIFORM_LOCATION ||
        m_minHeightLoc == INVALID_UNIFORM_LOCATION ||
        m_maxHeightLoc == INVALID_UNIFORM_LOCATION ||
        m_tex0HeightLoc == INVALID_UNIFORM_LOCATION ||
        m_tex1HeightLoc == INVALID_UNIFORM_LOCATION ||
        m_tex2HeightLoc == INVALID_UNIFORM_LOCATION ||
        m_tex3HeightLoc == INVALID_UNIFORM_LOCATION ||
        m_reversedLightDirLoc == INVALID_UNIFORM_LOCATION ||
        m_secondLightDirLoc == INVALID_UNIFORM_LOCATION ||
        m_tex0UnitLoc == INVALID_UNIFORM_LOCATION ||
        m_tex1UnitLoc == INVALID_UNIFORM_LOCATION ||
        m_tex2UnitLoc == INVALID_UNIFORM_LOCATION ||
        m_tex3UnitLoc == INVALID_UNIFORM_LOCATION ||
        m_mainLightIntensityLoc == INVALID_UNIFORM_LOCATION ||
        m_secondLightIntensityLoc == INVALID_UNIFORM_LOCATION) {
        return false;
    }

    Enable();

    glUniform1i(m_tex0UnitLoc, COLOR_TEXTURE_UNIT_INDEX_0);
    glUniform1i(m_tex1UnitLoc, COLOR_TEXTURE_UNIT_INDEX_1);
    glUniform1i(m_tex2UnitLoc, COLOR_TEXTURE_UNIT_INDEX_2);
    glUniform1i(m_tex3UnitLoc, COLOR_TEXTURE_UNIT_INDEX_3);

    glUseProgram(0);

    return true;
}


void TerrainTechnique::SetVP(const Matrix4f& VP)
{
    glUniformMatrix4fv(m_VPLoc, 1, GL_TRUE, (const GLfloat*)VP.m);
}


void TerrainTechnique::SetMinMaxHeight(float Min, float Max)
{
    glUniform1f(m_minHeightLoc, Min);
    glUniform1f(m_maxHeightLoc, Max);
}


void TerrainTechnique::SetTextureHeights(float Tex0Height, float Tex1Height, float Tex2Height, float Tex3Height)
{
    glUniform1f(m_tex0HeightLoc, Tex0Height);
    glUniform1f(m_tex1HeightLoc, Tex1Height);
    glUniform1f(m_tex2HeightLoc, Tex2Height);
    glUniform1f(m_tex3HeightLoc, Tex3Height);
}


void TerrainTechnique::SetLightDir(const Vector3f& Dir)
{
    Vector3f ReversedLightDir = Dir * -1.0f;
    ReversedLightDir = ReversedLightDir.Normalize();
    glUniform3f(m_reversedLightDirLoc, ReversedLightDir.x, ReversedLightDir.y, ReversedLightDir.z);
}

void TerrainTechnique::SetSecondLightDir(const Vector3f& Dir)
{
    Vector3f ReversedLightDir = Dir * -1.0f;
    ReversedLightDir = ReversedLightDir.Normalize();
    glUniform3f(m_secondLightDirLoc, ReversedLightDir.x, ReversedLightDir.y, ReversedLightDir.z);
}

void TerrainTechnique::SetMainLightIntensity(float Intensity)
{
    glUniform1f(m_mainLightIntensityLoc, Intensity);
}

void TerrainTechnique::SetSecondLightIntensity(float Intensity)
{
    glUniform1f(m_secondLightIntensityLoc, Intensity);
}