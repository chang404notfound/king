// 定义板载LED引脚，避免使用"魔法数字"
#define LED_PIN 2
void setup() {
  // put your setup code here, to run once:
  // 初始化串口通信
  Serial.begin(115200);
  // 初始化板载LED引脚为输出模式
  pinMode(LED_PIN, OUTPUT); 

}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Hello ESP32!");
  digitalWrite(LED_PIN, HIGH);   // 点亮LED
  delay(1000);                   // 等待1秒
  digitalWrite(LED_PIN, LOW);    // 熄灭LED
  delay(1000);              // 等待1秒

}
