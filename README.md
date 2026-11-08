# STM32 Blue Pill 板载 LED 闪烁

基于 **PlatformIO + Arduino framework** 的 STM32F103C8T6（Blue Pill）点灯示例。

程序不阻塞 `loop()`，使用 `millis()` 计时，每 1 秒翻转一次板载 LED（PC13）。板载 LED 为低电平点亮，代码中已处理；`millis()` 回绕也通过无符号减法正确处理。

## 硬件

| 项目 | 说明 |
| --- | --- |
| 开发板 | STM32F103C8T6 Blue Pill（64 KB Flash） |
| 下载器 | ST-Link V2 |
| 板载 LED | PC13，低电平点亮 |
| 框架 | Arduino (STM32duino) |
| 构建工具 | PlatformIO |

### SWD 接线

| ST-Link V2 | Blue Pill |
| --- | --- |
| 3.3V | 3.3V |
| SWDIO | SWDIO |
| SWCLK | SWCLK |
| GND | GND |

> 不要接 5V。推荐 Blue Pill 使用 USB 供电，ST-Link 只接 `SWDIO / SWCLK / GND`。

## 构建与烧录

在项目目录下执行：

```powershell
# 编译
pio run

# 通过 ST-Link 烧录
pio run -t upload

# 串口监视（115200）
pio device monitor
```

也可以直接使用 VS Code 底部 PlatformIO 工具栏的 **Build** / **Upload** 按钮。

调试：使用 VS Code 的 **Run and Debug**，选择 `PIO Debug`（需要 ST-Link）。

## 项目结构

```text
blink-bluepill/
├─ platformio.ini     # 开发板、框架、下载/调试工具配置
├─ src/
│  └─ main.cpp        # 非阻塞 LED 闪烁程序
├─ .vscode/
│  └─ extensions.json # 推荐安装 PlatformIO IDE 扩展
└─ README.md
```

`.pio/`、`.vscode/c_cpp_properties.json`、`.vscode/launch.json` 等构建产物与自动生成文件已在 `.gitignore` 中忽略。

## 备注

- 配置中的开发板为 `bluepill_f103c8`（64 KB Flash）。如果手上的芯片是 128 KB 版本，请把 `platformio.ini` 中的 `board` 改为 `bluepill_f103c8_128k`。
- 串口监视波特率默认为 `115200`，可在 `platformio.ini` 的 `monitor_speed` 中修改。
