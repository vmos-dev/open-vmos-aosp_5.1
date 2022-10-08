#include <dirent.h>

/* macros */
#define MAX_ROLES 20
#define MAX_TABLE_SIZE 30
#define MAX_CONCURRENT_INSTANCES 1
    /* limit the number of max occuring instances of same component,
       tune this if you like
    */

/* struct definitions */
typedef struct _ComponentTable {
    OMX_STRING name;
    OMX_U16 nRoles;
    OMX_STRING pRoleArray[MAX_ROLES];
    OMX_HANDLETYPE* pHandle[MAX_CONCURRENT_INSTANCES];
    int refCount;
}ComponentTable;

/* function prototypes */
OMX_ERRORTYPE TIOMX_BuildComponentTable();

