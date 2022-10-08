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

#ifndef _RESPONSECODE_H
#define _RESPONSECODE_H

class ResponseCode {
    // Keep in sync with
    // frameworks/base/services/java/com/android/server/NetworkManagementService.java
public:
    // 100 series - Requestion action was initiated; expect another reply
    // before proceeding with a new command.
    static const int ActionInitiated           = 100;
    static const int InterfaceListResult       = 110;
    static const int TetherInterfaceListResult = 111;
    static const int TetherDnsFwdTgtListResult = 112;
    static const int TtyListResult             = 113;
    static const int TetheringStatsListResult  = 114;
    static const int TetherDnsFwdNetIdResult   = 115;

    // 200 series - Requested action has been successfully completed
    static const int CommandOkay               = 200;
    static const int TetherStatusResult        = 210;
    static const int IpFwdStatusResult         = 211;
    static const int InterfaceGetCfgResult     = 213;
    static const int SoftapStatusResult        = 214;
    static const int UsbRNDISStatusResult      = 215;
    static const int InterfaceRxCounterResult  = 216;
    static const int InterfaceTxCounterResult  = 217;
    static const int InterfaceRxThrottleResult = 218;
    static const int InterfaceTxThrottleResult = 219;
    static const int QuotaCounterResult        = 220;
    static const int TetheringStatsResult      = 221;
    static const int DnsProxyQueryResult       = 222;
    static const int ClatdStatusResult         = 223;

    // 400 series - The command was accepted but the requested action
    // did not take place.
    static const int OperationFailed           = 400;
    static const int DnsProxyOperationFailed   = 401;
    static const int ServiceStartFailed        = 402;
    static const int ServiceStopFailed         = 403;

    // 500 series - The command was not accepted and the requested
    // action did not take place.
    static const int CommandSyntaxError = 500;
    static const int CommandParameterError = 501;

    // 600 series - Unsolicited broadcasts
    static const int InterfaceChange                = 600;
    static const int BandwidthControl               = 601;
    static const int ServiceDiscoveryFailed         = 602;
    static const int ServiceDiscoveryServiceAdded   = 603;
    static const int ServiceDiscoveryServiceRemoved = 604;
    static const int ServiceRegistrationFailed      = 605;
    static const int ServiceRegistrationSucceeded   = 606;
    static const int ServiceResolveFailed           = 607;
    static const int ServiceResolveSuccess          = 608;
    static const int ServiceSetHostnameFailed       = 609;
    static const int ServiceSetHostnameSuccess      = 610;
    static const int ServiceGetAddrInfoFailed       = 611;
    static const int ServiceGetAddrInfoSuccess      = 612;
    static const int InterfaceClassActivity         = 613;
    static const int InterfaceAddressChange         = 614;
    static const int InterfaceDnsInfo               = 615;
    static const int RouteChange                    = 616;
};
#endif
