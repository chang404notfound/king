/*
 * 警车双闪灯效（双通道反相PWM）- 最终稳定版
 * 修复所有潜在问题，确保平滑反相呼吸
 */

// ========== 配置区 ==========
#define LED_A_PIN 16      // LED A 引脚（建议蓝色）
#define LED_B_PIN 17      // LED B 引脚（建议红色）
// 如果18/19不工作，尝试换成：5, 17, 16, 4 等

#define PWM_CHANNEL_A 0   
#define PWM_CHANNEL_B 1   
#define PWM_FREQ 5000     
#define PWM_RESOLUTION 8  

// ========== 全局变量 ==========
int brightness = 0;           // 当前亮度（0-255）
int fadeDirection = 1;          // 1 = 增加, -1 = 减少
const int fadeStep = 2;         // 变化步长
const int delayTime = 20;       // 延时（毫秒）

// 调试计数器
unsigned long loopCount = 0;

void setup() {
  // 初始化串口（必须等待连接成功）
  Serial.begin(115200);
  delay(1000);  // 给串口监视器打开的时间
  
  Serial.println("\n\n========================================");
  Serial.println("    警车双闪灯效 - 最终稳定版");
  Serial.println("========================================");
  Serial.println("请确认：");
  Serial.println("1. 串口监视器波特率设置为 115200");
  Serial.println("2. LED_A 接 GPIO" + String(LED_A_PIN) + "（建议蓝色）");
  Serial.println("3. LED_B 接 GPIO" + String(LED_B_PIN) + "（建议红色）");
  Serial.println("4. 两个LED都有220Ω限流电阻");
  Serial.println("========================================\n");
  
  // 配置PWM通道
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_A_PIN, PWM_CHANNEL_A);
  
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_B_PIN, PWM_CHANNEL_B);
  
  // 初始测试：两个LED都亮一下，确认硬件正常
  Serial.println("硬件测试：两灯同时亮起...");
  ledcWrite(PWM_CHANNEL_A, 255);
  ledcWrite(PWM_CHANNEL_B, 255);
  delay(500);
  
  Serial.println("硬件测试：两灯同时熄灭...");
  ledcWrite(PWM_CHANNEL_A, 0);
  ledcWrite(PWM_CHANNEL_B, 0);
  delay(500);
  
  Serial.println("开始反相呼吸效果...\n");
}

void loop() {
  // ========== 核心：反相PWM计算 ==========
  
  // LED_A: 正相（随brightness增加而变亮）
  int pwmA = brightness;
  
  // LED_B: 反相（随brightness增加而变暗）
  int pwmB = 255 - brightness;
  
  // 安全限制（防止任何溢出）
  pwmA = constrain(pwmA, 0, 255);
  pwmB = constrain(pwmB, 0, 255);
  
  // 输出到PWM通道
  ledcWrite(PWM_CHANNEL_A, pwmA);
  ledcWrite(PWM_CHANNEL_B, pwmB);
  
  // ========== 调试输出（每20次循环打印一次，避免刷屏） ==========
  loopCount++;
  if (loopCount % 20 == 0) {
    Serial.print("亮度基准: ");
    Serial.print(brightness);
    Serial.print(" | LED_A(正相): ");
    Serial.print(pwmA);
    Serial.print(" | LED_B(反相): ");
    Serial.println(pwmB);
  }
  
  // ========== 更新亮度值（关键：先计算，再判断，最后赋值） ==========
  int next = brightness + (fadeStep * fadeDirection);
  
  // 边界检测和反转
  if (next >= 255) {
    brightness = 255;
    fadeDirection = -1;  // 到达顶端，开始下降
    Serial.println(">>> 到达顶端，反转方向（A将变暗，B将变亮）");
  } 
  else if (next <= 0) {
    brightness = 0;
    fadeDirection = 1;   // 到达底端，开始上升
    Serial.println(">>> 到达底端，反转方向（A将变亮，B将变暗）");
  } 
  else {
    brightness = next;  // 正常更新
  }
  
  delay(delayTime);
}