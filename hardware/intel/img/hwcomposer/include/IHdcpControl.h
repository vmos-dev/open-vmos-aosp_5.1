/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#ifndef IHDCP_CONTROL_H
#define IHDCP_CONTROL_H

namespace android {
namespace intel {

typedef void (*HdcpStatusCallback)(bool success, void *userData);

class IHdcpControl {
public:
    IHdcpControl() {}
    virtual ~IHdcpControl() {}
public:
    virtual bool startHdcp() = 0;
    virtual bool startHdcpAsync(HdcpStatusCallback cb, void *userData) = 0;
    virtual bool stopHdcp() = 0;
};

} // namespace intel
} // namespace android


#endif /* IHDCP_CONTROL_H */
