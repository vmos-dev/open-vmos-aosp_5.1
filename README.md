# VMOS Pro Android 5.1 ROM Introduction
[中文README](./vmosdocs/README_CN.md)
#### I. Project Introduction
* This project is a virtual machine source code for use with VMOS Pro. It is based on the Android source code aosp-5.1.1_r38 version and supports devices higher than Android 5.1.
* VMOS Pro application download address: https://www.vmos.com

#### 2. Introduction
VMOS Pro Android 5.1 ROM can be understood as a virtual machine running in the Android system through VMOS Pro. By compiling its open source code, a custom ROM can be implemented, and a "personalized virtual machine" created by you can be run in VMOS Pro.
At present, this technology is widely used in application multi-opening, data isolation, mobile office security, mobile phone simulation information and other fields.

# VMOS Pro Android 5.1 ROM Technical Architecture
![Gopher image](vmosdocs/images/architecture.png)
The company's core technology, the VMOS Pro virtual operating system, is an Android virtual system that runs on the Android system with ordinary APP permissions. The upper left part of the figure is the real Android system, including the application layer, the Framework layer, the HAL abstraction layer and the Android operating kernel.
By running the VMOS Pro virtual operating system in the application layer of the real Android system, it will create and start an Android virtual operating system modified based on AOSP. The specific generated virtual operating system is shown in the upper right part of the figure.
The VMOS Pro virtual operating system in the upper right part includes an independent application layer, a Framework layer, and a HAL abstraction layer. The applications running in the virtual system are isolated from the applications running in the real Android system. The application layer of the VMOS Pro virtual operating system, the Framework layer , the HAL abstraction layer can be independently controlled and modified by the developers and users of Xiaosu Technology. The VMOS Pro virtual operating system virtualizes the HAL abstraction layer and can use the hardware device data of the real Android system, or fill in the virtual data.


# VMOS Pro Android 5.1 ROM build and deployment
#### Construct
* Prepare a linux operating system computer or virtual machine to store the code, the source code size is about 50Gb, and the overall size of the compiled and output project is about 100Gb, please prepare the required disk space in advance;
* Pull code: ```git clone --recurse-submodules https://github.com/VMOS-XiaoSuan/open-vmos-aosp_5.1.git```
* Compilation environment: It is recommended to use Docker to compile to avoid meaningless compilation environment debugging. [Use Docker to compile Android source code](./Compile%20AOSP%20with%20Docker.md)
* Execute the compile script ```make_arm64_user.sh```(64-bit) or ```make_arm_user.sh```(32-bit) in the source root directory

#### deploy
1. Maintain the system version number: manually change the version number in ```rom_version_arm64``` (64-bit) or ```rom_version_arm``` (32-bit) in the source root directory. (Just for version management, you can do it without modification)
2. Execute the package script ```pack_arm64_user.sh``` (64-bit) or ```pack_arm_user.sh``` (32-bit) in the source root directory
3. After the script is executed successfully, the system image package will be output in ````$ANDROID_BUILD_TOP/vmos_51_$TARGET_PRODUCT-$TARGET_BUILD_VARIANT.zip```
4. The packaged zip package can be imported into VMOS Pro (version 2.9.3 and above) for use

# VMOS Pro Android 5.1 ROM usage scenarios
#### 1. Implement multiple applications
By adding multiple Android 5.1 ROMs to VMOS Pro, you can install multiple apps or games such as QQ on one phone at the same time, and log in to multiple accounts.

#### 2. Meet the needs of mobile security
VMOS Pro Android 5.1 ROM provides a complete set of internal and external isolation mechanisms, which means that its interior is a "completely independent space from the system". In this way, on the mobile phone, which is inseparable from daily life and work, it can not only meet the security requirements of data isolation, but also meet the isolation of personal life and work scenarios.
Through the published open source code, mobile security-related requirements such as application behavior auditing, data encryption, data collection, data leakage prevention, and attack leakage prevention can be achieved with only a little customization.

#### 3. Meet the requirement of free ROOT HOOK and achieve complete control over the internal App
VMOS Pro Android 5.1 ROM provides Hook capabilities of Java and Native, in which various scene functions such as virtual positioning, App monitoring and management, and mobile security can be realized.

#### 4. Support Google services
Implemented support for Google services to support the operation of overseas apps such as Twitter, Instagram, FaceBook, and Youtube.

# VMOS Pro Android 5.1 ROM Compatible Stability
This virtual machine source code is used in conjunction with VMOS Pro, and has been widely used in a large number of common models at home and abroad.
As of now, the Android systems supported by VMOS Pro Android 5.1 ROM:
Devices higher than Android 5.1.
  
Supported APP types:
Including 32-bit and 64-bit.
  
Supported HOOK types:
Includes Java Hooks and Native Hooks.
  
Supported CPU types:
Includes ARM 32 and ARM 64.

# Authorization description
VMOS Pro Android 5.1 ROM technology belongs to: Hunan Xiaosuan Technology Information Co., Ltd., and has applied for a number of VMOS Pro Android 5.1 ROM intellectual property rights, which are protected by the Intellectual Property Law of the People's Republic of China.
Individuals can use the code of VMOS Pro Android 5.1 ROM on Github, download the source code for reference learning, modification and customization, and import it into VMOS Pro after compiling and packaging.
However, it cannot be used for commercial use. If you need it for commercial use, you can contact to buy it. VMOS Pro Android 5.1 ROM has been launched more than 6 million times in VMOS Pro. After purchasing a commercial license, you will enjoy the technical achievements of continuous iterative updates.

# Hereby declare
If you use the VMOS Pro Android 5.1 ROM for internal use, commercial profit or uploading to the app market without authorization, we will collect evidence and report to the police (copyright infringement) or prosecute.
Anyone who reports unauthorized or illegal use of VMOS Pro Android 5.1 ROM code to develop products will be rewarded upon verification. We will keep the identity of the whistleblower confidential!