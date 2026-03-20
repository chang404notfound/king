// 定义LED引脚
const int ledPin1 = 2;   // ESP32板载LED，GPIO 2
const int ledPin2 = 26;  // 外接LED，GPIO 26

// 共用时间变量（两个LED频率相同）
unsigned long previousMillis = 0;
const long interval = 500;  // 1秒间隔（两个LED相同）

// LED状态变量
bool ledState1 = LOW;
bool ledState2 = LOW;

// 闪烁模式选择：true=同步闪烁，false=交替闪烁
bool syncMode = true;

void setup() {
  // 初始化串口通信，设置波特率为115200
  Serial.begin(115200);
  
  // 将两个LED引脚设置为输出模式
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  
  // 初始化时打印系统运行时间
  Serial.print("System started at: ");
  Serial.print(millis());
  Serial.println(" ms");
  
  if (syncMode) {
    Serial.println("Mode: 同步闪烁 (Both LEDs blink together)");
  } else {
    Serial.println("Mode: 交替闪烁 (LEDs blink alternately)");
    // 初始状态设为相反，实现交替效果
    ledState2 = HIGH;
    digitalWrite(ledPin2, ledState2);
  }
}

void loop() {
  // 获取当前系统运行时间
  unsigned long currentMillis = millis();
  
  // 检查是否到达设定的时间间隔
  if (currentMillis - previousMillis >= interval) {
    // 保存当前时间
    previousMillis = currentMillis;
    
    // 切换LED1状态
    ledState1 = !ledState1;
    digitalWrite(ledPin1, ledState1);
    
    // 根据模式切换LED2状态
    if (syncMode) {
      // 同步模式：LED2与LED1状态相同
      ledState2 = ledState1;
    } else {
      // 交替模式：LED2与LED1状态相反
      ledState2 = !ledState1;
    }
    digitalWrite(ledPin2, ledState2);
    
    // 串口输出状态
    Serial.print("[");
    Serial.print(currentMillis);
    Serial.print(" ms] LED1 (GPIO 2): ");
    Serial.print(ledState1 ? "ON " : "OFF");
    Serial.print(" | LED2 (GPIO 26): ");
    Serial.println(ledState2 ? "ON " : "OFF");
  }
  
  // 此处可以添加其他非阻塞代码
}