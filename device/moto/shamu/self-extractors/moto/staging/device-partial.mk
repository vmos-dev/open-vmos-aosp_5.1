# Copyright 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Motorola blob(s) necessary for Shamu hardware
PRODUCT_COPY_FILES := \
    vendor/moto/shamu/proprietary/adspd:system/bin/adspd:moto \
    vendor/moto/shamu/proprietary/ramdump:system/bin/ramdump:moto \
    vendor/moto/shamu/proprietary/tcmd_mini:system/bin/tcmd_mini:moto \
    vendor/moto/shamu/proprietary/Bluetooth_cal.acdb:system/etc/Bluetooth_cal.acdb:moto \
    vendor/moto/shamu/proprietary/FIT_V15_audio.cfg:system/etc/diag/mdm/FIT_V15_audio.cfg:moto \
    vendor/moto/shamu/proprietary/FIT_V15.cfg:system/etc/diag/mdm/FIT_V15.cfg:moto \
    vendor/moto/shamu/proprietary/sensors_qxdm.cfg:system/etc/diag/mdm/sensors_qxdm.cfg:moto \
    vendor/moto/shamu/proprietary/sensors_qxdm.cfg:system/etc/diag/sensors_qxdm.cfg:moto \
    vendor/moto/shamu/proprietary/vpu.b00:system/etc/firmware/vpu.b00:moto \
    vendor/moto/shamu/proprietary/vpu.b01:system/etc/firmware/vpu.b01:moto \
    vendor/moto/shamu/proprietary/vpu.b02:system/etc/firmware/vpu.b02:moto \
    vendor/moto/shamu/proprietary/vpu.b03:system/etc/firmware/vpu.b03:moto \
    vendor/moto/shamu/proprietary/vpu.b04:system/etc/firmware/vpu.b04:moto \
    vendor/moto/shamu/proprietary/vpu.b05:system/etc/firmware/vpu.b05:moto \
    vendor/moto/shamu/proprietary/vpu.b06:system/etc/firmware/vpu.b06:moto \
    vendor/moto/shamu/proprietary/vpu.b07:system/etc/firmware/vpu.b07:moto \
    vendor/moto/shamu/proprietary/vpu.b08:system/etc/firmware/vpu.b08:moto \
    vendor/moto/shamu/proprietary/vpu.b09:system/etc/firmware/vpu.b09:moto \
    vendor/moto/shamu/proprietary/vpu.b10:system/etc/firmware/vpu.b10:moto \
    vendor/moto/shamu/proprietary/vpu.b11:system/etc/firmware/vpu.b11:moto \
    vendor/moto/shamu/proprietary/vpu.b12:system/etc/firmware/vpu.b12:moto \
    vendor/moto/shamu/proprietary/vpu.mbn:system/etc/firmware/vpu.mbn:moto \
    vendor/moto/shamu/proprietary/vpu.mdt:system/etc/firmware/vpu.mdt:moto \
    vendor/moto/shamu/proprietary/General_cal.acdb:system/etc/General_cal.acdb:moto \
    vendor/moto/shamu/proprietary/Global_cal.acdb:system/etc/Global_cal.acdb:moto \
    vendor/moto/shamu/proprietary/Handset_cal.acdb:system/etc/Handset_cal.acdb:moto \
    vendor/moto/shamu/proprietary/Hdmi_cal.acdb:system/etc/Hdmi_cal.acdb:moto \
    vendor/moto/shamu/proprietary/Headset_cal.acdb:system/etc/Headset_cal.acdb:moto \
    vendor/moto/shamu/proprietary/org.simalliance.openmobileapi.xml:system/etc/permissions/org.simalliance.openmobileapi.xml:moto \
    vendor/moto/shamu/proprietary/Speaker_cal.acdb:system/etc/Speaker_cal.acdb:moto \
    vendor/moto/shamu/proprietary/org.simalliance.openmobileapi.jar:system/framework/org.simalliance.openmobileapi.jar:moto \
    vendor/moto/shamu/proprietary/libadspd-jni.so:system/lib/libadspd-jni.so:moto \
    vendor/moto/shamu/proprietary/libadspd.so:system/lib/libadspd.so:moto \
    vendor/moto/shamu/proprietary/librecoglib.so:system/lib/librecoglib.so:moto \
    vendor/moto/shamu/proprietary/libsupermodel.so:system/lib/libsupermodel.so:moto \
    vendor/moto/shamu/proprietary/acdb.mbn:system/vendor/firmware/acdb.mbn:moto \
    vendor/moto/shamu/proprietary/atmel-a432-14061601-0102aa-shamu-p1.tdat:system/vendor/firmware/atmel-a432-14061601-0102aa-shamu-p1.tdat:moto \
    vendor/moto/shamu/proprietary/atmel-a432-14103001-0103aa-shamu.tdat:system/vendor/firmware/atmel-a432-14103001-0103aa-shamu.tdat:moto \
    vendor/moto/shamu/proprietary/left.boost.music.eq:system/vendor/firmware/left.boost.music.eq:moto \
    vendor/moto/shamu/proprietary/left.boost_music_table.preset:system/vendor/firmware/left.boost_music_table.preset:moto \
    vendor/moto/shamu/proprietary/left.boost_n1b12.patch:system/vendor/firmware/left.boost_n1b12.patch:moto \
    vendor/moto/shamu/proprietary/left.boost_n1c2.patch:system/vendor/firmware/left.boost_n1c2.patch:moto \
    vendor/moto/shamu/proprietary/left.boost.ringtone.eq:system/vendor/firmware/left.boost.ringtone.eq:moto \
    vendor/moto/shamu/proprietary/left.boost_ringtone_table.preset:system/vendor/firmware/left.boost_ringtone_table.preset:moto \
    vendor/moto/shamu/proprietary/left.boost.speaker:system/vendor/firmware/left.boost.speaker:moto \
    vendor/moto/shamu/proprietary/left.boost.voice.eq:system/vendor/firmware/left.boost.voice.eq:moto \
    vendor/moto/shamu/proprietary/left.boost_voice_table.preset:system/vendor/firmware/left.boost_voice_table.preset:moto \
    vendor/moto/shamu/proprietary/right.boost.music.eq:system/vendor/firmware/right.boost.music.eq:moto \
    vendor/moto/shamu/proprietary/right.boost_music_table.preset:system/vendor/firmware/right.boost_music_table.preset:moto \
    vendor/moto/shamu/proprietary/right.boost_n1b12.patch:system/vendor/firmware/right.boost_n1b12.patch:moto \
    vendor/moto/shamu/proprietary/right.boost_n1c2.patch:system/vendor/firmware/right.boost_n1c2.patch:moto \
    vendor/moto/shamu/proprietary/right.boost.ringtone.eq:system/vendor/firmware/right.boost.ringtone.eq:moto \
    vendor/moto/shamu/proprietary/right.boost_ringtone_table.preset:system/vendor/firmware/right.boost_ringtone_table.preset:moto \
    vendor/moto/shamu/proprietary/right.boost.speaker:system/vendor/firmware/right.boost.speaker:moto \
    vendor/moto/shamu/proprietary/right.boost.voice.eq:system/vendor/firmware/right.boost.voice.eq:moto \
    vendor/moto/shamu/proprietary/right.boost_voice_table.preset:system/vendor/firmware/right.boost_voice_table.preset:moto \
    vendor/moto/shamu/proprietary/VRGain.bin:system/vendor/firmware/VRGain.bin:moto \
    vendor/moto/shamu/proprietary/libmdmcutback.so:system/vendor/lib/libmdmcutback.so:moto \
    vendor/moto/shamu/proprietary/libmotext_inf.so:system/vendor/lib/libmotext_inf.so:moto \
    vendor/moto/shamu/proprietary/libqmimotext.so:system/vendor/lib/libqmimotext.so:moto \
    vendor/moto/shamu/proprietary/lts_en_us_9_5_2b.raw:system/vendor/moto/audiomonitor/sensory/lts_en_us_9_5_2b.raw:moto \
    vendor/moto/shamu/proprietary/nn_de_mfcc_16k_15_big_250_v3_4.raw:system/vendor/moto/audiomonitor/sensory/nn_de_mfcc_16k_15_big_250_v3_4.raw:moto \
    vendor/moto/shamu/proprietary/nn_en_uk_mfcc_16k_15_big_250_v2_2.raw:system/vendor/moto/audiomonitor/sensory/nn_en_uk_mfcc_16k_15_big_250_v2_2.raw:moto \
    vendor/moto/shamu/proprietary/nn_en_us_mfcc_16k_15_big_250_v5_1_1.raw:system/vendor/moto/audiomonitor/sensory/nn_en_us_mfcc_16k_15_big_250_v5_1_1.raw:moto \
    vendor/moto/shamu/proprietary/nn_es_es_mfcc_16k_15_big_250_v1_5.raw:system/vendor/moto/audiomonitor/sensory/nn_es_es_mfcc_16k_15_big_250_v1_5.raw:moto \
    vendor/moto/shamu/proprietary/nn_es_us_mfcc_16k_15_big_250_v1_5.raw:system/vendor/moto/audiomonitor/sensory/nn_es_us_mfcc_16k_15_big_250_v1_5.raw:moto \
    vendor/moto/shamu/proprietary/nn_fr_ca_mfcc_16k_15_big_250_v2_5.raw:system/vendor/moto/audiomonitor/sensory/nn_fr_ca_mfcc_16k_15_big_250_v2_5.raw:moto \
    vendor/moto/shamu/proprietary/nn_fr_fr_mfcc_16k_15_big_250_v2_4.raw:system/vendor/moto/audiomonitor/sensory/nn_fr_fr_mfcc_16k_15_big_250_v2_4.raw:moto \
    vendor/moto/shamu/proprietary/nn_it_mfcc_16k_15_big_250_v3_5.raw:system/vendor/moto/audiomonitor/sensory/nn_it_mfcc_16k_15_big_250_v3_5.raw:moto \
    vendor/moto/shamu/proprietary/nn_pt_mfcc_16k_15_big_250_v1_1.raw:system/vendor/moto/audiomonitor/sensory/nn_pt_mfcc_16k_15_big_250_v1_1.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_dede_1_1.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_dede_1_1.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_enuk_1_1.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_enuk_1_1.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_enus_2_0.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_enus_2_0.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_eses_1_2.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_eses_1_2.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_esus_1_2.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_esus_1_2.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_frca_prec1_1_0.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_frca_prec1_1_0.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_frfr_1_1.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_frfr_1_1.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_it_it_1_0.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_it_it_1_0.raw:moto \
    vendor/moto/shamu/proprietary/phonemesearch_ptbr_1_0.raw:system/vendor/moto/audiomonitor/sensory/phonemesearch_ptbr_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_dede_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_dede_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_enuk_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_enuk_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_enus_2_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_enus_2_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_eses_1_2.raw:system/vendor/moto/audiomonitor/sensory/svsid_eses_1_2.raw:moto \
    vendor/moto/shamu/proprietary/svsid_esus_1_2.raw:system/vendor/moto/audiomonitor/sensory/svsid_esus_1_2.raw:moto \
    vendor/moto/shamu/proprietary/svsid_frca_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_frca_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_frfr_1_1.raw:system/vendor/moto/audiomonitor/sensory/svsid_frfr_1_1.raw:moto \
    vendor/moto/shamu/proprietary/svsid_it_it_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_it_it_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_ptbr_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_ptbr_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_triggerogn1_dede_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_triggerogn1_dede_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_triggerogn1_enuk_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_triggerogn1_enuk_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_triggerogn1_eses_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_triggerogn1_eses_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_triggerogn1_itit_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_triggerogn1_itit_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_triggerogn_enus_3_1.raw:system/vendor/moto/audiomonitor/sensory/svsid_triggerogn_enus_3_1.raw:moto \
    vendor/moto/shamu/proprietary/svsid_triggerogn_frfr_1_0.raw:system/vendor/moto/audiomonitor/sensory/svsid_triggerogn_frfr_1_0.raw:moto \
    vendor/moto/shamu/proprietary/svsid_triggerogn_ptbr_2_1.raw:system/vendor/moto/audiomonitor/sensory/svsid_triggerogn_ptbr_2_1.raw:moto \

