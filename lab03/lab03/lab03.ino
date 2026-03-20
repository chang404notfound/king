/*
 * SOS LED 闪烁程序 - 双LED版本
 * 支持: 13号端口(板载LED) + 26号端口(外接LED)
 * 信号模式: 短闪(200ms) ×3 → 长闪(600ms) ×3 → 短闪(200ms) ×3 → 停顿(2000ms)
 * 使用 millis() 实现非阻塞控制
 */

const int LED_PIN_2 = 2;        // 板载LED引脚
const int LED_PIN_26 = 26;        // 外接LED引脚（第26号端口）

// 时间常量（毫秒）
const unsigned long DOT_TIME = 200;      // 短闪持续时间
const unsigned long DASH_TIME = 600;     // 长闪持续时间  
const unsigned long GAP_TIME = 200;      // 闪之间的间隔
const unsigned long CYCLE_PAUSE = 2000;  // 完整SOS后的停顿

// SOS 模式定义: 1=短闪, 2=长闪, 0=间隔
// 模式: S(短短短) - O(长长长) - S(短短短)
const int PATTERN[] = {1, 0, 1, 0, 1, 0,   // S: 短 间隔 短 间隔 短 间隔
                       2, 0, 2, 0, 2, 0,   // O: 长 间隔 长 间隔 长 间隔  
                       1, 0, 1, 0, 1};     // S: 短 间隔 短 间隔 短
const int PATTERN_LENGTH = 15;

// 状态变量
enum State { LED_OFF, LED_ON, PAUSE_BETWEEN_CYCLES };
State currentState = LED_OFF;

int patternIndex = 0;                    // 当前执行到模式的哪一步
unsigned long previousMillis = 0;          // 记录上一次状态变化的时间
unsigned long currentInterval = 0;         // 当前状态需要持续的时间

void setup() {
  // 初始化两个LED引脚
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_26, OUTPUT);
  
  // 初始状态：全部熄灭
  digitalWrite(LED_PIN_2, LOW);
  digitalWrite(LED_PIN_26, LOW);
  
  Serial.begin(9600);
  Serial.println("双LED SOS 信号发送器启动...");
  Serial.println("端口13(板载LED) + 端口26(外接LED)");
}

// 同时控制两个LED的开关
void setBothLEDs(int state) {
  digitalWrite(LED_PIN_2, state);
  digitalWrite(LED_PIN_26, state);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 检查是否到了状态切换的时间
  if (currentMillis - previousMillis >= currentInterval) {
    previousMillis = currentMillis;
    
    switch (currentState) {
      case LED_OFF:
        // 熄灭状态结束，开始下一个闪烁
        if (patternIndex < PATTERN_LENGTH) {
          int signal = PATTERN[patternIndex];
          
          if (signal == 1) {
            // 短闪
            setBothLEDs(HIGH);
            currentInterval = DOT_TIME;
            Serial.print(".");
          } else if (signal == 2) {
            // 长闪
            setBothLEDs(HIGH);
            currentInterval = DASH_TIME;
            Serial.print("-");
          } else if (signal == 0) {
            // 间隔（保持熄灭，直接处理下一个）
            currentInterval = GAP_TIME;
            patternIndex++;
            return; // 不切换状态，保持OFF
          }
          
          currentState = LED_ON;
          patternIndex++;
        } else {
          // 一个完整SOS周期完成，进入长停顿
          Serial.println(" [SOS完成，等待下一轮...]");
          currentInterval = CYCLE_PAUSE;
          currentState = PAUSE_BETWEEN_CYCLES;
          patternIndex = 0;
        }
        break;
        
      case LED_ON:
        // 点亮结束，熄灭两个LED，准备下一个
        setBothLEDs(LOW);
        currentInterval = GAP_TIME;
        currentState = LED_OFF;
        break;
        
      case PAUSE_BETWEEN_CYCLES:
        // 长停顿结束，重新开始SOS
        currentState = LED_OFF;
        currentInterval = 0; // 立即开始
        Serial.println("开始新的SOS周期...");
        break;
    }
  }
  
  // 在这里可以添加其他非阻塞代码
  // 例如：读取传感器、处理串口数据等
}