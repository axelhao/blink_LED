#include <Arduino.h>

// ==================== 硬件相关 ====================
// Blue Pill 板载 LED：PC13，低电平点亮（active LOW）。
// PC13 没有定时器通道，analogWrite() 无效，所以呼吸/渐变都用「软件 PWM」实现。
#define LED_PIN PC13

// ==================== 模式定义 ====================
enum LightMode {
  MODE_OFF    = 0,   // 0: 熄灭
  MODE_ON     = 1,   // 1: 常亮
  MODE_BREATH = 2    // 2: 呼吸灯
};

LightMode currentMode = MODE_OFF;  // 默认初始模式为熄灭

// ==================== 亮度与时间参数 ====================
// 亮度统一用 0 ~ 10000 的整数表示（分辨率 0.01%），方便做平滑斜坡
static const int32_t kLevelMax = 10000;

// 【可调】呼吸周期：一个完整“亮 → 暗 → 亮”的时间（毫秒）
// 3000 偏快 / 5000 自然舒缓 / 8000 很慢的深呼吸
static const uint32_t kBreathPeriodMs = 4000;

// 【可调】软件 PWM 帧长：1 ms（= 1 kHz）。远高于人眼闪烁融合频率，肉眼看不出“闪”
static const uint16_t kPwmFrameUs = 1000;

// 【可调】模式切换的渐变速度：亮度走完整个量程（0 → 100%）所需时间（毫秒）
// 400 干脆利落 / 700 柔和 / 1200 很慢很柔
static const uint32_t kFadeFullScaleMs = 700;

// 亮度换算脉宽的除数：duty(µs) = level² / kDutyScale
static const uint32_t kDutyScale = ((uint32_t)kLevelMax * kLevelMax) / kPwmFrameUs;

// ==================== 运行时状态 ====================
static int32_t  currentLevel  = 0;  // 当前实际亮度（被 fade 斜坡限制，不会突变）
static uint32_t lastFrameMs   = 0;  // 上一帧的时间戳
static uint32_t breathStartMs = 0;  // 进入呼吸模式的时刻（用于计算相位）

// ---- 底层：点亮 / 熄灭（on = 亮；PC13 低电平亮）----
static inline void ledWrite(bool on) {
  digitalWrite(LED_PIN, on ? LOW : HIGH);
}

// ---- 各模式“最终想要”的亮度 ----
static int32_t targetLevel(uint32_t nowMs) {
  switch (currentMode) {
    case MODE_ON:
      return kLevelMax;                        // 常亮 = 全亮
    case MODE_BREATH: {
      const uint32_t half = kBreathPeriodMs / 2;
      const uint32_t t   = (nowMs - breathStartMs) % kBreathPeriodMs;  // 相位
      const uint32_t tri = (t < half) ? t : (kBreathPeriodMs - t);     // 三角波 0→half→0
      return (int32_t)((tri * (uint32_t)kLevelMax) / half);
    }
    default:
      return 0;                                // 熄灭 = 全暗
  }
}

// ---- 让 currentLevel 以固定斜率逼近目标值：fade（渐变）就是这么来的 ----
static void updateLevel(uint32_t nowMs) {
  uint32_t dt = nowMs - lastFrameMs;
  lastFrameMs = nowMs;
  if (dt > 100) dt = 100;                    // 万一中间卡顿，限制单次步长，避免亮度突跳

  const int32_t target = targetLevel(nowMs);
  if (currentLevel == target) return;

  const int32_t maxStep = (int32_t)((dt * (uint32_t)kLevelMax) / kFadeFullScaleMs);
  if (currentLevel < target) {
    currentLevel += maxStep;
    if (currentLevel > target) currentLevel = target;
  } else {
    currentLevel -= maxStep;
    if (currentLevel < target) currentLevel = target;
  }
}

// ---- 输出一帧软件 PWM：先亮 duty µs，剩下时间熄灭 ----
static void pwmFrame() {
  // gamma(≈2) 校正：人眼对暗部更敏感，平方后亮暗过渡更细腻柔和
  uint32_t duty = ((uint32_t)currentLevel * (uint32_t)currentLevel) / kDutyScale;
  if (duty > kPwmFrameUs) duty = kPwmFrameUs;

  if (duty == 0) {                           // 全暗：整帧熄灭
    ledWrite(false);
    delayMicroseconds(kPwmFrameUs);
    return;
  }
  ledWrite(true);                            // 亮
  delayMicroseconds((uint16_t)duty);
  ledWrite(false);                           // 灭
  delayMicroseconds((uint16_t)(kPwmFrameUs - duty));
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // 默认熄灭（PC13 高电平灭）

  lastFrameMs   = millis();
  breathStartMs = lastFrameMs;

  Serial.begin(115200);
  delay(1000);
  Serial.println("======================================");
  Serial.println("  Blue Pill 双向串口控制程序启动");
  Serial.println("  请输入指令切换模式：");
  Serial.println("  输入 [0] : 关闭 LED（渐暗）");
  Serial.println("  输入 [1] : 常亮 LED（渐亮）");
  Serial.println("  输入 [2] : 开启呼吸灯模式（渐入）");
  Serial.println("======================================");
}

void loop() {
  // 1. 处理串口指令：一次把缓冲区读空，连发多个字符也不会丢
  while (Serial.available() > 0) {
    const char incomingChar = Serial.read();

    // 过滤掉回车换行等无关字符，只处理 0/1/2
    if (incomingChar == '0' || incomingChar == '1' || incomingChar == '2') {
      Serial.print(">> 收到电脑指令: ");
      Serial.println(incomingChar);

      // 注意：这里只改 currentMode，不改 currentLevel，
      // 亮度由 updateLevel() 平滑过渡过去，所以切换时是“渐变”而不是“硬跳”。
      if (incomingChar == '0') {
        currentMode = MODE_OFF;
        Serial.println("状态更新：LED 渐暗至关闭。");
      } else if (incomingChar == '1') {
        currentMode = MODE_ON;
        Serial.println("状态更新：LED 渐亮至常亮。");
      } else {
        currentMode = MODE_BREATH;
        breathStartMs = millis();  // 重置相位：从最暗起步，接上前一刻的亮度做一次柔和过渡
        Serial.println("状态更新：进入呼吸灯模式...");
      }
    }
  }

  // 2. 每帧做两件事：更新亮度斜坡 + 输出一帧软件 PWM
  //    每帧只有 1 ms，所以 fade 能 1 kHz 地平滑更新，串口响应也依然及时。
  const uint32_t now = millis();
  updateLevel(now);
  pwmFrame();
}