### 什么是Docker？

Docker 是一个开源的应用容器引擎，让开发者可以打包他们的应用以及依赖包到一个可移植的镜像中，然后发布到任何流行的 Linux或Windows 机器上，也可以实现虚拟化。容器是完全使用沙箱机制，相互之间不会有任何接口。
### 使用Docker编译AOSP有什么优势?
编译AOSP需要不同版本的JDK和依赖库, 在Docker虚拟化中可以避免编译不同AOSP版本时JDK和依赖库的冲突, 同时不影响主机的环境.
### 安装Docker
```shell
curl -fsSL https://get.docker.com | bash -s docker --mirror Aliyun
```
### 容器镜像下载
使用国内镜像仓库，加速Docker镜像的下载
```shell
# vim /etc/docker/daemon.json
{
    "registry-mirrors": ["http://hub-mirror.c.163.com"]
}
# systemctl restart docker.service
```

**国内加速地址有：**

Docker中国区官方镜像
[https://registry.docker-cn.com](https://registry.docker-cn.com)

网易
[http://hub-mirror.c.163.com](http://hub-mirror.c.163.com)

ustc
[https://docker.mirrors.ustc.edu.cn](https://docker.mirrors.ustc.edu.cn)

中国科技大学
[https://docker.mirrors.ustc.edu.cn](https://docker.mirrors.ustc.edu.cn)

阿里云容器  服务
[https://cr.console.aliyun.com/](https://cr.console.aliyun.com/)

### 基本命令
#### 创建容器
通过[@openstf ](/openstf ) 已经准备好的镜像可以直接运行 

```shell
# 安卓 9
docker run --name vmos-9 -it -v /aosp-9.0:/aosp  openstf/aosp:jdk8 /bin/bash
# 安卓 8
docker run --name vmos-8 -it -v /aosp-8.1:/aosp  openstf/aosp:jdk8 /bin/bash
# 安卓 7
docker run --name vmos-7 -it -v /aosp-7.1:/aosp  openstf/aosp:jdk8 /bin/bash
# 安卓 5
docker run --name vmos-5-jdk6 -it -v /aosp-5.1:/aosp  openstf/aosp:jdk6 /bin/bash
docker run --name vmos-5-jdk7 -it -v /aosp-5.1:/aosp  openstf/aosp:jdk7 /bin/bash

```

#### 查看容器
当运行虚拟机后, 容器会自动创建可以通过`docker container ls -a`看到创建的容器

```shell
$ docker container ls -a
CONTAINER ID        IMAGE                    COMMAND             CREATED             STATUS                     PORTS               NAMES
822daa0d13da        openstf/aosp:jdk6             "/bin/bash"         2 minutes ago         Exited (0) 2 minutes ago                         interesting_brown
```

#### 修改容器名字

```shell
$ docker container rename interesting_brown aosp_51
```

#### 启动容器

```shell
$ docker start aosp_51
```

#### 附加容器

```shell
$ docker attach aosp_51
```

#### 编译安卓
附加容器后, 就可以执行基本的AOSP编译命令了
