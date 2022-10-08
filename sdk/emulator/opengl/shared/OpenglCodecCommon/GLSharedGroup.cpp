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

#include "GLSharedGroup.h"

#include <string.h>

/**** BufferData ****/

BufferData::BufferData() : m_size(0) {};

BufferData::BufferData(GLsizeiptr size, void * data) : m_size(size)
{
    void* buffer = NULL;

    if (size > 0) {
        buffer = m_fixedBuffer.alloc(size);
        if (data) {
            memcpy(buffer, data, size);
        }
    }
}

/**** ProgramData ****/
ProgramData::ProgramData() : m_numIndexes(0),
                             m_initialized(false),
                             m_locShiftWAR(false)
{
    m_Indexes = NULL;
}

void ProgramData::initProgramData(GLuint numIndexes)
{
    m_initialized = true;
    m_numIndexes = numIndexes;
    delete[] m_Indexes;
    m_Indexes = new IndexInfo[numIndexes];
    m_locShiftWAR = false;
}

bool ProgramData::isInitialized()
{
    return m_initialized;
}

ProgramData::~ProgramData()
{
    delete[] m_Indexes;
    m_Indexes = NULL;
}

void ProgramData::setIndexInfo(GLuint index, GLint base, GLint size, GLenum type)
{
    if (index>=m_numIndexes)
        return;
    m_Indexes[index].base = base;
    m_Indexes[index].size = size;
    m_Indexes[index].type = type;
    if (index > 0) {
        m_Indexes[index].appBase = m_Indexes[index-1].appBase +
                                   m_Indexes[index-1].size;
    }
    else {
        m_Indexes[index].appBase = 0;
    }
    m_Indexes[index].hostLocsPerElement = 1;
    m_Indexes[index].flags = 0;
    m_Indexes[index].samplerValue = 0;
}

void ProgramData::setIndexFlags(GLuint index, GLuint flags)
{
    if (index >= m_numIndexes)
        return;
    m_Indexes[index].flags |= flags;
}

GLuint ProgramData::getIndexForLocation(GLint location)
{
    GLuint index = m_numIndexes;
    GLint minDist = -1;
    for (GLuint i=0;i<m_numIndexes;++i)
    {
        GLint dist = location - m_Indexes[i].base;
        if (dist >= 0 &&
            (minDist < 0 || dist < minDist)) {
            index = i;
            minDist = dist;
        }
    }
    return index;
}

GLenum ProgramData::getTypeForLocation(GLint location)
{
    GLuint index = getIndexForLocation(location);
    if (index<m_numIndexes) {
        return m_Indexes[index].type;
    }
    return 0;
}

void ProgramData::setupLocationShiftWAR()
{
    m_locShiftWAR = false;
    for (GLuint i=0; i<m_numIndexes; i++) {
        if (0 != (m_Indexes[i].base & 0xffff)) {
            return;
        }
    }
    // if we have one uniform at location 0, we do not need the WAR.
    if (m_numIndexes > 1) {
        m_locShiftWAR = true;
    }
}

GLint ProgramData::locationWARHostToApp(GLint hostLoc, GLint arrIndex)
{
    if (!m_locShiftWAR) return hostLoc;

    GLuint index = getIndexForLocation(hostLoc);
    if (index<m_numIndexes) {
        if (arrIndex > 0) {
            m_Indexes[index].hostLocsPerElement =
                              (hostLoc - m_Indexes[index].base) / arrIndex;
        }
        return m_Indexes[index].appBase + arrIndex;
    }
    return -1;
}

GLint ProgramData::locationWARAppToHost(GLint appLoc)
{
    if (!m_locShiftWAR) return appLoc;

    for(GLuint i=0; i<m_numIndexes; i++) {
        GLint elemIndex = appLoc - m_Indexes[i].appBase;
        if (elemIndex >= 0 && elemIndex < m_Indexes[i].size) {
            return m_Indexes[i].base +
                   elemIndex * m_Indexes[i].hostLocsPerElement;
        }
    }
    return -1;
}

GLint ProgramData::getNextSamplerUniform(GLint index, GLint* val, GLenum* target)
{
    for (GLint i = index + 1; i >= 0 && i < (GLint)m_numIndexes; i++) {
        if (m_Indexes[i].type == GL_SAMPLER_2D) {
            if (val) *val = m_Indexes[i].samplerValue;
            if (target) {
                if (m_Indexes[i].flags & INDEX_FLAG_SAMPLER_EXTERNAL) {
                    *target = GL_TEXTURE_EXTERNAL_OES;
                } else {
                    *target = GL_TEXTURE_2D;
                }
            }
            return i;
        }
    }
    return -1;
}

bool ProgramData::setSamplerUniform(GLint appLoc, GLint val, GLenum* target)
{
    for (GLuint i = 0; i < m_numIndexes; i++) {
        GLint elemIndex = appLoc - m_Indexes[i].appBase;
        if (elemIndex >= 0 && elemIndex < m_Indexes[i].size) {
            if (m_Indexes[i].type == GL_TEXTURE_2D) {
                m_Indexes[i].samplerValue = val;
                if (target) {
                    if (m_Indexes[i].flags & INDEX_FLAG_SAMPLER_EXTERNAL) {
                        *target = GL_TEXTURE_EXTERNAL_OES;
                    } else {
                        *target = GL_TEXTURE_2D;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool ProgramData::attachShader(GLuint shader)
{
    size_t n = m_shaders.size();
    for (size_t i = 0; i < n; i++) {
        if (m_shaders[i] == shader) {
            return false;
        }
    }
    m_shaders.append(shader);
    return true;
}

bool ProgramData::detachShader(GLuint shader)
{
    size_t n = m_shaders.size();
    for (size_t i = 0; i < n; i++) {
        if (m_shaders[i] == shader) {
            m_shaders.remove(i);
            return true;
        }
    }
    return false;
}

/***** GLSharedGroup ****/

GLSharedGroup::GLSharedGroup() :
    m_buffers(), m_programs(), m_shaders() {}

GLSharedGroup::~GLSharedGroup() {}

BufferData * GLSharedGroup::getBufferData(GLuint bufferId)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    return m_buffers.get(bufferId);
}

void GLSharedGroup::addBufferData(GLuint bufferId, GLsizeiptr size, void * data)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    m_buffers.set(bufferId, new BufferData(size, data));
}

void GLSharedGroup::updateBufferData(GLuint bufferId, GLsizeiptr size, void * data)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    m_buffers.set(bufferId, new BufferData(size, data));
}

GLenum GLSharedGroup::subUpdateBufferData(GLuint bufferId, GLintptr offset, GLsizeiptr size, void * data)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    BufferData * buf = m_buffers.get(bufferId);
    if ((!buf) || (buf->m_size < offset+size) || (offset < 0) || (size<0)) return GL_INVALID_VALUE;

    //it's safe to update now
    memcpy((char*)buf->m_fixedBuffer.ptr() + offset, data, size);
    return GL_NO_ERROR;
}

void GLSharedGroup::deleteBufferData(GLuint bufferId)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    (void) m_buffers.remove(bufferId);
}

void GLSharedGroup::addProgramData(GLuint program)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    m_programs.set(program, new ProgramData());
}

void GLSharedGroup::initProgramData(GLuint program, GLuint numIndexes)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData *pData = m_programs.get(program);
    if (pData) {
        pData->initProgramData(numIndexes);
    }
}

bool GLSharedGroup::isProgramInitialized(GLuint program)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return pData && pData->isInitialized();
}

void GLSharedGroup::deleteProgramData(GLuint program)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    m_programs.remove(program);
}

void GLSharedGroup::attachShader(GLuint program, GLuint shader)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* programData = m_programs.get(program);
    if (programData && programData->attachShader(shader)) {
        refShaderDataLocked(shader);
    }
}

void GLSharedGroup::detachShader(GLuint program, GLuint shader)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* programData = m_programs.get(program);
    if (programData && programData->detachShader(shader)) {
        unrefShaderDataLocked(shader);
    }
}

void GLSharedGroup::setProgramIndexInfo(GLuint program,
                                        GLuint index,
                                        GLint base,
                                        GLint size,
                                        GLenum type,
                                        const char* name)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    if (!pData) {
        return;
    }
    pData->setIndexInfo(index,base,size,type);

    if (type == GL_SAMPLER_2D) {
        size_t n = pData->getNumShaders();
        for (size_t i = 0; i < n; i++) {
            GLuint shaderId = pData->getShader(i);
            ShaderData* shader = m_shaders.get(shaderId);
            if (!shader) continue;
#if 0  // TODO(digit): Understand why samplerExternalNames is always empty?
            ShaderData::StringList::iterator nameIter =
                    shader->samplerExternalNames.begin();
            ShaderData::StringList::iterator nameEnd  =
                    shader->samplerExternalNames.end();
            while (nameIter != nameEnd) {
                if (*nameIter == name) {
                    pData->setIndexFlags(
                            index,
                            ProgramData::INDEX_FLAG_SAMPLER_EXTERNAL);
                    break;
                }
                ++nameIter;
            }
#endif
        }
    }
}


GLenum GLSharedGroup::getProgramUniformType(GLuint program, GLint location)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return pData ? pData->getTypeForLocation(location) : 0;
}

bool  GLSharedGroup::isProgram(GLuint program)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return (pData != NULL);
}

void GLSharedGroup::setupLocationShiftWAR(GLuint program)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    if (pData) pData->setupLocationShiftWAR();
}

GLint GLSharedGroup::locationWARHostToApp(GLuint program,
                                          GLint hostLoc,
                                          GLint arrIndex)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return pData ? pData->locationWARHostToApp(hostLoc, arrIndex) : hostLoc;
}

GLint GLSharedGroup::locationWARAppToHost(GLuint program, GLint appLoc)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return pData ? pData->locationWARAppToHost(appLoc) : appLoc;
}

bool GLSharedGroup::needUniformLocationWAR(GLuint program)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return pData ? pData->needUniformLocationWAR() : false;
}

GLint GLSharedGroup::getNextSamplerUniform(GLuint program,
                                           GLint index,
                                           GLint* val,
                                           GLenum* target) const
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return pData ? pData->getNextSamplerUniform(index, val, target) : -1;
}

bool GLSharedGroup::setSamplerUniform(GLuint program,
                                      GLint appLoc,
                                      GLint val,
                                      GLenum* target)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ProgramData* pData = m_programs.get(program);
    return pData ? pData->setSamplerUniform(appLoc, val, target) : false;
}

bool GLSharedGroup::addShaderData(GLuint shader)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ShaderData* data = new ShaderData;
    data->refcount = 1;
    m_shaders.set(shader, data);
    return true;
}

ShaderData* GLSharedGroup::getShaderData(GLuint shader)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    ShaderData* data = m_shaders.get(shader);
    if (data) {
        data->refcount++;
    }
    return data;
}

void GLSharedGroup::unrefShaderData(GLuint shader)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    unrefShaderDataLocked(shader);
}

void GLSharedGroup::refShaderDataLocked(GLuint shader)
{
    ShaderData* data = m_shaders.get(shader);
    if (data) {
        data->refcount++;
    }
}

void GLSharedGroup::unrefShaderDataLocked(GLuint shader)
{
    ShaderData* data = m_shaders.get(shader);
    if (data && --data->refcount == 0) {
        m_shaders.remove(shader);
    }
}
