/*
 * portother.h, port class for other
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
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

#ifndef __PORTOTHER_H
#define __PORTOTHER_H

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>

class PortOther : public PortBase
{
public:
    PortOther();

    OMX_ERRORTYPE SetPortOtherParam(
        const OMX_OTHER_PARAM_PORTFORMATTYPE *otherparam, bool internal);
    const OMX_OTHER_PARAM_PORTFORMATTYPE *GetPortOtherParam(void);

private:
    OMX_OTHER_PARAM_PORTFORMATTYPE otherparam;
};

/* end of PortOther */

#endif /* __PORTOTHER_H */
