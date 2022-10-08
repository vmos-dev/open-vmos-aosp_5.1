/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _INTERFACE_CONTROLLER_H
#define _INTERFACE_CONTROLLER_H

class InterfaceController {
 public:
	InterfaceController();
	virtual ~InterfaceController();
	int setEnableIPv6(const char *interface, const int on);
	int setIPv6PrivacyExtensions(const char *interface, const int on);
	int setIPv6NdOffload(char* interface, const int on);
	int setMtu(const char *interface, const char *mtu);

 private:
	int writeIPv6ProcPath(const char *interface, const char *setting, const char *value);
	int isInterfaceName(const char *name);
	void setOnAllInterfaces(const char* filename, const char* value);
	void setAcceptRA(const char* value);
	void setAcceptRARouteTable(int tableOrOffset);
	void setIPv6OptimisticMode(const char *value);
};

#endif
