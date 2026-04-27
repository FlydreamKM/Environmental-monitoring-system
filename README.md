# 嵌入式环境监测系统

## 1. 项目简介

本项目是一款基于 **STM32F103C8T6**（Cortex-M3, 72 MHz）的嵌入式环境监测系统。系统通过多路传感器实时采集环境温度、湿度、光照、烟雾浓度及可燃气体浓度，并在 1.44 英寸 128×128 彩色 TFT 液晶屏上以卡片式 UI 进行展示。当烟雾或可燃气体浓度超标时，系统会驱动蜂鸣器发出三联短音报警，同时通过 TB6612 双路电机驱动控制排气风扇进行 proportional PWM 调速排气，实现声光联动报警。

### 核心功能
- **温湿度监测**：SHT40（I2C2），精度 ±0.2 °C / ±1.8 %RH
- **光照监测**：VEML7700（I2C2），范围 0.0076 ~ 83k lux
- **烟雾监测**：MAX30105 IR 反射式检测（I2C1），支持基线校准与低通滤波
- **可燃气体监测**：MP-4 半导体气敏传感器（ADC1），估算甲烷浓度（ppm）
- **实时时钟**：RTC + LSE 32.768 kHz，VBAT 备份保持时间
- **报警联动**：蜂鸣器 + 双路 PWM 电机排气，带滞回控制避免抖动
- **GUI 显示**：ST7735 128×128 SPI 屏，左栏五卡片 + 右栏 RTC 时间

---

## 2. 硬件架构

### 2.1 主控
| 项目 | 规格 |
|------|------|
| MCU | STM32F103C8T6 |
| 内核 | ARM Cortex-M3 |
| 主频 | 72 MHz（HSE 8 MHz → PLL ×2 → AHB /2） |
| Flash | 64 KB |
| SRAM | 20 KB |

### 2.2 传感器与外设
| 模块 | 型号 | 接口 | 地址 / 通道 | 功能 |
|------|------|------|-------------|------|
| 温湿度 | SHT40 | I2C2 | 0x44 | 温度、相对湿度 |
| 光照 | VEML7700 | I2C2 | 0x10 | 环境光照（lux）|
| 烟雾 | MAX30105 | I2C1 | 0x57 | IR 反射式烟雾颗粒检测 |
| 可燃气体 | MP-4 | ADC1 | CH1 (PA1) | 甲烷浓度估算（ppm）|
| 显示屏 | ST7735 | SPI1 | — | 128×128 RGB565 |
| 电机驱动 | TB6612FNG | TIM1/TIM2 + GPIO | — | 双路 DC 电机 PWM 调速 |
| 蜂鸣器 | 有源蜂鸣器 | GPIO (PB1) | — | 三联短音报警 |
| RTC | 内部 RTC | LSE | — | 实时时钟，VBAT 备份 |

### 2.3 引脚分配表

| 功能 | 引脚 | 模式 | 说明 |
|------|------|------|------|
| **ST7735 TFT** | | | |
| SPI1_SCK | PA5 | AF_PP | SPI 时钟 |
| SPI1_MOSI | PA7 | AF_PP | SPI 数据 |
| TFT_DC | PA2 | OUT_PP | Data/Command 选择 |
| TFT_RST | PA3 | OUT_PP | 硬件复位 |
| TFT_CS | PA4 | OUT_PP | 片选 |
| TFT_BLK | PA6 | OUT_PP | 背光控制 |
| **I2C1 (MAX30105)** | | | |
| I2C1_SCL | PB8 | AF_OD (remap) | 时钟线 |
| I2C1_SDA | PB9 | AF_OD (remap) | 数据线 |
| **I2C2 (SHT40 + VEML7700)** | | | |
| I2C2_SCL | PB10 | AF_OD | 时钟线 |
| I2C2_SDA | PB11 | AF_OD | 数据线 |
| **ADC1 (MP-4)** | | | |
| ADC1_IN1 | PA1 | Analog | MP-4 VRL 输入 |
| **TIM1 / Motor A** | | | |
| TIM1_CH1 (PWMA) | PA8 | AF_PP | 电机 A PWM |
| AIN1 | PA10 | OUT_PP | 电机 A 方向 1 |
| AIN2 | PA9 | OUT_PP | 电机 A 方向 2 |
| **TIM2 / Motor B** | | | |
| TIM2_CH1 (PWMB) | PA15 | AF_PP (remap) | 电机 B PWM |
| BIN1 | PA11 | OUT_PP | 电机 B 方向 1 |
| BIN2 | PA12 | OUT_PP | 电机 B 方向 2 |
| **TB6612 STBY** | PB15 | OUT_PP | 驱动器待机 |
| **Buzzer** | PB1 | OUT_PP | 有源蜂鸣器 |
| **MAX30105 INT** | PB6 | IN_FLOAT | 中断输入（预留）|
| **MQ-x DO** | PA0 | IN_FLOAT | 数字输出（预留）|

---

## 3. 软件架构

### 3.1 整体架构

```
┌─────────────────────────────────────────┐
│  Application (main.c)                   │
│  ├─ Sensor polling loop (non-blocking)  │
│  ├─ Alarm logic (buzzer + motors)       │
│  └─ GUI update calls                    │
├─────────────────────────────────────────┤
│  GUI Layer (gui.cpp / GuiLite)          │
│  ├─ Chinese bitmap fonts                │
│  ├─ Double-buffer card compositing      │
│  └─ ST7735 PushBuffer flushing          │
├─────────────────────────────────────────┤
│  Driver Layer                           │
│  ├─ ST7735 (SPI + DMA async)            │
│  ├─ SHT40 (blocking + NB state machine) │
│  ├─ VEML7700_HAL (I2C, NOWAIT lux)      │
│  ├─ MAX30105 (I2C FIFO)                 │
│  ├─ SmokeDetector_HAL (filter + alarm)  │
│  ├─ MP-4 (ADC + ppm model)              │
│  └─ TB6612 (TIM PWM + GPIO)             │
├─────────────────────────────────────────┤
│  HAL / CMSIS (STM32CubeF1)              │
│  ├─ GPIO, I2C, SPI, ADC, TIM, RTC, DMA  │
│  └─ NVIC, SysTick                       │
└─────────────────────────────────────────┘
```

### 3.2 主循环设计

系统采用**裸机轮询（bare-metal super-loop）**架构，无 RTOS。所有传感器读取均为非阻塞或短阻塞设计：

| 传感器 | 轮询周期 | 阻塞时间 | 说明 |
|--------|----------|----------|------|
| SHT40 | 200 ms | < 1 ms 启动 + 10 ms 等待 | 非阻塞状态机（NB_Start → NB_Poll） |
| VEML7700 | 100 ms | ~0 ms | NOWAIT 模式，直接读寄存器 |
| MAX30105 (烟雾) | 200 ms | < 10 ms | FIFO 轮询 |
| MP-4 (气体) | 500 ms | ~250 ms | 50 次 ADC 采样平均（去极值） |
| RTC 时间 | 1000 ms | < 1 ms | 读备份域寄存器 |

### 3.3 使用的第三方库
- **STM32CubeF1 HAL**：ST 官方硬件抽象层（CMake 子项目 `stm32cubemx`）
- **GuiLite**：超轻量级 GUI 框架（单头文件 `GuiLite.h`）

---

## 4. 传感器工作原理

### 4.1 SHT40 温湿度
SHT40 使用 I2C 命令触发测量，典型转换时间约 8~10 ms。驱动提供两套 API：
- **阻塞式**：`SHT40_Read_Temperature_Humidity()`，内部延时 10 ms 后读取 6 字节数据（温度 + CRC + 湿度 + CRC）。
- **非阻塞式**：`SHT40_NB_Start()` 发送命令后立即返回；主循环中 `SHT40_NB_Poll()` 在 10 ms 后读取结果。避免阻塞其他传感器轮询。

公式：
- Temperature = -45 + 175 × (T_raw / 65535)  [°C]
- Humidity    = -6  + 125 × (H_raw / 65535)  [%RH]

### 4.2 VEML7700 光照
VEML7700 是一款低功耗环境光传感器，支持 4 档增益和 6 档积分时间。驱动使用 **NOWAIT 模式**（`VEML_LUX_NORMAL_NOWAIT`），在 100 ms 周期内直接读取 ALS 数据寄存器并转换为 lux：
- Resolution = 0.0036 × (800 / IT) × (2 / Gain)
- Lux = Resolution × ALS_raw
- 可选非线性校正多项式（Vishay AN84323）用于高精度场景。

### 4.3 MAX30105 → 烟雾检测
MAX30105 原本是心率/血氧传感器，本项目利用其 **IR LED 反射通道**检测烟雾颗粒对红外光的散射衰减。

流程：
1. `begin()` 初始化：仅使能 IR LED 槽位，采样率 400 Hz，脉冲宽度 215 μs，ADC 范围 4096。
2. `calibrateBaseline(100)`：在洁净空气中采集 100 个 IR 样本求平均，建立基线 `_baselineIR`。
3. `getSmokeConcentration()`：读取当前 IR 值 → 一阶 IIR 低通滤波（α=0.2）→ 计算与基线差值 → 乘以经验系数 `_scaleFactor`（默认 0.03）得到 ppm 估计值。
4. 滞回比较器：当 ppm > `_threshold`（默认 50 ppm）时置位报警标志；当 ppm < `_threshold - _hysteresis` 时解除。

### 4.4 MP-4 可燃气体
MP-4 是半导体型气敏元件，加热后表面电阻随还原性气体浓度变化。本项目采用 **3.3 V 常供电加热**（非标准 5 V），因此灵敏度显著降低，需引入经验补偿。

测量链：
1. `MP4_ReadVoltageAvg(50)`：50 次 ADC 采样，剔除一个最大/最小值后取平均，得到 VRL。
2. 计算传感器电阻：`Rs = RL × (Vc - Vrl) / Vrl`，其中 RL = 10 kΩ，Vc = 3.3 V。
3. 计算比值：`Ratio = Rs / R0`（R0 为洁净空气下的标定电阻）。
4. 3.3 V 加热补偿：`Ratio_comp = Ratio / 0.65`（经验系数，sqrt(3.3/5.0) ≈ 0.65）。
5. 对数线性反推浓度：`ppm = (Ratio_comp / a) ^ (1/b)`，其中 a=12.5，b=-0.699（基于 5 V 手册甲烷曲线）。

**注意**：3.3 V 下建议预热 ≥30 分钟，并在洁净空气中执行 `MP4_CalibrateR0()` 重新标定 R0。如跳过校准，可在 `main()` 中设置典型默认值 R0≈40 kΩ（仅供参考）。

---

## 5. 报警与联动逻辑

### 5.1 报警条件
| 传感器 | 报警阈值 | 说明 |
|--------|----------|------|
| 烟雾 (MAX30105) | ≥ 1000 ppm | 在 `main.c` 中硬编码 |
| 可燃气体 (MP-4) | ≥ 1000 ppm | 由 `MP4_ALARM_THRESHOLD` 定义 |

任一条件满足即触发蜂鸣器报警。

### 5.2 蜂鸣器报警模式
采用非阻塞状态机，周期约 1.5 s：
- **滴滴滴**：100 ms ON → 50 ms OFF × 3 次
- **长停顿**：1200 ms
- 循环直到浓度恢复正常

### 5.3 电机排气联动
| 电机 | 控制对象 | PWM 源 | 启停逻辑 |
|------|----------|--------|----------|
| Motor A | 烟雾排气 | TIM1_CH1 (PA8) | 烟雾 ppm > 1000 启动，< 900 停止 |
| Motor B | 燃气排气 | TIM2_CH1 (PA15) | 燃气 ppm > 1000 启动，< 900 停止 |

- **Proportional PWM**：浓度 1000~5000 ppm 线性映射到 PWM 0~65535。
- **Hysteresis**：启动阈值 1000 ppm，停止阈值 900 ppm，防止在阈值边界频繁启停。
- 电机方向固定为 CW（顺时针），STOP 时短接制动。

---

## 6. 编译与烧录

### 6.1 工具链
- **编译器**：`arm-none-eabi-gcc`（推荐 10.3.1 或 newer）
- **构建系统**：CMake ≥ 3.22
- **IDE**：VS Code + CMake Tools，或 STM32CubeIDE
- **调试器**：ST-Link V2 + OpenOCD

### 6.2 CMake 构建

```bash
cd health_moniter
cmake --preset Debug
# 或手动：
cmake -B build/Debug -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
```

生成的产物：
- `build/Debug/health_moniter.elf` — 调试符号文件
- `build/Debug/health_moniter.hex` — 烧录文件
- `build/Debug/health_moniter.bin` — 原始二进制

### 6.3 烧录（OpenOCD）

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/Debug/health_moniter.hex verify reset exit"
```

### 6.4 调试配置（VS Code launch.json 示例）

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug STM32",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/Debug/health_moniter.elf",
            "miDebuggerServerAddress": "localhost:3333",
            "debugServerPath": "openocd",
            "debugServerArgs": "-f interface/stlink.cfg -f target/stm32f1x.cfg",
            "preLaunchTask": "CMake: build",
            "setupCommands": [
                { "text": "target remote localhost:3333" },
                { "text": "monitor reset halt" },
                { "text": "load" }
            ]
        }
    ]
}
```

---

## 7. 目录结构

```
health_moniter/
├── Core/
│   ├── Inc/                    # 用户头文件
│   │   ├── main.h              # GPIO 引脚定义、外设句柄外部声明
│   │   ├── st7735.h            # ST7735 驱动头
│   │   ├── fonts.h             # 字模结构体声明
│   │   ├── gui.hpp             # C 兼容 GUI API
│   │   ├── GuiLite.h           # GuiLite 框架头
│   │   ├── sht40.h             # SHT40 驱动头
│   │   ├── VEML7700_HAL.h      # VEML7700 驱动头
│   │   ├── MAX30105.h          # MAX30105 驱动头
│   │   ├── SmokeDetector_HAL.h # 烟雾检测封装头
│   │   ├── mp4.h               # MP-4 气体传感器头
│   │   ├── tb6612.h            # TB6612 电机驱动头
│   │   └── stm32f1xx_it.h      # 中断处理函数原型
│   └── Src/                    # 用户源文件
│       ├── main.c              # 主程序：初始化 + 传感器轮询 + 报警
│       ├── st7735.c            # ST7735 SPI/DMA 驱动实现
│       ├── fonts.c             # 7x10 / 11x18 / 16x26 ASCII 字模数据
│       ├── gui.cpp             # GUI：汉字点阵、双缓冲、卡片刷新
│       ├── guilite_impl.cpp    # GuiLite 激活桩
│       ├── sht40.c             # SHT40 阻塞 + 非阻塞驱动
│       ├── VEML7700_HAL.cpp    # VEML7700 HAL 驱动实现
│       ├── MAX30105.cpp        # MAX30105 底层驱动实现
│       ├── SmokeDetector_HAL.cpp # 烟雾检测封装（基线/滤波/报警）
│       ├── mp4.c               # MP-4 ADC 驱动 + ppm 计算
│       ├── tb6612.c            # TB6612 自包含 PWM/GPIO 驱动
│       ├── stm32f1xx_it.c      # 中断服务程序
│       └── stm32f1xx_hal_msp.c # HAL MSP 初始化
├── Drivers/
│   ├── CMSIS/                  # ARM CMSIS 核心头文件
│   └── STM32F1xx_HAL_Driver/   # ST HAL 库源码（Inc + Src）
├── cmake/
│   ├── stm32cubemx/            # CubeMX 生成的 CMake 子项目
│   └── gcc-arm-none-eabi.cmake # GCC ARM 工具链配置
├── build/                      # 构建输出（Debug / Release）
├── CMakeLists.txt              # 主 CMake 配置
├── CMakePresets.json           # CMake 预设
├── startup_stm32f103xb.s       # 启动汇编文件
├── STM32F103XX_FLASH.ld        # 链接脚本
└── README.md                   # 本文件
```

---

## 8. 许可与致谢

### 8.1 许可证
- **STM32CubeF1 HAL / CMSIS**：版权归 STMicroelectronics，以 BSD-3-Clause 许可发布。
- **GuiLite**：Apache-2.0 许可，Copyright (c) GuiLite 项目作者。
- **ST7735 / Fonts 驱动代码**：基于公共领域（public domain）的 Adafruit 与社区代码改写。
- **本项目用户代码**（`Core/` 目录下的自定义驱动与主程序）：以 MIT 许可发布，可自由修改与再分发。

### 8.2 致谢
- [Adafruit ST7735 Library](https://github.com/adafruit/Adafruit-ST7735-Library) — TFT 驱动基础
- [SparkFun MAX3010x Sensor Library](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library) — MAX30105 驱动基础
- [Adafruit VEML7700 Library](https://github.com/adafruit/Adafruit_VEML7700) — 光照驱动基础
- [GuiLite](https://github.com/idea4good/GuiLite) — 轻量级 GUI 框架
- STMicroelectronics — STM32CubeMX / HAL 生态系统

---

## 9. 维护与扩展建议

1. **MP-4 校准**：首次上电或更换传感器后，请在洁净空气中执行 `MP4_CalibrateR0()`，并确保 3.3 V 预热 ≥30 分钟。
2. **烟雾系数标定**：`SmokeDetectorHAL` 中的 `_scaleFactor`（默认 0.03）需根据实际烟雾源（如蚊香、纸烟）进行标定。
3. **显示方向**：如使用其他尺寸的 ST7735 模组，可在 `st7735.h` 中修改 `ST7735_IS_128X128` / `ST7735_IS_160X128` 等宏。
4. **功耗优化**：可将 VEML7700 切换至 Power Save Mode，或将 MAX30105 置于 shutDown 状态以降低待机电流。
5. **RTOS 迁移**：当前为裸机轮询；如需更高实时性，可将传感器轮询拆分为 FreeRTOS 任务，利用 `SHT40_NB_Poll()` 和 DMA 完成信号进行任务同步。
