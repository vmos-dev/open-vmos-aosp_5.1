/**
 * Copyright(c) 2011 Trusted Logic.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Trusted Logic nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SMC_PROPERTIES_H__
#define _SMC_PROPERTIES_H__


#ifdef __cplusplus
extern "C" {
#endif

/* definition of public properties (Driver and tf_daemon) */
#define FILE_SYSTEM_FILE_NAME             "filesystem.storage.fileName"
#define KEYSTORE_SYSTEM_FILE_NAME         "filesystem.keystore.fileName"
#define KEYSTORE_USER_FILE_NAME           "filesystem.keystore.user.fileName"
#define SUPER_PARTITION_FILE_NAME         "filesystem.mc.fileName"


/**
 * parse the config file
 * @param configFile the path of the configuration file
 * @return 0 if succeed, else 1
 */
int smcPropertiesParse(const char *pConfigFile);


/**
 * get the value of a property
 * @param pProp we are asking the value of this property
 * @return the value if found, else NULL
 */
char *smcGetPropertyAsString(char *pProp);


/**
 * get the value of a property
 * @param pProp we are asking the value of this property
 * @param pVal the value of the property
 * @return 0 if found, else 1
 */
int smcGetPropertyAsInt(char *pProp, int *pVal);


#ifdef __cplusplus
}
#endif

#endif /* _SMC_PROPERTIES_H__ */
