### 概述

这是由redstoner_35所开发的，基于FlashLightOS-Entry固件所开发的低成本32mm侧按全程降压恒流驱动。这款驱动相比强大的[Xtern-Ripper V3](https://github.com/redstoner-35/Xtern-Ripper)来说更为精简实用，且简化了操作逻辑，更适合于面向圈外人士使用的实用手电。且该驱动为单PCB设计，6mm的最大高度基本可以适配市面上大部分主流手电外壳。


### 关于FlashLightOS-Entry

FlashLightOS-Entry是面向低成本，较低性能的增强型8位8051单片机平台的FlashLightOS实现。相比运行在ARM平台的标准版FlashLightOS进行了更进一步的逻辑调整和优化降低性能和代码开销。该实现相比标准版FlashLightOS移除了几乎所有的挡位自定义和运行状态记录的功能，挡位逻辑硬编码于单片机的ROM内不可调整，也不需要外挂EEPROM作为系统的数据存储器。这使得该固件可以运行在32-48MHz主频的增强型8051单片机上，并且通过单片机片内EEPROM完成少量数据的存储从而最大限度的简化了驱动数字域部分的电路设计。

### 驱动硬件结构

该驱动为了节约成本使用了现成的DC-DC芯片，并通过基于运放的外部恒流环路根据通过的LED电流，动态的调整DCDC芯片反馈引脚的电位从而调节输出电压实现恒流。对于LED辉度的控制，MCU则通过PWM+二阶RC滤波的方式为恒流环路提供设置输出电流的参考电压使LED运行在目标电流。与此同时，为了解决在低输出电流下检流部分的信噪比不足导致低亮度抖动，因此本驱动加入了LED电流的切换开关根据输出电流的范围动态选择电流通过的检流电阻确保在整个挡位的选择范围内信噪比足够。

### 工程结构

该工程一共有3个文件夹，分别对应2套PCB。文件格式为Altium Designer 15。可以用LCEDA导入后转换为LCEDA格式以便查看BOM和位号。各个PCB作用如下：

+ MainPCB：这个文件夹包含驱动的主板。
+ SideKeyPCB：这个文件夹包含配套使用的LED侧按按键板，通过细排线连接到驱动上层的子板。侧按板直径12mm，适配东东海D8B外壳。
+ Firmware：这个文件夹包含驱动的基础版本固件源代码（仅支持侧按功能）
+ Firmware-DualMode：这个文件夹包含驱动的双模式版本固件源代码（测按/尾按操作功能均支持）

### PCB打样注意事项

+ `MainPCB`：1.6mm 4层板，默认层叠顺序（Top GND VCC Bottom）必须过孔塞树脂+沉金。内层必须加厚为1Oz。
+ `SideKeyPCB`:1.0mm厚度双面板，铜厚1Oz喷锡即可。

### 调试说明

该驱动的设计为免调试的。只需要按照PCB上的丝印、器件位置和BOM表焊接好所有器件并为单片机刷入附带的[程序文件](/MainPCB/Firmware.hex)即可使用，不需要任何调试工作。对于驱动的成品图片可以参考下方：
![背面](/%E6%88%90%E5%93%81%E5%9B%BE%E7%89%87/1.jpg)
![正面](/%E6%88%90%E5%93%81%E5%9B%BE%E7%89%87/2.jpg)

工程同时提供了[双模换挡功能的固件](/MainPCB/Firmware-DualMode.hex)供您体验。如果您使用的外壳为颈部电子按键+尾按双开关的筒子，则建议您刷入尾按版本的固件体验尾按换挡的便捷。

对于背面的装筒效果则如下图所示：

![装筒后背面](/%E6%88%90%E5%93%81%E5%9B%BE%E7%89%87/3.jpg)

**注:V1.1版本的驱动在背部新增了一个用于选择2串和三串模式的配置电阻。如果该电阻留空，则为三串模式。如果电阻短接，则为2串模式。电阻的具体位置请参照下图**
![新版本背面](/%E6%88%90%E5%93%81%E5%9B%BE%E7%89%87/4.PNG)

请特别注意需要根据电池的串数配置正确的调整配置电阻。如果3串锂电池使用2串的配置会导致低电压保护失效引起电池过度放电而损毁！

### 驱动的操作逻辑

该驱动具备无极调光和传统挡位模式两种逻辑供用户自由选择：
![无极调光](/Firmware/Img/Ramp.png)
![挡位模式](/Firmware/Img/Gear.png)

需要补充的是，图中的任意挡位均可以通过单击立即回到关闭状态。对于图中没有提及的特殊逻辑，则请参考如下内容：

+ 切换无极调光和挡位模式：关机状态下四击开关，此时开关指示灯将会闪烁三次。如果闪烁颜色为红色则无极调光已被关闭，绿色则为开启。
+ 战术模式：关机状态下六击开关，手电主灯迅速频闪一次表示进入战术模式。此时按下开关手电以极亮运行，松开则熄灭，再度6击则退出。
+ 按键锁：关机状态下五击开关，手电主灯点亮0.5秒表示进入锁定模式，此时除五击解锁外的任何操作均被忽略。
+ *侧按指示灯亮度调整：在月光挡位下五击开关，侧按指示灯亮度开始从最高到最低循环变化。当指示灯位于你想要的亮度时，单击关机并保存。* (该特性因为需要兼容尾按断电换挡的大量代码因此在双模固件内被移除)
+ 查询电量：进入操作和逻辑中的一致。当进入电量查询模式后手电首先以红黄绿依次点亮的方式提示用户即将显示电量，延迟1秒后手电将通过红色，黄色和绿色的指定次数闪烁提示用户当前的电池电压。计算公式为：**红色闪烁次数x10+黄色闪烁次数+绿色闪烁次数x0.1=当前电池电压(V)**，如果指定的位为0，则指示灯将会通过瞬时点亮的快速闪烁提示用户。例如`红色慢闪1次-黄色快闪一次-绿色慢闪4次`表示电池电压为10x1+0+4x0.1=10.4V。播报完电池当前电压后，手电将会在延迟2秒后通过后续的长亮(或者慢闪)提示用户当前电池电压对应的总体电量水平（指示方式参考下文中驱动位于运行状态时的指示）

同时该驱动具备错误检测系统，如果驱动检测到无法解决的致命问题则驱动会通过指示灯提示用户发生的错误类型。
每个指示循环首先以红黄绿的颜色切换闪烁开始，然后通过紧跟着的红色慢闪次数指示错误ID号，慢闪结束后会停顿一会并重新开始循环，对于ID号所对应的错误描述请参考如下内容：

+ 闪烁1次(Fault_DCDCFailedToStart) 驱动在上电自检尝试启动DCDC时失败。检查驱动输出正极是否对外壳短路，DCDC芯片的使能信号和内部LDO输出电压是否正常。
+ 闪烁2次(Fault_DCDCENOOC) 驱动在上电自检时无法关闭DCDC。检查单片机控制DCDC EN电源的输出是否能正确关闭，EN是否受控。
+ 闪烁3次(Fault_DCDCShort) 驱动在运行时检测到输出短路。
+ 闪烁4次(Fault_DCDCOpen) 驱动在运行时检测到输出空载，检查LED是否损坏。
+ 闪烁5次(Fault_NTCFailed) 驱动在运行期间检测到测量驱动温度的热敏电阻出现异常，检查热敏电阻和测温部分的上拉电阻是否损坏。
+ 闪烁6次(Fault_OverHeat) 驱动检测到外壳温度过高，请等待手电冷却。冷却后故障会自动解除
+ *闪烁7次(Fault_InputConnectFailed, 驱动检测到电源输入动态压降过大。请检查电池架和电池是否正常，且驱动的固定压环是否充分拧紧*(该特性因为需要兼容尾按断电换挡因此被移除，对于基础版固件则存在)
+ 闪烁8次(Fault_MPUHang) 驱动检测到MCU故障挂死并触发强制复位。如果该故障第一次出现，您可重新刷入固件。仍然无法解决则需要更换MCU。

对于电压指示，驱动在工作状态以及首次装入电池时会通过指示灯点亮的方式提示用户当前系统的电池电量，各电量状态和对应的指示灯状态请参考如下内容：

+ 绿色常亮：电池电量充足
+ 黄色常亮：电池电量较为充足
+ 红色常亮：电池电量不足，请尽快为手电充电
+ 红色慢闪：电池电量严重不足，请立即为手电充电（此时手电的可用挡位和功能将会被限制）

**V1.1版本独有机制：在手电首次装入电池时，驱动将会通过黄色快速闪烁指示用户通过配置电阻设置的电池串数。如果闪烁2次则驱动为2串模式，闪烁3次则为三串模式。**

### 尾按换挡操作逻辑

对于使用东东海D4GC-M4A-D5E等外壳的用户，本驱动提供了实验性质的双模式换挡固件供用户尝试。该固件除了支持原版固件通过颈部电子开关换挡的功能以外，还支持使用手电的尾部机械开关通过断电的方式进行快捷换挡。换挡方式如下：

**特别注意：如果用户通过侧部开关令手电进入关闭状态，则尾部开关换挡功能将被禁用。此时按压尾部开关则无法换挡**

+ 手电开启且处于传统挡位模式下，用户轻按尾按后手电按照从低到高(`0.8A->1.5A->3A->7.5A->0.8A...`)再到低反复循环的方式换挡，快速轻按2次则进入极亮档，快速轻按3次则进入爆闪挡位。如果需要关机，则重按尾按到底听到咔哒声后松手即可关闭。通过尾按关闭手电后下次再重按尾按打开手电后，手电会回到上次关闭之前的挡位继续运行（如果上次关机前挡位为特殊挡位则会回到低亮档）
+ 手电开启且处于无极调光模式下，用户可通过轻按尾按一次进行亮度调整。轻按尾按后手电亮度将会开始从低到高，再到低亮反复变化。当手电亮度达到您的需求后，再次轻按尾按后手电将会保存亮度。快速轻按2次则进入极亮档，快速轻按3次则进入爆闪挡位。如果需要关机，则重按尾按到底听到咔哒声后松手即可关闭。通过尾按关闭手电后下次再重按尾按打开手电后，手电会回到上次关闭之前的亮度继续运行（如果上次关机前挡位为特殊挡位则会回到常亮挡位）

----------------------------------------------------------------------------------------------------------------------------------
© redstoner_35 @ 35's Embedded Systems Inc.  2024