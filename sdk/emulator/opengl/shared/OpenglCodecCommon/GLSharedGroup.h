/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef _GL_SHARED_GROUP_H_
#define _GL_SHARED_GROUP_H_

#include "emugl/common/id_to_object_map.h"
#include "emugl/common/mutex.h"
#include "emugl/common/pod_vector.h"
#include "emugl/common/smart_ptr.h"

#define GL_API
#ifndef ANDROID
#define GL_APIENTRY
#define GL_APIENTRYP
#endif

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include "ErrorLog.h"
#include "FixedBuffer.h"

struct BufferData {
    BufferData();
    BufferData(GLsizeiptr size, void * data);
    GLsizeiptr  m_size;
    FixedBuffer m_fixedBuffer;
};

class ProgramData {
private:
    typedef struct _IndexInfo {
        GLint base;
        GLint size;
        GLenum type;
        GLint appBase;
        GLint hostLocsPerElement;
        GLuint flags;
        GLint samplerValue; // only set for sampler uniforms
    } IndexInfo;

    GLuint m_numIndexes;
    IndexInfo* m_Indexes;
    bool m_initialized;
    bool m_locShiftWAR;

    emugl::PodVector<GLuint> m_shaders;

public:
    enum {
        INDEX_FLAG_SAMPLER_EXTERNAL = 0x00000001,
    };

    ProgramData();
    void initProgramData(GLuint numIndexes);
    bool isInitialized();
    virtual ~ProgramData();
    void setIndexInfo(GLuint index, GLint base, GLint size, GLenum type);
    void setIndexFlags(GLuint index, GLuint flags);
    GLuint getIndexForLocation(GLint location);
    GLenum getTypeForLocation(GLint location);

    bool needUniformLocationWAR() const { return m_locShiftWAR; }
    void setupLocationShiftWAR();
    GLint locationWARHostToApp(GLint hostLoc, GLint arrIndex);
    GLint locationWARAppToHost(GLint appLoc);

    GLint getNextSamplerUniform(GLint index, GLint* val, GLenum* target);
    bool setSamplerUniform(GLint appLoc, GLint val, GLenum* target);

    bool attachShader(GLuint shader);
    bool detachShader(GLuint shader);
    size_t getNumShaders() const { return m_shaders.size(); }
    GLuint getShader(size_t i) const { return m_shaders[i]; }
};

struct ShaderData {
#if 0  // TODO(digit): Undertand why this is never used?
    typedef android::List<android::String8> StringList;
    StringList samplerExternalNames;
#endif
    int refcount;
};

class GLSharedGroup {
private:
    emugl::IdToObjectMap<BufferData> m_buffers;
    emugl::IdToObjectMap<ProgramData> m_programs;
    emugl::IdToObjectMap<ShaderData> m_shaders;
    mutable emugl::Mutex m_lock;

    void refShaderDataLocked(GLuint shader);
    void unrefShaderDataLocked(GLuint shader);

public:
    GLSharedGroup();
    ~GLSharedGroup();
    BufferData * getBufferData(GLuint bufferId);
    void    addBufferData(GLuint bufferId, GLsizeiptr size, void * data);
    void    updateBufferData(GLuint bufferId, GLsizeiptr size, void * data);
    GLenum  subUpdateBufferData(GLuint bufferId, GLintptr offset, GLsizeiptr size, void * data);
    void    deleteBufferData(GLuint);

    bool    isProgram(GLuint program);
    bool    isProgramInitialized(GLuint program);
    void    addProgramData(GLuint program); 
    void    initProgramData(GLuint program, GLuint numIndexes);
    void    attachShader(GLuint program, GLuint shader);
    void    detachShader(GLuint program, GLuint shader);
    void    deleteProgramData(GLuint program);
    void    setProgramIndexInfo(GLuint program, GLuint index, GLint base, GLint size, GLenum type, const char* name);
    GLenum  getProgramUniformType(GLuint program, GLint location);
    void    setupLocationShiftWAR(GLuint program);
    GLint   locationWARHostToApp(GLuint program, GLint hostLoc, GLint arrIndex);
    GLint   locationWARAppToHost(GLuint program, GLint appLoc);
    bool    needUniformLocationWAR(GLuint program);
    GLint   getNextSamplerUniform(GLuint program, GLint index, GLint* val, GLenum* target) const;
    bool    setSamplerUniform(GLuint program, GLint appLoc, GLint val, GLenum* target);

    bool    addShaderData(GLuint shader);
    // caller must hold a reference to the shader as long as it holds the pointer
    ShaderData* getShaderData(GLuint shader);
    void    unrefShaderData(GLuint shader);
};

typedef emugl::SmartPtr<GLSharedGroup> GLSharedGroupPtr; 

#endif //_GL_SHARED_GROUP_H_
