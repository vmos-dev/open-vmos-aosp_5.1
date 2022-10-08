# Introduction
The source code is based on aosp-5.1.1_r38 version.  
The source code size is about 20Gb, and the overall size of the compiled and output project is about 50Gb. Please prepare the required disk space in advance.  

# Building
1. Clone sources: ```git clone --recurse-submodules https://github.com/VMOS-XiaoSuan/open-vmos-aosp_5.1.git```
2. Compilation environment: It is recommended to use Docker to compile to avoid meaningless compilation environment debugging.  [Use Docker to compile Android source code](./Compile%20AOSP%20with%20Docker.md)  
3. Execute the compile script ```make_arm64_user.sh```(64-bit) or ```make_arm_user.sh```(32-bit) in the source root directory.  

# Development
1. Maintain system version number: manually change the version number in ```$ANDROID_BUILD_TOP/RomVersion```. (Just for version management, you can do it without modification)  
2. Execute the package script ```pack.sh``` in the source root directory.  
3. After the script is executed successfully, the system image package will be output in ````$ANDROID_BUILD_TOP/vmos_51_$TARGET_PRODUCT-$TARGET_BUILD_VARIANT.zip```.  