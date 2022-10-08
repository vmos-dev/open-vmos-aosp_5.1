#include <string.h>
#include <stdio.h>
#include "ml_sysfs_helper.h"
#include <dirent.h>
#include <ctype.h>
#define MPU_SYSFS_ABS_PATH "/sys/class/invensense/mpu"

enum PROC_SYSFS_CMD {
	CMD_GET_SYSFS_PATH,
	CMD_GET_DMP_PATH,
	CMD_GET_CHIP_NAME,
	CMD_GET_SYSFS_KEY,
	CMD_GET_TRIGGER_PATH,
	CMD_GET_DEVICE_NODE
};
static char sysfs_path[100];
static char *chip_name[] = {
    "ITG3500", 
    "MPU6050", 
    "MPU9150", 
    "MPU3050", 
    "MPU6500"
};
static int chip_ind;
static int initialized =0;
static int status = 0;
static int iio_initialized = 0;
static int iio_dev_num = 0;

#define IIO_MAX_NAME_LENGTH 30

#define FORMAT_SCAN_ELEMENTS_DIR "%s/scan_elements"
#define FORMAT_TYPE_FILE "%s_type"

#define CHIP_NUM ARRAY_SIZE(chip_name)

static const char *iio_dir = "/sys/bus/iio/devices/";

/**
 * find_type_by_name() - function to match top level types by name
 * @name: top level type instance name
 * @type: the type of top level instance being sort
 *
 * Typical types this is used for are device and trigger.
 **/
int find_type_by_name(const char *name, const char *type)
{
	const struct dirent *ent;
	int number, numstrlen;

	FILE *nameFile;
	DIR *dp;
	char thisname[IIO_MAX_NAME_LENGTH];
	char *filename;

	dp = opendir(iio_dir);
	if (dp == NULL) {
		printf("No industrialio devices available");
		return -ENODEV;
	}

	while (ent = readdir(dp), ent != NULL) {
		if (strcmp(ent->d_name, ".") != 0 &&
			strcmp(ent->d_name, "..") != 0 &&
			strlen(ent->d_name) > strlen(type) &&
			strncmp(ent->d_name, type, strlen(type)) == 0) {
			numstrlen = sscanf(ent->d_name + strlen(type),
					   "%d",
					   &number);
			/* verify the next character is not a colon */
			if (strncmp(ent->d_name + strlen(type) + numstrlen,
					":",
					1) != 0) {
				filename = malloc(strlen(iio_dir)
						+ strlen(type)
						+ numstrlen
						+ 6);
				if (filename == NULL)
					return -ENOMEM;
				sprintf(filename, "%s%s%d/name",
					iio_dir,
					type,
					number);
				nameFile = fopen(filename, "r");
				if (!nameFile)
					continue;
				free(filename);
				fscanf(nameFile, "%s", thisname);
				if (strcmp(name, thisname) == 0)
					return number;
				fclose(nameFile);
			}
		}
	}
	return -ENODEV;
}

/* mode 0: search for which chip in the system and fill sysfs path
   mode 1: return event number
 */
static int parsing_proc_input(int mode, char *name){
	const char input[] = "/proc/bus/input/devices";
	char line[4096], d;
	char tmp[100];
	FILE *fp;
	int i, j, result, find_flag;
	int event_number = -1;
	int input_number = -1;

	if(NULL == (fp = fopen(input, "rt")) ){
		return -1;
	}
	result = 1;
	find_flag = 0;
	while(result != 0 && find_flag < 2){
		i = 0;
		d = 0;
		memset(line, 0, 100);
		while(d != '\n'){		
			result = fread(&d, 1, 1, fp);
			if(result == 0){
				line[0] = 0;
				break;
			}
			sprintf(&line[i], "%c", d);		
			i ++;
		}
		if(line[0] == 'N'){
			i = 1;
			while(line[i] != '"'){
				i++;
			}
			i++;
			j = 0;
			find_flag = 0;
			if (mode == 0){
				while(j < CHIP_NUM){
					if(!memcmp(&line[i], chip_name[j], strlen(chip_name[j]))){
						find_flag = 1;
						chip_ind = j;
					}
					j++;
				}
			} else if (mode  != 0){
				if(!memcmp(&line[i], name, strlen(name))){
					find_flag = 1;
				}
			}
		}		
		if(find_flag){
			if(mode == 0){
				if(line[0] == 'S'){
					memset(tmp, 0, 100);
					i =1;
					while(line[i] != '=') i++;
					i++;
					j = 0;
					while(line[i] != '\n'){
						tmp[j] = line[i];
						i ++; j++;
					}	
					sprintf(sysfs_path, "%s%s", "/sys", tmp);
					find_flag++;
				}
			} else if(mode == 1){
				if(line[0] == 'H') {
					i = 2;
					while(line[i] != '=') i++;
					while(line[i] != 't') i++;	
					i++;
					event_number = 0;
					while(line[i] != '\n'){
						if(line[i] >= '0' && line[i] <= '9')
							event_number = event_number*10 + line[i]-0x30;
						i ++;
					}
					find_flag ++;
				}
			} else if (mode == 2) {
				if(line[0] == 'S'){
					memset(tmp, 0, 100);
					i =1;
					while(line[i] != '=') i++;
					i++;
					j = 0;
					while(line[i] != '\n'){
						tmp[j] = line[i];
						i ++; j++;
					}
					input_number = 0;
					if(tmp[j-2] >= '0' && tmp[j-2] <= '9') 
						input_number += (tmp[j-2]-0x30)*10;
					if(tmp[j-1] >= '0' && tmp[j-1] <= '9') 
						input_number += (tmp[j-1]-0x30);
					find_flag++;
				}
			}
		}
	}
	fclose(fp);
	if(find_flag == 0){
		return -1;
	}
	if(0 == mode)
		status = 1;
	if (mode == 1)
		return event_number;
	if (mode == 2)
		return input_number;
	return 0;

}
static void init_iio() {
	int i, j;
	char iio_chip[10];
	int dev_num;
	for(j=0; j< CHIP_NUM; j++) {
		for (i=0; i<strlen(chip_name[j]); i++) {
			iio_chip[i] = tolower(chip_name[j][i]);
		}
		iio_chip[strlen(chip_name[0])] = '\0';
		dev_num = find_type_by_name(iio_chip, "iio:device");
		if(dev_num >= 0) {
			iio_initialized = 1;
			iio_dev_num = dev_num;
			chip_ind = j;
		}
	}
}

static int process_sysfs_request(enum PROC_SYSFS_CMD cmd, char *data)
{
	char key_path[100];
	FILE *fp;
	int i, result;
	if(initialized == 0){
		parsing_proc_input(0, NULL);
		initialized = 1;
	}
	if(initialized && status == 0) {
		init_iio();
		if (iio_initialized == 0)
			return -1;
	}

	memset(key_path, 0, 100);
	switch(cmd){
	case CMD_GET_SYSFS_PATH:
		if (iio_initialized == 1)
			sprintf(data, "/sys/bus/iio/devices/iio:device%d", iio_dev_num);
		else
			sprintf(data, "%s%s", sysfs_path, "/device/invensense/mpu");
		break;
	case CMD_GET_DMP_PATH:
		if (iio_initialized == 1)
			sprintf(data, "/sys/bus/iio/devices/iio:device%d/dmp_firmware", iio_dev_num);
		else
			sprintf(data, "%s%s", sysfs_path, "/device/invensense/mpu/dmp_firmware");
		break;
	case CMD_GET_CHIP_NAME:
		sprintf(data, "%s", chip_name[chip_ind]);
		break;
	case CMD_GET_TRIGGER_PATH:
		sprintf(data, "/sys/bus/iio/devices/trigger%d", iio_dev_num);
		break;
	case CMD_GET_DEVICE_NODE:
		sprintf(data, "/dev/iio:device%d", iio_dev_num);
		break;
	case CMD_GET_SYSFS_KEY:
		memset(key_path, 0, 100);
		if (iio_initialized == 1)
			sprintf(key_path, "/sys/bus/iio/devices/iio:device%d/key", iio_dev_num);
		else	
			sprintf(key_path, "%s%s", sysfs_path, "/device/invensense/mpu/key");

		if((fp = fopen(key_path, "rt")) == NULL)
			return -1;
		for(i=0;i<16;i++){
			fscanf(fp, "%02x", &result);
			data[i] = (char)result;
		}
		
		fclose(fp);
		break;
	default:
		break;
	}
	return 0;
}
/** 
 *  @brief  return sysfs key. if the key is not available
 *          return false. So the return value must be checked
 *          to make sure the path is valid.
 *  @unsigned char *name: This should be array big enough to hold the key
 *           It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 */
inv_error_t inv_get_sysfs_key(unsigned char *key)
{
	if (process_sysfs_request(CMD_GET_SYSFS_KEY, (char*)key) < 0)
		return INV_ERROR_NOT_OPENED;
	else
		return INV_SUCCESS;
}

/** 
 *  @brief  return the sysfs path. If the path is not 
 *          found yet. return false. So the return value must be checked
 *          to make sure the path is valid.
 *  @unsigned char *name: This should be array big enough to hold the sysfs
 *           path. It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 */
inv_error_t inv_get_sysfs_path(char *name)
{
	if (process_sysfs_request(CMD_GET_SYSFS_PATH, name) < 0)
		return INV_ERROR_NOT_OPENED;
	else
		return INV_SUCCESS;
}

inv_error_t inv_get_sysfs_abs_path(char *name)
{
    strcpy(name, MPU_SYSFS_ABS_PATH);
    return INV_SUCCESS;
}

/** 
 *  @brief  return the dmp file path. If the path is not 
 *          found yet. return false. So the return value must be checked
 *          to make sure the path is valid.
 *  @unsigned char *name: This should be array big enough to hold the dmp file
 *           path. It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 */
inv_error_t inv_get_dmpfile(char *name)
{
   	if (process_sysfs_request(CMD_GET_DMP_PATH, name) < 0)
		return INV_ERROR_NOT_OPENED;
	else
		return INV_SUCCESS;
}
/** 
 *  @brief  return the chip name. If the chip is not 
 *          found yet. return false. So the return value must be checked
 *          to make sure the path is valid.
 *  @unsigned char *name: This should be array big enough to hold the chip name
 *           path(8 bytes). It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 */
inv_error_t inv_get_chip_name(char *name)
{
   	if (process_sysfs_request(CMD_GET_CHIP_NAME, name) < 0)
		return INV_ERROR_NOT_OPENED;
	else
		return INV_SUCCESS;
}
/** 
 *  @brief  return event handler number. If the handler number is not found
 *          return false. the return value must be checked
 *          to make sure the path is valid.
 *  @unsigned char *name: This should be array big enough to hold the chip name
 *           path(8 bytes). It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 *  @int *num: event number store
 */
inv_error_t  inv_get_handler_number(const char *name, int *num)
{
	initialized = 0;
	if ((*num = parsing_proc_input(1, (char *)name)) < 0)
		return INV_ERROR_NOT_OPENED;
	else
		return INV_SUCCESS;	
}

/** 
 *  @brief  return input number. If the handler number is not found
 *          return false. the return value must be checked
 *          to make sure the path is valid.
 *  @unsigned char *name: This should be array big enough to hold the chip name
 *           path(8 bytes). It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 *  @int *num: input number store
 */
inv_error_t  inv_get_input_number(const char *name, int *num)
{
	initialized = 0;
	if ((*num = parsing_proc_input(2, (char *)name)) < 0)
		return INV_ERROR_NOT_OPENED;
	else {
		return INV_SUCCESS;
	}	
}

/** 
 *  @brief  return iio trigger name. If iio is not initialized, return false.
 *          So the return must be checked to make sure the numeber is valid.
 *  @unsigned char *name: This should be array big enough to hold the trigger
 *           name. It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 */
inv_error_t inv_get_iio_trigger_path(char *name)
{
	if (process_sysfs_request(CMD_GET_TRIGGER_PATH, (char *)name) < 0)
		return INV_ERROR_NOT_OPENED;
	else
		return INV_SUCCESS;
}

/** 
 *  @brief  return iio device node. If iio is not initialized, return false.
 *          So the return must be checked to make sure the numeber is valid.
 *  @unsigned char *name: This should be array big enough to hold the device
 *           node. It should be zeroed before calling this function.
 *           Or it could have unpredicable result.
 */
inv_error_t inv_get_iio_device_node(char *name)
{
	if (process_sysfs_request(CMD_GET_DEVICE_NODE, (char *)name) < 0)
		return INV_ERROR_NOT_OPENED;
	else
		return INV_SUCCESS;
}
