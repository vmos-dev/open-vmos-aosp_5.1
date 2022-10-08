/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _COMMANDLISTENER_H__
#define _COMMANDLISTENER_H__

#include <sysutils/FrameworkListener.h>

#include "NetdCommand.h"
#include "NetworkController.h"
#include "TetherController.h"
#include "NatController.h"
#include "PppController.h"
#include "SoftapController.h"
#include "BandwidthController.h"
#include "IdletimerController.h"
#include "InterfaceController.h"
#include "ResolverController.h"
#include "FirewallController.h"
#include "ClatdController.h"

class CommandListener : public FrameworkListener {
    static TetherController *sTetherCtrl;
    static NatController *sNatCtrl;
    static PppController *sPppCtrl;
    static SoftapController *sSoftapCtrl;
    static BandwidthController *sBandwidthCtrl;
    static IdletimerController *sIdletimerCtrl;
    static InterfaceController *sInterfaceCtrl;
    static ResolverController *sResolverCtrl;
    static FirewallController *sFirewallCtrl;
    static ClatdController *sClatdCtrl;

public:
    static NetworkController *sNetCtrl;

    CommandListener();
    virtual ~CommandListener() {}

private:

    class SoftapCmd : public NetdCommand {
    public:
        SoftapCmd();
        virtual ~SoftapCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class InterfaceCmd : public NetdCommand {
    public:
        InterfaceCmd();
        virtual ~InterfaceCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class IpFwdCmd : public NetdCommand {
    public:
        IpFwdCmd();
        virtual ~IpFwdCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class TetherCmd : public NetdCommand {
    public:
        TetherCmd();
        virtual ~TetherCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class NatCmd : public NetdCommand {
    public:
        NatCmd();
        virtual ~NatCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class ListTtysCmd : public NetdCommand {
    public:
        ListTtysCmd();
        virtual ~ListTtysCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class PppdCmd : public NetdCommand {
    public:
        PppdCmd();
        virtual ~PppdCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class BandwidthControlCmd : public NetdCommand {
    public:
        BandwidthControlCmd();
        virtual ~BandwidthControlCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    protected:
        void sendGenericOkFail(SocketClient *cli, int cond);
        void sendGenericOpFailed(SocketClient *cli, const char *errMsg);
        void sendGenericSyntaxError(SocketClient *cli, const char *usageMsg);
    };

    class IdletimerControlCmd : public NetdCommand {
    public:
        IdletimerControlCmd();
        virtual ~IdletimerControlCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class ResolverCmd : public NetdCommand {
    public:
        ResolverCmd();
        virtual ~ResolverCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class FirewallCmd: public NetdCommand {
    public:
        FirewallCmd();
        virtual ~FirewallCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    protected:
        int sendGenericOkFail(SocketClient *cli, int cond);
        static FirewallRule parseRule(const char* arg);
    };

    class ClatdCmd : public NetdCommand {
    public:
        ClatdCmd();
        virtual ~ClatdCmd() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

    class NetworkCommand : public NetdCommand {
    public:
        NetworkCommand();
        virtual ~NetworkCommand() {}
        int runCommand(SocketClient* client, int argc, char** argv);
    private:
        int syntaxError(SocketClient* cli, const char* message);
        int operationError(SocketClient* cli, const char* message, int ret);
        int success(SocketClient* cli);
    };
};

#endif
