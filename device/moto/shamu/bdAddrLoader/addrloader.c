/*
 * Copyright 2013 The Android Open Source Project
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#define LOG_TAG "bdAddrLoader"

#include <cutils/log.h>
#include <cutils/properties.h>

#define FILE_PATH_MAX   100
#define BD_ADDR_LEN  6
#define BD_ADDR_STR_LEN 18


#define ARG_TYPE_PATH_FILE  0x11
#define ARG_TYPE_PATH_PROP  0x12

#define ARG_TYPE_DATA_HEX   0x21
#define ARG_TYPE_DATA_ASCII 0x22

typedef struct _ArgEl {
   const char *szSrc;    // Source Path
   int nPathType;        // Type of Source Path
   int nDataType;        // Type of Data
} ArgEl;

typedef ArgEl InArg;

#define DEFAULT_BDADDR_PROP "persist.service.bdroid.bdaddr"

typedef struct _OutArg {
   ArgEl dest;
   char  cSeperator;    // a character to be used for sperating like ':' of "XX:XX:XX:XX:XX:XX"
   char  bPrintOut;     // Print out bd addr in standard out or not
} OutArg;

typedef struct _LoadedData {
    union {
       unsigned char bin[BD_ADDR_LEN];
       char sz[BD_ADDR_STR_LEN];
    }data;
    int nDataType;
} LoadedBDAddr;

typedef enum _res {
    SUCCESS = 0,
    FAIL
} Res;

int hexa_to_ascii(const unsigned char* hexa, char* ascii, int nHexLen)
{
    int i, j;
    char hex_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                'A', 'B', 'C', 'D', 'E', 'F'};

    for (i = 0, j = 0; i <nHexLen; i++, j += 2) {
        ascii[j] = hex_table[hexa[i] >> 4];
        ascii[j + 1] = hex_table[hexa[i] & 0x0F];
    }

    ascii[nHexLen*2] = '\0';

    ALOGI("hex_to_ascii() - converted Data (%s)", ascii);

    return SUCCESS;
}

int readBDAddrData(const char* szFilePath, unsigned char* addrData, int nDataLen)
{
    int nFd, nRdCnt;

    nFd = open(szFilePath, O_RDONLY);

    if (nFd < 0) {
        ALOGW("There is no Address File in FTM area : %s\n", szFilePath);
        return FAIL;
    }

    nRdCnt = read(nFd, addrData, nDataLen);
    if (nRdCnt != nDataLen) {
        ALOGE("Fail to read Address data from FTM area\n");
        close(nFd);
        return FAIL;
    }
    close(nFd);
    return SUCCESS;
}

void formattingBdAddr(char *szBDAddr, const char cSep)
{
    int i = 1, j = 0;
    int pos = 0;
    for (i=1; i<BD_ADDR_LEN; i++) {
       pos = strlen(szBDAddr);
       for (j=0; j<(BD_ADDR_LEN*2)-i*2; j++) {
          szBDAddr[pos-j] = szBDAddr[pos-j-1];
       }
       szBDAddr[pos-j]=cSep;
    }
}

int readBDAddr(InArg inArg, LoadedBDAddr *loadedBDAddr)
{
    Res res = FAIL;
    unsigned char addrData[BD_ADDR_LEN] = {0,};
    int nDataLen = 0;

    ALOGI("Read From %s by Path type(0x%2x), Data type (0x%2x)",
            inArg.szSrc, inArg.nPathType, inArg.nDataType);

    if (inArg.nPathType == ARG_TYPE_PATH_FILE) {
        switch (inArg.nDataType) {
        case ARG_TYPE_DATA_HEX:
            if (!readBDAddrData(inArg.szSrc, loadedBDAddr->data.bin, BD_ADDR_LEN)) {
                loadedBDAddr->nDataType = ARG_TYPE_DATA_HEX;
                return SUCCESS;
            }
            break;
        case ARG_TYPE_DATA_ASCII:
           if (!readBDAddrData(inArg.szSrc, (unsigned char *)loadedBDAddr->data.sz, BD_ADDR_STR_LEN)) {
                loadedBDAddr->nDataType = ARG_TYPE_DATA_ASCII;
                return SUCCESS;
            }
            break;
        default:
            return FAIL;
        }
    } else if (inArg.nPathType == ARG_TYPE_PATH_PROP) {
        char prop_value[PROPERTY_VALUE_MAX];
        switch (inArg.nDataType) {
        case ARG_TYPE_DATA_HEX:
            if (property_get(inArg.szSrc, prop_value, "") >= 0 && strlen(prop_value) < BD_ADDR_LEN) {
                strlcpy((char *)loadedBDAddr->data.bin, prop_value, BD_ADDR_LEN);
                loadedBDAddr->nDataType = ARG_TYPE_DATA_HEX;
                return SUCCESS;
            }
            break;
        case ARG_TYPE_DATA_ASCII:
            if (property_get(inArg.szSrc, prop_value, "") >= 0 && strlen(prop_value) < BD_ADDR_STR_LEN) {
                strlcpy(loadedBDAddr->data.sz, prop_value, BD_ADDR_STR_LEN);
                loadedBDAddr->nDataType = ARG_TYPE_DATA_ASCII;
                return SUCCESS;
            }
            break;
        default:
            return FAIL;
        }
    } else {
        ALOGE("Error invalid argument : (%d)", inArg.nPathType);
    }

    ALOGE("Fail to read BDAddr from %s", inArg.szSrc);
    return FAIL;
}

int writeBDAddr(OutArg outArg, LoadedBDAddr *loadedBDAddr)
{
    char szTmp[BD_ADDR_STR_LEN] = {0,};

    ALOGI("Output Data type(0x%2x), bPrintout(%d), bPath(%s)",
        outArg.dest.nDataType, outArg.bPrintOut, outArg.dest.szSrc);

    ALOGI("Loaded Data type(0x%2x)", loadedBDAddr->nDataType);

    if (outArg.dest.nDataType == ARG_TYPE_DATA_ASCII
            && loadedBDAddr->nDataType == ARG_TYPE_DATA_HEX) {
        if (!hexa_to_ascii(loadedBDAddr->data.bin, szTmp, BD_ADDR_LEN)) {
            memcpy(loadedBDAddr->data.sz, szTmp, BD_ADDR_STR_LEN);
            loadedBDAddr->nDataType = ARG_TYPE_DATA_ASCII;
        } else {
            ALOGE("Fail to convert data");
            return FAIL;
        }
    }

    if (loadedBDAddr->nDataType == ARG_TYPE_DATA_ASCII) {
       // check out which addr data is already formated
       if (strchr(loadedBDAddr->data.sz, '.') == NULL
               && strchr(loadedBDAddr->data.sz, ':') == NULL) {
           formattingBdAddr(loadedBDAddr->data.sz, outArg.cSeperator);
       }
    }
    // print out szBDAddr
    if (outArg.bPrintOut
            && loadedBDAddr->nDataType == ARG_TYPE_DATA_ASCII
            && strlen(loadedBDAddr->data.sz)==(BD_ADDR_STR_LEN-1)) {
       printf("%s",loadedBDAddr->data.sz);
       if (property_set(DEFAULT_BDADDR_PROP, loadedBDAddr->data.sz) < 0)
           ALOGE("Failed to set address in prop %s", DEFAULT_BDADDR_PROP);
    } else {
       ALOGE("Invalid Data is loaded : %s", loadedBDAddr->data.sz);
       return FAIL;
    }
    // TODO :: writing File or Property
    return SUCCESS;
}

int main(int argc, char *argv[])
{
    int nFd, nRdCnt;
    int c;

    InArg inArg;
    OutArg outArg;
    LoadedBDAddr loadedBDAddr;

    //initialize arg
    memset(&inArg, 0, sizeof(InArg));
    memset(&outArg, 0, sizeof(OutArg));
    memset(&loadedBDAddr, 0, sizeof(LoadedBDAddr));

    //load args;
    while((c=getopt(argc, argv, ":f:p:hsx")) != -1){
        switch(c){
        case 'f': // input path
            if (optarg != NULL) {
                ALOGI("option : f=%s", optarg);
                inArg.szSrc = optarg;
            } else {
                ALOGW("Invalid Argument(%s) of input path", optarg);
            }
            inArg.nPathType = ARG_TYPE_PATH_FILE;
            break;
        case 'p': // output path
            if (optarg != NULL) {
                ALOGI("option : p=%s", optarg);
                inArg.szSrc = optarg;
            } else {
                ALOGW("Invalid Argument(%s) of out Path", optarg);
            }
            inArg.nPathType = ARG_TYPE_PATH_PROP;
            break;
        case 'h': // data type to be read is hex
            ALOGI("option : h");
            inArg.nDataType = ARG_TYPE_DATA_HEX;
            break;
        case 's': // data type to be read is ascii
            ALOGI("option : s");
            inArg.nDataType = ARG_TYPE_DATA_ASCII;
            break;
        case 'x':
            ALOGI("option : x");
            outArg.bPrintOut = 1; //true
            break;
        default:
            ALOGW("Unknown option : %c", c);
            break;
        }
    }

    // setting up Arguments with default value
    outArg.cSeperator = ':';
    outArg.dest.nDataType = ARG_TYPE_DATA_ASCII;

    // load bd addr and print out bd addr in formated ascii
    if (readBDAddr(inArg, &loadedBDAddr)) {
        ALOGE("Fail to load data !!");
        return FAIL;
    }

    if (writeBDAddr(outArg, &loadedBDAddr)) {
        ALOGE("Fail to write data !!");
        return FAIL;
    }

    return 1;
}
