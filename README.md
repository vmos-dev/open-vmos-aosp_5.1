# 介绍
* 此项目是用于配合 VMOS Pro 使用的虚拟机源码，基于 Android 源码 aosp-5.1.1_r38 版本打造，支持高于 Android 5.1的设备使用。
* VMOS Pro 应用下载地址：https://www.vmos.com
* 源码大小20Gb左右，编译输出后的项目整体大小在50Gb左右，请提前准备好所需的磁盘空间

# 构建
* 拉取代码：```git clone --recurse-submodules https://github.com/VMOS-XiaoSuan/open-vmos-aosp_5.1.git```
* 编译环境：建议使用Docker编译，避免无意义的编译环境调试。[使用Docker编译Android源代码](./Compile%20AOSP%20with%20Docker.md)  
* 在源码根目录下执行编译脚本```make_arm64_user.sh```(64位) 或者 ```make_arm_user.sh```(32位)  

# 部署
1. 维护系统版本号：手动更改源码根目录下```RomVersion```中的版本号。（只是为了做版本管理，不修改也可以）  
2. 在源码根目录下执行打包脚本```pack.sh```  
3. 脚本执行成功后，系统镜像包会输出在```$ANDROID_BUILD_TOP/vmos_51_$TARGET_PRODUCT-$TARGET_BUILD_VARIANT.zip```中

# 使用
