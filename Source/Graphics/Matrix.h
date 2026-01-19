#pragma once

#include "Base/Types.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

#ifdef (DAEDALUS_CTR)
#include <GL/picaGL.h>
#endif

#if defined(DAEDALUS_PSP)

    #include <psptypes.h>
    #include <pspgu.h>

    using EGuMatrixType = int; // GU_PROJECTION, GU_VIEW, etc.

    inline void SetMatrix(EGuMatrixType type, const glm::mat4& m)
    {
        alignas(16) ScePspFMatrix4 tmp;
        std::memcpy(&tmp, glm::value_ptr(m), sizeof(tmp));
        sceGuSetMatrix(type, &tmp);
    }

#elif defined(DAEDALUS_GL) || defined(DAEDALUS_CTR)


    // Map your “GU_*” names to GL matrix modes on desktop.
    // #define GU_PROJECTION GL_PROJECTION
    enum eMatrixType
    {
        GU_PROJECTION = GL_PROJECTION,
        GU_MODELVIEW  = GL_MODELVIEW,
        GU_CLAMP      = GL_CLAMP,
    };
extern glm::mat4 gProjection;

inline void SetMatrix(eMatrixType type, const glm::mat4& m)
{
    if (type == GU_PROJECTION)
        gProjection = m;
}
#else

    // other backends...

#endif