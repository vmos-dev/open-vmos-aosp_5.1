/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "lights"
#define LOGE ALOGE

#include <cutils/log.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>
#include <hardware/hardware.h>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int write_int(char const *path, int value)
{
	int fd;
	static int already_warned = -1;
	fd = open(path, O_RDWR);
	if (fd >= 0) {
		char buffer[20];
		int bytes = sprintf(buffer, "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == -1) {
			LOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
}

static int rgb_to_brightness(struct light_state_t const *state)
{
	int color = state->color & 0x00ffffff;
	return ((77 * ((color >> 16) & 0x00ff))
		+ (150 * ((color >> 8) & 0x00ff)) +
		(29 * (color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t *dev,
			       struct light_state_t const *state)
{
	int err = 0;
	int brightness = rgb_to_brightness(state);
	//+++ power save
	int range_num = 1;
	int old_brightness_range_indexs[] = {160,255};
	int new_brightness_range_indexs[] = {160,182};
	int old_brightness = brightness;
	int i=0;
	int old_brightness_in_range = -1;// 0:1~10 , 1:10~102 , 2:102~255
	for(i=0;i<range_num;i++){
		int t_range_min = old_brightness_range_indexs[i];
		int t_range_max = old_brightness_range_indexs[i+1];
		if( brightness>=t_range_min && brightness<=t_range_max){
			old_brightness_in_range = i;
			break;
		}
	}
	if(old_brightness_in_range>=0){
		int old_br_min = old_brightness_range_indexs[old_brightness_in_range];
		int olb_br_max = old_brightness_range_indexs[old_brightness_in_range+1];
		int new_br_min = new_brightness_range_indexs[old_brightness_in_range];
		int new_br_max = new_brightness_range_indexs[old_brightness_in_range+1];
		int new_brightness = new_br_min +
				(new_br_max-new_br_min)*(old_brightness-old_br_min)/(olb_br_max-old_br_min);
		brightness = new_brightness;
	}
	//---
	pthread_mutex_lock(&g_lock);
	err = write_int("/sys/class/backlight/pwm-backlight/brightness",
			brightness);
	pthread_mutex_unlock(&g_lock);

	return err;
}

/** Close the lights device */
static int close_lights(struct light_device_t *dev)
{
	if (dev)
		free(dev);
	return 0;
}

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t *module, char const *name,
		       struct hw_device_t **device)
{
	pthread_t lighting_poll_thread;

	int (*set_light) (struct light_device_t *dev,
			  struct light_state_t const *state);

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
		set_light = set_light_backlight;
	else
		return -EINVAL;

	pthread_mutex_init(&g_lock, NULL);

	struct light_device_t *dev = malloc(sizeof(struct light_device_t));
	memset(dev, 0, sizeof(*dev));

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t *)module;
	dev->common.close = (int (*)(struct hw_device_t *))close_lights;
	dev->set_light = set_light;

	*device = (struct hw_device_t *)dev;

	return 0;
}

static struct hw_module_methods_t lights_methods =
{
	.open =  open_lights,
};

/*
 * The backlight Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM =
{
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "NVIDIA Kai lights module",
	.author = "NVIDIA",
	.methods = &lights_methods,
};
