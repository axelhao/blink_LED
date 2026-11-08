# Blue Pill 呼吸灯 · 串口模式控制

基于 **STM32F103C8T6（Blue Pill）+ PlatformIO + Arduino 框架** 的板载 LED 演示程序：
通过串口发送 `0` / `1` / `2` 切换「熄灭 / 常亮 / 呼吸灯」三种模式，
**所有模式切换都带 fade（渐变过渡）**，呼吸灯为 4 秒一轮的自然呼吸效果。

| 项目 | 说明 |
| --- | --- |
| MCU | STM32F103C8T6（72 MHz / 64 KB Flash / 20 KB RAM） |
| 框架 | Arduino（`framework = arduino`，ststm32 platform 20.0.0） |
| 开发环境 | PlatformIO（VSCode 插件或 CLI） |
| 烧录调试 | ST-Link（SWD，`upload_protocol = stlink`） |
| 串口 | UART1，115200 8N1 |
| 资源占用 | RAM 6.2%（1272 B）/ Flash 20.7%（13552 B） |


## 1. 功能特性

| 指令 | 模式 | 视觉效果 |
| --- | --- | --- |
| `2` | 呼吸灯 BREATH | 2 秒渐亮 + 2 秒渐暗，**4 秒一轮**，符合人眼感的 gamma 曲线 |
| `1` | 常亮 ON | 从当前亮度在 700 ms 内平滑升到全亮（渐亮） |
| `0` | 熄灭 OFF | 从当前亮度在 700 ms 内平滑降到全灭（渐暗） |

- 任意两个模式之间切换都不会「硬跳」，而是按固定斜率平滑过渡。
- 呼吸模式进行中切到常亮/熄灭，同样从**当前亮度**接着渐变，不会闪一下。
- 串口指令一次读空缓冲区，连续快速发送多个字符也不会丢。
- 非阻塞思路：主循环每帧只占 1 ms，串口仍能及时响应。


## 2. 硬件与接线

### 2.1 板载 LED

Blue Pill 的板载 LED 接在 **PC13**，并且是**低电平点亮**（active LOW）：

- 引脚输出 `LOW` → 点亮
- 引脚输出 `HIGH` → 熄灭

> 注意：PC13 灌电流能力仅 3 mA 左右，LED 本身偏暗，属正常现象。

### 2.2 ST-Link 烧录接线（SWD）

| ST-Link | Blue Pill |
| --- | --- |
| SWDIO | DIO（PA13） |
| SWCLK | CLK（PA14） |
| GND | GND |
| 3.3V | 3.3V（也可不接，由 USB 供电） |

### 2.3 串口接线（UART1）

| USB-TTL | Blue Pill |
| --- | --- |
| TX | PA10（RX1） |
| RX | PA9（TX1） |
| GND | GND（必须共地） |

波特率 **115200**，8 数据位、无校验、1 停止位。TX/RX 要交叉连接。


## 3. 目录结构

```
bluepill_blink_LED/
├── platformio.ini        # 板卡 / 框架 / 烧录配置
├── src/
│   └── main.cpp          # 全部业务代码（单文件）
├── include/  lib/  test/ # PlatformIO 生成的标准空目录
└── .vscode/              # IDE 配置（可忽略）
```


## 4. 快速开始

### 4.1 使用 VSCode + PlatformIO

1. 用 VSCode 打开本工程目录（含 `platformio.ini` 的那一层）。
2. 底部状态栏：
   - **Build**（√ 图标）编译
   - **Upload**（→ 图标）编译并烧录
   - **Monitor**（插头图标）打开串口监视器

### 4.2 使用命令行

```bash
# 编译
pio run

# 编译 + 烧录（ST-Link）
pio run -t upload

# 打开串口监视器（115200）
pio device monitor
```

Windows 上 pio 一般位于 `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`。


## 5. 使用说明

1. 上电复位后，板载 LED 熄灭，串口打印菜单：

   ```
   ======================================
     Blue Pill 双向串口控制程序启动
     请输入指令切换模式：
     输入 [0] : 关闭 LED（渐暗）
     输入 [1] : 常亮 LED（渐亮）
     输入 [2] : 开启呼吸灯模式（渐入）
   ======================================
   ```

2. 在串口监视器里输入单个字符 `0` / `1` / `2` 并发送，即可切换模式。
   - 回车、换行等其它字符会被自动过滤，勾不勾「发送新行」都可以。
   - 每次识别到有效指令会回显：`>> 收到电脑指令: 2` 以及当前状态。


## 6. 可调参数

全部集中在 `src/main.cpp` 顶部，改完重新烧录即可：

| 参数 | 默认值 | 含义 | 调整建议 |
| --- | --- | --- | --- |
| `kBreathPeriodMs` | `4000` | 呼吸周期（一轮「亮→暗→亮」） | `3000` 紧凑 / `4000` 自然 / `6000~8000` 很慢 |
| `kFadeFullScaleMs` | `700` | 模式切换渐变时长（走完 0→100% 的时间） | `300` 干脆 / `700` 柔和 / `1200` 很柔 |
| `kPwmFrameUs` | `1000` | 软件 PWM 帧长（1 ms → 1 kHz） | 建议保持 1000~2000，不要小于 500 |
| `kLevelMax` | `10000` | 亮度内部量程（分辨率 0.01%） | 一般不用改 |

> 渐变时长与亮度跨度成正比：从 50% 切到 0% 约耗时 `kFadeFullScaleMs / 2`。


## 7. 实现原理

### 7.1 为什么用软件 PWM？

`analogWrite()` 需要引脚所在定时器的通道输出，而 **PC13 没有定时器通道**，
调用 `analogWrite(PC13, x)` 只会退化成全亮/全灭。所以这里用最朴素的办法：

```cpp
// 一帧 1 ms：先亮 duty 微秒，再把剩余时间熄灭
ledWrite(true);
delayMicroseconds(duty);
ledWrite(false);
delayMicroseconds(kPwmFrameUs - duty);
```

1 kHz 的刷新率远高于人眼闪烁融合频率（约 60~90 Hz），所以肉眼看不出「闪」，
只看到亮度。

### 7.2 「目标亮度 + 实际亮度」= fade

程序把两件事解耦：

```
模式  --targetLevel()-->  目标亮度(0~10000)
                                |
                         updateLevel()  <- 每 1 ms 用固定斜率逼近
                                |
                          实际亮度 currentLevel  --> pwmFrame() --> 引脚
```

- `targetLevel()`：`OFF`→0，`ON`→10000，`BREATH`→三角波当前值。
- `updateLevel()`：每次最多走 `maxStep = dt * kLevelMax / kFadeFullScaleMs`，
  所以任何模式变化都表现为一段渐变，而不是瞬间跳变。
- 切换模式时**只改 `currentMode`**，亮度由斜坡负责追过去 —— 这正是「fade 感」的来源。

### 7.3 呼吸曲线与 gamma 校正

呼吸用三角波（`0 → 半周期 → 0`），再对亮度做平方（gamma≈2）校正：

```cpp
duty = level * level / kDutyScale;   // kDutyScale = kLevelMax² / kPwmFrameUs
```

因为人眼对暗部变化更敏感，线性亮度变化会显得「暗部跳得快、亮部发闷」；
平方之后亮暗过渡更细腻柔和，接近真实呼吸灯。

### 7.4 时间基准

- 所有节奏都基于 `millis()`，不做阻塞式长延时。
- `loop()` 每帧只耗时约 1 ms：先处理串口（`while (Serial.available())`），
  再 `updateLevel()` + `pwmFrame()`。
- `updateLevel()` 中对 `dt` 做了上限保护（100 ms），防止偶发卡顿后亮度突跳。


## 8. 常见问题（FAQ）

**LED 一直不亮 / 很暗**
1. 确认当前模式不是 `MODE_OFF`（发 `1` 试试）。
2. PC13 驱动能力只有 3 mA 左右，板载 LED 本身就暗；软件 PWM 低占空比时更暗。
3. 烧录后需要复位（OpenOCD 已经自动 `Resetting Target`）。

**串口没反应 / 乱码**
1. 波特率必须是 **115200**。
2. USB-TTL 的 GND 必须与板子共地，TX/RX 必须交叉。
3. PA9/PA10 是 UART1；如果用的是别组 UART，需要改引脚映射。

**烧录失败**
1. 检查 ST-Link 接线（SWDIO/SWCLK/GND）与杜邦线是否松动。
2. 换成短一点的杜邦线，必要时降低 SWD 速率。
3. 若克隆板是 128 KB Flash，把 `platformio.ini` 的 `board` 改为 `bluepill_f103c8_128k`。

**呼吸时看得出「台阶」**
1. 别把 `kPwmFrameUs` 调太大（> 2000 µs 会让刷新率掉到 500 Hz 以下）。
2. 把 `kBreathPeriodMs` 调大一点，同样亮度步长下过渡更慢更顺。

**想让亮度范围更大 / 完全无抖动**
把外接 LED 换到带定时器通道的引脚（如 `PA0` = TIM2_CH1、`PA8` = TIM1_CH1、
`PB0` = TIM3_CH3），改用硬件 PWM：

```cpp
analogWriteResolution(10);   // 10 位分辨率
analogWrite(PA0, duty);      // 亮度计算交给定时器，CPU 零负担、无抖动
```


## 9. 可扩展方向

- 用按键 / 编码器本地切换模式。
- 增加 `[3]` 模式：心跳、呼吸 + 闪一下、SOS 等自定义曲线。
- 亮度曲线改查表（LUT）实现自定义 gamma（如 1.8 / 2.2）。
- 把 `pwmFrame()` 换成定时器中断或 DMA 驱动，主循环彻底解放。