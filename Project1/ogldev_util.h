#ifndef OGLDEV_UTIL_H
#define OGLDEV_UTIL_H


#ifndef _WIN64
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <string.h>
#include <assert.h>
#include <time.h>
#ifndef OGLDEV_VULKAN
#include <glew.h>
#endif
#include "ogldev_types.h"


using namespace std;

bool ReadFile(const char* fileName, string& outFile);
char* ReadBinaryFile(const char* pFileName, int& size);

void WriteBinaryFile(const char* pFilename, const void* pData, int size);

void OgldevError(const char* pFileName, uint line, const char* msg, ...);
void OgldevFileError(const char* pFileName, uint line, const char* pFileError);

#define OGLDEV_ERROR0(msg) OgldevError(__FILE__, __LINE__, msg)
#define OGLDEV_ERROR(msg, ...) OgldevError(__FILE__, __LINE__, msg, __VA_ARGS__)
#define OGLDEV_FILE_ERROR(FileError) OgldevFileError(__FILE__, __LINE__, FileError);

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ZERO_MEM_VAR(var) memset(&var, 0, sizeof(var))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifdef _WIN64
#define SNPRINTF _snprintf_s
#define VSNPRINTF vsnprintf_s
#define RANDOM rand
#define SRANDOM srand((unsigned)time(NULL))
#pragma warning (disable: 4566)
#else
#define SNPRINTF snprintf
#define VSNPRINTF vsnprintf
#define RANDOM random
#define SRANDOM srandom(getpid())
#endif

#define INVALID_UNIFORM_LOCATION 0xffffffff
#define INVALID_OGL_VALUE 0xffffffff

#define NUM_CUBE_MAP_FACES 6

#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }

long long GetCurrentTimeMillis();


#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals |  aiProcess_JoinIdenticalVertices )

#define NOT_IMPLEMENTED printf("Not implemented case in %s:%d\n", __FILE__, __LINE__); exit(0);

#ifndef OGLDEV_VULKAN
#define GLExitIfError                                                          \
{                                                                               \
    GLenum Error = glGetError();                                                \
                                                                                \
    if (Error != GL_NO_ERROR) {                                                 \
        printf("OpenGL error in %s:%d: 0x%x\n", __FILE__, __LINE__, Error);     \
        exit(0);                                                                \
    }                                                                           \
}

#define GLCheckError() (glGetError() == GL_NO_ERROR)

void gl_check_error(const char* function, const char* file, int line);

#define CHECK_GL_ERRORS

#ifdef CHECK_GL_ERRORS
#define GCE gl_check_error(__FUNCTION__, __FILE__, __LINE__);
#else
#define GCE
#endif

void glDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam);
#endif

string GetDirFromFilename(const string& Filename);

#define MAX_BONES (200)

#define CLAMP(Val, Start, End) Val = std::min(std::max((Val), (Start)), (End));

int GetGLMajorVersion();
int GetGLMinorVersion();

int IsGLVersionHigher(int MajorVer, int MinorVer);

#endif  /* OGLDEV_UTIL_H */