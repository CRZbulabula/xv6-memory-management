# 操作系统内存管理作业报告
陈荣钊 伍冠宇 高俊峰 孙梓健

## 1.环境和运行说明
本作业在教师提供的xv6版本下进行编写，配置方法同xv6。

测试环境：`WSL`、`Ubuntu18.04`

需求软件：qemu, make, gcc

运行方法：在xv6文件夹下输入`make qemu-nox`可打开无图形界面的qemu命令行，等待初始化完成出现`$`号时，可以输入命令。随后输入下述命令进行相应测试。

`virtualMemoryTest`     虚拟页式存储测试

`StackGrowTest`         栈增测试

`NullPtrProtectTest`    零指针保护测试

`mallocTest`            随机申请和释放内存统计外碎片大小

## 2.作业情况说明
本作业实现的内存管理功能有：
- 虚拟页式存储管理
- 栈自增
- 零指针保护
- 对比四种分区分配算法

### 2.1 虚拟页式存储管理

### 2.2 栈自增
xv6中，栈的大小被统一写死为4KB。这对于系统栈一般是够用的，但是对于用户程序就捉襟见肘。栈自增可以解决该问题，使栈的空间在程序运行时动态地增加，大大地增强了OS的服务能力。

#### 2.2.1 实现原理
在进程结构体`proc`中新增变量`stackSize`表示进程当前使用的栈大小。把用户栈（虚拟地址）放在最顶端即`KERNBASE`向下增长。保持栈底到堆顶至少一个页面的差距。当用户使用栈底之下的地址时，因为该地址还未分配物理空间，所以会引发缺页中断，此时增长栈。

#### 2.2.2 测试方法
在`StackGrowTest.c`中，`testKB`和`testMB`以递归方式每次声明char数组在栈中申请1KB或1MB的空间。经测试可申请249MB的栈空间，即递归249层。`main`中以直接声明char数组的方式直接申请栈空间，经测试可申请251MB的栈空间。

#### 2.2.3 主要涉及的文件
vm.c中的`stackIncre`函数，增长栈空间，新分配一页物理内存给进程，更新页表。

vm.c中的`pageFault`函数，虚拟地址处于特定范围时调用`stackIncre`函数。

`exec.c`做栈的初始化，`proc.c`fork时复制栈空间，`syscall.c`、`sysproc.c`根据栈大小判断地址合法范围。

### 2.3 零指针保护
零指针是指向地址为0的指针。在C语言中，零指针时有效的，会访问虚拟地址为0的地址，这个地址是否有效由操作系统规定。现代编程习惯认为指向地址为0的指针是空指针，空指针不可使用，这样规定了空指针，可以防止指针指向不确定的内存空间而引起错误。xv6中可以使用零指针，我们实现了零指针保护的功能，即零指针不可用。

#### 2.3.1 实现原理
在`exec.c`中，加载程序进内存时，原先是从地址零开始装载，现改为从第二个页面开始，空出第一个页面。同时修改`Makefile`使编译程序时所有地址加上了一个页面的偏移量(`-Ttest`)。这样没有对用户行为造成改变，但虚拟地址第一页为空。然后任何对虚拟地址小于一个页面的地址进行访问都会引发缺页中断，这时再杀死进程就达到了零指针保护的目的。

#### 2.3.2 测试方法
`NullPtrProtectTest.c`中先声明了一个零指针，然后对其访问。如果程序正常结束，说明测试失败，零指针保护无效。如果显示OS杀死该进程，说明测试成功，零指针保护有效。

### 2.4 对比四种分区分配算法