/*
 * Copyright 2012 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "wfc_util_log.h"

/*
static void wfc_util_printf(char *pSPointer, int length)
{
	char *pPrintBuff = NULL;

	if( NULL == pSPointer || 0 >= length ) {
		wfc_util_log_error("wfc_util_printf : unvalid parameters");
		return;
	}

	wfc_util_log_error("wfc_util_printf : lenght is (%d)", length);
	pPrintBuff = malloc(length+1);

	if( NULL != pPrintBuff ) {
		memset( pPrintBuff, 0, (length+1) );
		memcpy(pPrintBuff, pSPointer, length);

		wfc_util_log_error("wfc_util_printf : %s", pPrintBuff);

		free(pPrintBuff);
	} else {
		wfc_util_log_error("wfc_util_printf : can not malloc(%d)", (length+1));
	}
	return;
}
*/

static void wfc_util_finsert_new_string(int fd, char **ppReadedBuff, char *pNewStringValue, char *pEndOfCfg)
{
	off_t sz_file;
	int   sz_backupBuff = 0;
	char *pReadBuff = NULL, *pBackupBuff = NULL;
	char *pSPointer = NULL, *pETagPointer = NULL;

	if( 0 == fd || NULL == pNewStringValue || 0 == strlen(pNewStringValue) ) {
		wfc_util_log_error("wfc_util_finsert_new_string : unvalid parameters");
		return;
	}

	if( NULL == ppReadedBuff) {
		// TODO:
		return;
	} else {
		pReadBuff = *ppReadedBuff;
	}

	/*
	 * find END TAG string
	 */
	pETagPointer = strstr(pReadBuff, pEndOfCfg);
	pSPointer = pETagPointer - 1;

	/*
	 * calcurate file size and size of the tail of file
	 */
	sz_file = lseek( fd,  0, SEEK_END );
	sz_backupBuff = (int)sz_file - (pETagPointer - pReadBuff);

	/*
	 * prefare the buffer to store the tail of file
	 */
	pBackupBuff = malloc(sz_backupBuff);

	if( NULL != pBackupBuff ) {
		/*
		 * copy the tail of file.
		 */
		memset( pBackupBuff, 0, sz_backupBuff );
		memcpy( pBackupBuff, pETagPointer, sz_backupBuff );

		/*
		 * write new string.
		 */
		lseek( fd, (int)(pSPointer-pReadBuff), SEEK_SET );
		write( fd, pNewStringValue, strlen(pNewStringValue));

		/*
		 * update pETagPointer.
		 */
		pETagPointer = pSPointer + strlen(pNewStringValue);

		/*
		 * write the tail of file.
		 */
		lseek( fd, (int)(pETagPointer-pReadBuff), SEEK_SET );
		write( fd, pBackupBuff, sz_backupBuff );

		ftruncate(fd, sz_file + strlen(pNewStringValue) - 1); /* we use "-1" becasue of "pSPointer = pETagPointer - 1"*/

		free(pBackupBuff);

		/*
		 * make new *ppReadedBuff
		 */
		if( NULL != ppReadedBuff) {
			// TODO:
		}
	} else {
		wfc_util_log_error("wfc_util_finsert_new_string : can not malloc(%d)", sz_backupBuff);
	}

	return;
}

static void wfc_util_fupdate_string(int fd, char **ppReadedBuff,
                                    char *pETagPointer, char *pSValuePointer, char *pNewValueString)
{
	off_t sz_file;
	int   sz_newReadBuff = 0;
	char *pReadBuff = NULL, *pNewReadBuff = NULL, *pCurReadBuff = NULL;

	if( 0 == fd ) {
		wfc_util_log_error("wfc_util_fupdate_string : unvalid parameters");
		return;
	}

	if( NULL == ppReadedBuff) {
		// TODO:
		return;
	} else {
		pReadBuff = *ppReadedBuff;
	}

	/*
	 * calcurate file size and new file size
	 */
	sz_file  = lseek( fd,  0, SEEK_END );
	sz_newReadBuff = (int)sz_file - (int)(pETagPointer - pSValuePointer) + strlen(pNewValueString);

	/*
	 * prefare the buffer to read file
	 */
	pNewReadBuff = malloc(sz_newReadBuff);

	if( NULL != pNewReadBuff ) {
		/*
		 * copy buffer
		 */
		memset( pNewReadBuff, 0, sz_file );
		pCurReadBuff = pNewReadBuff;
		memcpy( pNewReadBuff, pReadBuff, (int)(pSValuePointer-pReadBuff) );
		pCurReadBuff += (int)(pSValuePointer-pReadBuff);

		/*
		 * copy new value string
		 */
		memcpy( pCurReadBuff, pNewValueString, strlen(pNewValueString));
		pCurReadBuff += strlen(pNewValueString);

		/*
		 * copy the remained buffer
		 */
		memcpy( pCurReadBuff, pETagPointer, ((int)(sz_file) - (int)(pETagPointer - pReadBuff) + 1));

		/*
		 * write file and update the file size
		 */
		lseek( fd,  0, SEEK_SET );
		write( fd, pNewReadBuff, sz_newReadBuff);
		ftruncate(fd, sz_newReadBuff);

		free(pNewReadBuff);
	} else {
		wfc_util_log_error("wfc_util_fupdate_string : can not malloc(%d)", (int)sz_newReadBuff);
	}

	return;
}

/*
 * wfc_util_fset_buffer
 *
 * return : void
 */
void wfc_util_fset_buffer(char *pFileName, int positionStart, unsigned char *pNewValue, int newValueLength)
{
	int fd;
	off_t sz_file;
	char *pReadBuff = NULL;

	fd = open( pFileName, O_RDWR );

	if( fd >= 0 ) {
		/*
		 * calcurate file size
		 */
		sz_file  = lseek( fd,  0, SEEK_END );

		/*
		 * prefare the buffer to read file
		 */
		pReadBuff = malloc(sz_file + 1);  // null terminated

		if( NULL != pReadBuff ) {
			/*
			 * read file
			 */
			memset( pReadBuff, 0, sz_file + 1);
			lseek( fd,  0, SEEK_SET );
			read( fd, pReadBuff, sz_file );

			if(sz_file >= (positionStart+newValueLength)) {
				lseek( fd, positionStart, SEEK_SET );
				write( fd, pNewValue, newValueLength );
			} else {
				/*
				 * insert with new length value buffer
				 */
				wfc_util_log_error("wfc_util_fset_buffer : file size(%d) is less than to write position(%d)", (int)sz_file, (positionStart+newValueLength));
				// TODO:
			}

			free(pReadBuff);
		} else {
			wfc_util_log_error("wfc_util_fset_buffer : can not malloc(%d)", (int)sz_file);
		}

		if ( -1 == fsync( fd ) ) {
			wfc_util_log_error("wfc_util_fset_buffer : fail to fsync()");
		}

		close( fd );
	} else {
		wfc_util_log_error("wfc_util_fset_buffer : can not open file");
	}

	return;
}

/*
 * wfc_util_fget_buffer
 *
 * return : it will return the length of the stored buffer value if procedure is success
 *          or will return 0 if not.
 */
int wfc_util_fget_buffer(char *pFileName, int positionStart, int lengthToRead, unsigned char *pValueBuff, int buffLength)
{
	int result = 0;
	int fd;
	off_t sz_file;
	char *pReadBuff = NULL;
	char *pSValuePointer = NULL, *pETagPointer = NULL;

	fd = open( pFileName, O_RDONLY );

	if( fd >= 0 ) {
		/*
		 * calcurate file size
		 */
		sz_file  = lseek( fd,  0, SEEK_END );

		if(sz_file >= (positionStart+lengthToRead)) {
			/*
			 * prefare the buffer to read file
			 */
			pReadBuff = malloc(sz_file + 1); // null terminated

			if( NULL != pReadBuff ) {
				/*
				 * read file
				 */
				memset( pReadBuff, 0, sz_file + 1 );
				lseek( fd,  0, SEEK_SET );
				read( fd, pReadBuff, sz_file );

				/*
				 * calculate the start buffer pointer
				 */
				pSValuePointer = pReadBuff + positionStart;

				/*
				 * calculate the end buffer pointer
				 */
				pETagPointer = pSValuePointer + lengthToRead;

				/*
				 * read the string value
				 */
				if( buffLength >= (int)(pETagPointer-pSValuePointer) ) {
					memset( pValueBuff, 0, buffLength );
					memcpy( pValueBuff, pSValuePointer, (int)(pETagPointer-pSValuePointer) );
					result = (int)(pETagPointer-pSValuePointer);
				} else {
					wfc_util_log_error("wfc_util_fget_buffer : not enough string value buffer(%d)", (int)(pETagPointer-pSValuePointer));
				}

				free(pReadBuff);
			} else {
				wfc_util_log_error("wfc_util_fget_buffer : can not malloc(%d)", (int)sz_file);
			}
		} else {
			wfc_util_log_error("wfc_util_fget_buffer : file size(%d) is less than to read position(%d)", (int)sz_file, (positionStart+lengthToRead));
		}
		close( fd );
	} else {
		wfc_util_log_error("wfc_util_fget_buffer : can not open file");
	}

	return result;
}

/*
 * wfc_util_fset_string
 *
 * The following format string will be added or updated to the file pFileName.
 * [pSTagString][pNewValueString][pETagString]
 *
 * pFileName       : file name and path
 * pEndOfCfg       : tag string to notify the end of configuration file
 * pSTagString     : tag string to notify purpose of the value
 * pETagString     : tag string to notify the end of the value
 * pNewValueString : string to set for pSTagString
 *
 * return : void
 */
void wfc_util_fset_string(char *pFileName, char *pEndOfCfg, char *pSTagString, char *pETagString, char *pNewValueString)
{
	int fd;
	off_t sz_file;
	int   sz_NewValueBuff = 0;
	char *pReadBuff = NULL, *pNewValueBuff = NULL;
	char *pSPointer = NULL, *pETagPointer = NULL, *pSValuePointer = NULL;

	fd = open( pFileName, O_RDWR );

	if( fd >= 0 ) {
		/*
		 * calcurate file size
		 */
		sz_file  = lseek( fd,  0, SEEK_END );

		/*
		 * prefare the buffer to read file
		 */
		if (sz_file > 0)
			pReadBuff = malloc(sz_file + 1); // null terminated

		if( NULL != pReadBuff ) {
			/*
			 * read file
			 */
			memset( pReadBuff, 0x00, sz_file + 1);
			if(lseek(fd, 0, SEEK_SET) != 0) {
				wfc_util_log_error("lseek failure");
			}
			read( fd, pReadBuff, sz_file );

			/* WBT fix, make sure it is terminated with \0 */
			pReadBuff[sz_file] = '\0';

			/*
			 * find TAG string
			 */
			pSPointer = strstr(pReadBuff, pSTagString);

			if(NULL != pSPointer) {
				/*
				 * find END OF LINE string
				 */
				pETagPointer = strstr(pSPointer, pETagString);

				if(NULL != pETagPointer) {
					/*
					 * write the new string value
					 */
					pSValuePointer = pSPointer+strlen(pSTagString);
					if(strlen(pNewValueString) == (unsigned int)(pETagPointer-pSValuePointer)) {
						lseek( fd, (int)(pSValuePointer-pReadBuff), SEEK_SET );
						write( fd, pNewValueString, strlen(pNewValueString));
					} else {
						/*
						 * insert with new length value string
						 */
						wfc_util_fupdate_string(fd, &pReadBuff, pETagPointer, pSValuePointer, pNewValueString);
					}
				} else {
					wfc_util_log_error("wfc_util_fset_string : can not find End TAG");
				}
			} else {
				/*
				 * "\n""[Start TAG][String Value][End TAG]""\n"
				 */
				sz_NewValueBuff = strlen(pSTagString) +
				                  strlen(pNewValueString) +
				                  strlen(pETagString) +
				                  2 + 1;
				pNewValueBuff = malloc( sz_NewValueBuff);

				if( NULL != pNewValueBuff ) {
					/*
					 * prefare the new string to insert
					 */
					memset( pNewValueBuff, 0, sz_NewValueBuff );
					sprintf( pNewValueBuff, "%c%s%s%s%c", '\n', pSTagString, pNewValueString, pETagString,'\n' );

					/*
					 * insert new string to the file
					 */
					wfc_util_finsert_new_string(fd, &pReadBuff, pNewValueBuff, pEndOfCfg);

					free( pNewValueBuff );
				} else {
					wfc_util_log_error("wfc_util_fset_string : can not malloc(%d)", (int)sz_file);
				}
			}

			free(pReadBuff);
		} else {
			wfc_util_log_error("wfc_util_fset_string : can not malloc(%d)", (int)sz_file);
		}

		if ( -1 == fsync( fd ) ) {
			wfc_util_log_error("wfc_util_fset_string : fail to fsync()");
		}

		close( fd );
	} else {
		wfc_util_log_error("wfc_util_fset_string : can not open file");
	}

	return;
}

/*
 * wfc_util_fget_string
 *
 * Read value from the following format string in the file pFileName.
 * [pSTagString][string value to read][pETagString]
 *
 * pFileName        : file name and path
 * pEndOfCfg        : tag string to notify the end of configuration file
 * pSTagString      : tag string to notify purpose of the value
 * pETagString      : tag string to notify the end of the value
 * pValueStringBuff : string buffer to get string value
 * stringBuffLength : the length of pValueStringBuff
 *
 * return : it will return the length of the stored string value if procedure is success
 *          or will return 0 if not.
 */
int wfc_util_fget_string(char *pFileName, char *pEndOfCfg __attribute__((unused)), char *pSTagString,
			 char *pETagString, char *pValueStringBuff, int stringBuffLength)
{
	int result = 0;
	int fd;
	off_t sz_file;
	char *pReadBuff = NULL;
	char *pSPointer = NULL, *pETagPointer = NULL, *pSValuePointer = NULL;

	fd = open( pFileName, O_RDONLY );

	if( fd >= 0 ) {
		/*
		 * calcurate file size
		 */
		sz_file  = lseek( fd,  0, SEEK_END );

		/*
		 * prefare the buffer to read file
		 */
		if (sz_file > 0)		// skip when value is 0
			pReadBuff = malloc(sz_file + 1);

		if( NULL != pReadBuff ) {
			/*
			 * read file
			 */
			memset( pReadBuff, 0, sz_file + 1);
			if(lseek(fd, 0, SEEK_SET) != 0) {
				wfc_util_log_error("lseek failure");
			}
			read( fd, pReadBuff, sz_file );

			/* WBT fix, make sure it is terminated with \0 */
			pReadBuff[sz_file] = '\0';

			/*
			 * find TAG string
			 */
			pSPointer = strstr( pReadBuff, pSTagString );

			if( NULL != pSPointer ) {
				/*
				 * find END OF LINE string
				 */
				pETagPointer = strstr(pSPointer, pETagString);

				if( NULL != pETagPointer ) {
					/*
					 * read the string value
					 */
					pSValuePointer = pSPointer+strlen(pSTagString);
					if( stringBuffLength >= (int)(pETagPointer-pSValuePointer) ) {
						memset( pValueStringBuff, 0, stringBuffLength );
						memcpy( pValueStringBuff, pSValuePointer, (int)(pETagPointer-pSValuePointer) );
						result = (int)(pETagPointer-pSValuePointer);
					} else {
						wfc_util_log_error("wfc_util_fget_string : not enough string value buffer(%d)", (int)(pETagPointer-pSValuePointer));
					}
				} else {
					wfc_util_log_error("wfc_util_fget_string : can not find End TAG");
				}
			} else {
				wfc_util_log_error("wfc_util_fget_string : can not find Start TAG");
			}
			free(pReadBuff);
		} else {
			wfc_util_log_error("wfc_util_fget_string : can not malloc(%d)", (int)sz_file);
		}
		close( fd );
	} else {
		wfc_util_log_error("wfc_util_fget_string : can not open file");
	}

	return result;
}

/*
 * wfc_util_ffile_check
 *
 * check whether pDestFName file exist or not
 *
 * pFileName   : file name and path
 * access_mode : R_OK | W_OK | X_OK | F_OK
 *
 * return : it will return 0 if the file exist
 *          or will return -1 if not.
 */
int wfc_util_ffile_check(char *pDestFName, int access_mode)
{
	struct stat st;

	if (access(pDestFName, access_mode) == 0) {
		if( stat( pDestFName, &st ) < 0 ) {
			wfc_util_log_error("Cannot stat the file \"%s\": %s", pDestFName, strerror(errno));
			return -1;
		}
		//check if config file has some data or is it empty due to previous errors
		if( st.st_size ) {
			return 0;
		}
	} else {
		wfc_util_log_error("Cannot access \"%s\": %s", pDestFName, strerror(errno));
	}

	return -1;
}

/*
 * wfc_util_ffile_check_copy
 *
 * check whether pDestFName file exist if not it will copy from pSourceFName file
 *
 * return : it will return 0 if procedure is success
 *          or will return -1 if not.
 */
int wfc_util_ffile_check_copy(char *pDestFName, char *pSourceFName, mode_t mode, uid_t uID, gid_t gID)
{
#define WFC_BUFFER_SIZE 2048
	char buf[WFC_BUFFER_SIZE] = {0}; // Null terminated
	int srcfd, destfd;
	int nread;
	struct stat st;

	if (access(pDestFName, R_OK|W_OK) == 0) {
		if( stat( pDestFName, &st ) < 0 ) {
			wfc_util_log_error("Cannot stat the file \"%s\": %s", pDestFName, strerror(errno));
			return -1;
		}
		//check if config file has some data or is it empty due to previous errors
		if( st.st_size ) {
			return 0;
		}
	//else continue to write the config from default template.
	} else if (errno != ENOENT) {
		wfc_util_log_error("Cannot access \"%s\": %s", pDestFName, strerror(errno));
		return -1;
	}

	srcfd = open(pSourceFName, O_RDONLY);
	if (srcfd < 0) {
		wfc_util_log_error("Cannot open \"%s\": %s", pSourceFName, strerror(errno));
		return -1;
	}

	destfd = open(pDestFName, O_CREAT|O_WRONLY, mode);
	if (destfd < 0) {
		close(srcfd);
		wfc_util_log_error("Cannot create \"%s\": %s", pDestFName, strerror(errno));
		return -1;
	}

	while ((nread = read(srcfd, buf, WFC_BUFFER_SIZE-1)) != 0) {
		if (nread < 0) {
			wfc_util_log_error("Error reading \"%s\": %s", pSourceFName, strerror(errno));
			close(srcfd);
			close(destfd);
			unlink(pDestFName);
			return -1;
		}
		// WBT fix, according to manual, the number of bytes read can't be bigger than read_size. I don't know why WBT complains for this.
		if (nread < WFC_BUFFER_SIZE)
			buf[nread] = '\0';
		else {
			buf[WFC_BUFFER_SIZE-1] = '\0';
			nread = WFC_BUFFER_SIZE-1;
		}
		write(destfd, buf, nread);
	}

	close(destfd);
	close(srcfd);

	/* remove this code because of permission problem when it is accessed from "atd" having system permission. */
	{
	#ifndef CONFIG_LGE_WLAN_WIFI_PATCH
	uid_t uid = getuid();
	gid_t gid = getgid();
	wfc_util_log_error("Error changing group ownership (%d) of %s to %d: %s", gid, pDestFName, gID, strerror(errno));
	if (0 == uid) {
	#endif /* CONFIG_LGE_WLAN_WIFI_PATCH */
		if (chown(pDestFName, uID, gID) < 0) {
			wfc_util_log_error("Error changing group ownership of %s to %d: %s", pDestFName, gID, strerror(errno));
			unlink(pDestFName);
			return -1;
		}
	#ifndef CONFIG_LGE_WLAN_WIFI_PATCH
	} else {
		wfc_util_log_error("wfc_util_ffile_check_copy : we can not excute chown[uid = %d, gid = %d]", uid, getgid());
	}
	#endif /* CONFIG_LGE_WLAN_WIFI_PATCH */
	}

	return 0;
}

