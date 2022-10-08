/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*!
 * \file      ExynosBuffer.h
 * \brief     header file for ExynosBuffer
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2011/06/02
 *
 * <b>Revision History: </b>
 * - 2010/06/03 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 * - 2012/03/14 : sangwoo.park(sw5771.park@samsung.com) \n
 *   Change file, struct name to ExynosXXX.
 *
 */

#ifndef EXYNOS_BUFFER_H_
#define EXYNOS_BUFFER_H_

#include <sys/types.h>

//! Buffer information
/*!
 * \ingroup Exynos
 */
struct ExynosBuffer
{
public:
    //! Buffer type
    enum BUFFER_TYPE
    {
        BUFFER_TYPE_BASE     = 0,
        BUFFER_TYPE_VIRT     = 1,      //!< virtual address
        BUFFER_TYPE_PHYS     = 1 << 1, //!< physical address
        BUFFER_TYPE_RESERVED = 1 << 2, //!< reserved type
        BUFFER_TYPE_FD       = 1 << 3, //!< physical address
        BUFFER_TYPE_MAX,
    };

    //! Buffer virtual address
    union {
        char *p;       //! single address.
        char *extP[3]; //! Y Cb Cr.
    } virt;

    //! Buffer physical address
    union {
        unsigned int p;       //! single address.
        unsigned int extP[3]; //! Y Cb Cr.
    } phys;

    //! Buffer file descriptors
    union {
	int fd;
	int extFd[3];
    } fd;

    //! Buffer reserved id
    union {
        unsigned int p;       //! \n
        unsigned int extP[3]; //! \n
    } reserved;

    //! Buffer size
    union {
        unsigned int s;
        unsigned int extS[3];
    } size;

#ifdef __cplusplus
    //! Constructor
    ExynosBuffer()
    {
        for (int i = 0; i < 3; i++) {
            virt.    extP[i] = NULL;
            phys.    extP[i] = 0;
	    fd.      extFd[i] = -1;
            reserved.extP[i] = 0;
            size.    extS[i] = 0;
        }
    }

    //! Constructor
    ExynosBuffer(const ExynosBuffer *other)
    {
        for (int i = 0; i < 3; i++) {
            virt.    extP[i] = other->virt.extP[i];
            phys.    extP[i] = other->phys.extP[i];
	    fd.      extFd[i] = other->fd.extFd[i];
            reserved.extP[i] = other->reserved.extP[i];
            size.    extS[i] = other->size.extS[i];
        }
    }

    //! Operator(=) override
    ExynosBuffer& operator =(const ExynosBuffer &other)
    {
        for (int i = 0; i < 3; i++) {
            virt.    extP[i] = other.virt.extP[i];
            phys.    extP[i] = other.phys.extP[i];
	    fd.      extFd[i] = other.fd.extFd[i];	
            reserved.extP[i] = other.reserved.extP[i];
            size.    extS[i] = other.size.extS[i];
        }
        return *this;
    }

    //! Operator(==) override
    bool operator ==(const ExynosBuffer &other) const
    {
        return (   virt.    extP[0] == other.virt.extP[0]
                && virt.    extP[1] == other.virt.extP[1]
                && virt.    extP[2] == other.virt.extP[2]
                && phys.    extP[0] == other.phys.extP[0]
                && phys.    extP[1] == other.phys.extP[1]
                && phys.    extP[2] == other.phys.extP[2]
                && fd.      extFd[0] == other.fd.extFd[0]
                && fd.      extFd[1] == other.fd.extFd[1]
                && fd.      extFd[2] == other.fd.extFd[2]
                && reserved.extP[0] == other.reserved.extP[0]
                && reserved.extP[1] == other.reserved.extP[1]
                && reserved.extP[2] == other.reserved.extP[2]
                && size.    extS[0] == other.size.extS[0]
                && size.    extS[1] == other.size.extS[1]
                && size.    extS[2] == other.size.extS[2]);
    }

    //! Operator(!=) override
    bool operator !=(const ExynosBuffer &other) const
    {
        // use operator(==)
        return !(*this == other);
    }

    //! Get Buffer type
    static int BUFFER_TYPE(ExynosBuffer *buf)
    {
        int type = BUFFER_TYPE_BASE;
        if (buf->virt.p)
            type |= BUFFER_TYPE_VIRT;
        if (buf->phys.p)
            type |= BUFFER_TYPE_PHYS;
	if (buf->fd.fd >= 0)
            type |= BUFFER_TYPE_FD;		
        if (buf->reserved.p)
            type |= BUFFER_TYPE_RESERVED;

        return type;
    }
#endif
};

#endif //EXYNOS_BUFFER_H_
