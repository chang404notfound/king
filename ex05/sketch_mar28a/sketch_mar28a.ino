/*
 * 多档位触摸调速呼吸灯
 * 功能：LED持续呼吸效果，触摸切换速度档位（1慢→2中→3快→循环）
 * 硬件：触摸传感器接GPIO 4，LED接GPIO 2（PWM输出）
 */

#define TOUCH_PIN 4       // 触摸传感器引脚
#define LED_PIN 2         // LED PWM输出引脚（ESP32支持PWM的引脚）
#define THRESHOLD 40      // 触摸触发阈值

// ========== 呼吸灯参数 ==========
const int PWM_MIN = 0;        // 最小亮度
const int PWM_MAX = 255;      // 最大亮度（8位PWM）
int brightness = 0;           // 当前亮度值
int fadeAmount = 5;           // 亮度变化步长（正数=渐亮，负数=渐暗）

// ========== 速度档位系统 ==========
/*
 * 档位说明：
 * 1档 - 缓慢呼吸：步长=1，延时=30ms（最慢）
 * 2档 - 正常呼吸：步长=3，延时=20ms（中等）  
 * 3档 - 急促呼吸：步长=8，延时=10ms（最快）
 */
struct SpeedLevel {
  int step;           // PWM步长
  int delayTime;      // 每步延时(ms)
  const char* name;   // 档位名称
};

// 定义三个速度档位
const SpeedLevel SPEED_LEVELS[] = {
  {1, 30, "缓慢"},    // 档位1
  {3, 20, "正常"},    // 档位2
  {8, 10, "急促"}     // 档位3
};
const int MAX_GEAR = 3;           // 总档位数

volatile int currentGear = 0;     // 当前档位索引（0=1档，1=2档，2=3档）
volatile bool gearChanged = false; // 档位变化标志

// ========== 触摸防抖变量 ==========
volatile unsigned long lastTouchTime = 0;
const unsigned long DEBOUNCE_TIME = 300;  // 防抖间隔300ms

// ========== 中断服务函数 ==========
void IRAM_ATTR onTouch() {
  unsigned long currentTime = millis();
  
  // 软件防抖
  if (currentTime - lastTouchTime > DEBOUNCE_TIME) {
    // 切换档位：0→1→2→0循环
    currentGear = (currentGear + 1) % MAX_GEAR;
    gearChanged = true;
    lastTouchTime = currentTime;
  }
}

void setup() {
  Serial.begin(115200);
  
  // 配置LED引脚
  pinMode(LED_PIN, OUTPUT);
  
  // 初始化PWM（ESP32使用ledc，Arduino使用analogWrite）
  #ifdef ESP32
    ledcSetup(0, 5000, 8);      // 通道0，5KHz，8位分辨率
    ledcAttachPin(LED_PIN, 0);  // 绑定引脚到通道
  #endif
  
  // 读取初始触摸值
  Serial.print("初始触摸值: ");
  Serial.println(touchRead(TOUCH_PIN));
  Serial.println("多档位触摸调速呼吸灯启动");
  Serial.println("当前档位: 1档(缓慢)");
  Serial.println("触摸传感器切换速度...");
  
  // 绑定触摸中断
  touchAttachInterrupt(TOUCH_PIN, onTouch, THRESHOLD);
}

void loop() {
  // 检查档位是否变化，打印信息
  if (gearChanged) {
    gearChanged = false;
    Serial.print("切换到档位 ");
    Serial.print(currentGear + 1);
    Serial.print(" (");
    Serial.print(SPEED_LEVELS[currentGear].name);
    Serial.println(")");
  }
  
  // 获取当前档位参数
  int step = SPEED_LEVELS[currentGear].step;
  int delayTime = SPEED_LEVELS[currentGear].delayTime;
  
  // ========== 呼吸灯核心逻辑 ==========
  // 根据档位调整步长（保持呼吸流畅性）
  // 高档位时步长大，亮度变化快
  
  // 计算实际PWM值（使用正弦曲线让呼吸更自然）
  static float angle = 0;
  angle += (step * 0.05);  // 角度增量随档位变化
  
  // 使用正弦函数产生平滑的呼吸效果（0-255）
  // sin()返回-1到1，映射到0-255
  brightness = (sin(angle) + 1.0) * 127.5;
  
  // 限制范围（防止溢出）
  brightness = constrain(brightness, PWM_MIN, PWM_MAX);
  
  // 输出PWM
  #ifdef ESP32
    ledcWrite(0, brightness);
  #else
    analogWrite(LED_PIN, brightness);
  #endif
  
  // 延时控制速度
  delay(delayTime);
}