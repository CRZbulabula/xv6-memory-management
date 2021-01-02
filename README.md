# xv6-memory-management
Testing and optimizing xV6 memory management

# qemu简介
+ 在windows下打开linux shell，输入make qemu-nox编译运行qemu操作系统

+ 阅读 <https://github.com/SerCharles/xv6VirtualMemory> 为代码添加适当改动

+ 重新编译前注意修改Makefile文件

+ 编译运行qemu，运行测试代码输出结果

# 内存管理流程

+ main.c文件控制着qemu的主线程

+ proc.c是进程的创建与内存管理

+ vm.c控制着虚拟内存

+ exec.c是运行线程的初始化

熟读以上代码的基础上展开工作