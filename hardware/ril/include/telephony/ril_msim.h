/*
 * Copyright (C) 2014 The Android Open Source Project
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


#ifndef ANDROID_RIL_MSIM_H
#define ANDROID_RIL_MSIM_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  RIL_UICC_SUBSCRIPTION_DEACTIVATE = 0,
  RIL_UICC_SUBSCRIPTION_ACTIVATE = 1
} RIL_UiccSubActStatus;

typedef enum {
  RIL_SUBSCRIPTION_1 = 0,
  RIL_SUBSCRIPTION_2 = 1,
  RIL_SUBSCRIPTION_3 = 2
} RIL_SubscriptionType;

typedef struct {
  int   slot;                        /* 0, 1, ... etc. */
  int   app_index;                   /* array subscriptor from applications[RIL_CARD_MAX_APPS] in
                                        RIL_REQUEST_GET_SIM_STATUS */
  RIL_SubscriptionType  sub_type;    /* Indicates subscription 1 or subscription 2 */
  RIL_UiccSubActStatus  act_status;
} RIL_SelectUiccSub;

#ifdef __cplusplus
}
#endif

#endif /*ANDROID_RIL_MSIM_H*/
