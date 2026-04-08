#include <WiFi.h>
#include <WebServer.h>

// ========== 配置区域 ==========
const char* ssid = "OPPO Find X7";      // 修改为你的WiFi名称
const char* password = "czf123456";  // 修改为你的WiFi密码

// LED引脚定义（ESP32通常使用GPIO2作为板载LED）
const int ledPin = 2;

// PWM配置
const int pwmChannel = 0;      // PWM通道0-15可选
const int pwmFrequency = 5000; // PWM频率5kHz
const int pwmResolution = 8;   // 分辨率8位 (0-255)

// 创建Web服务器实例，端口80
WebServer server(80);

// 当前亮度值
int currentBrightness = 0;

// ========== HTML页面 ==========
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 无极调光器</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            width: 100%;
            max-width: 400px;
            text-align: center;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 24px;
        }
        
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        
        .brightness-display {
            font-size: 48px;
            font-weight: bold;
            color: #2a5298;
            margin: 20px 0;
            transition: all 0.3s ease;
        }
        
        .slider-container {
            margin: 30px 0;
            position: relative;
        }
        
        input[type=range] {
            -webkit-appearance: none;
            width: 100%;
            height: 12px;
            border-radius: 6px;
            background: linear-gradient(to right, #ddd 0%, #2a5298 0%);
            outline: none;
            transition: all 0.3s;
        }
        
        input[type=range]::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 28px;
            height: 28px;
            border-radius: 50%;
            background: #2a5298;
            cursor: pointer;
            box-shadow: 0 4px 10px rgba(42, 82, 152, 0.4);
            transition: all 0.2s;
        }
        
        input[type=range]::-webkit-slider-thumb:hover {
            transform: scale(1.2);
            box-shadow: 0 6px 15px rgba(42, 82, 152, 0.6);
        }
        
        input[type=range]::-moz-range-thumb {
            width: 28px;
            height: 28px;
            border-radius: 50%;
            background: #2a5298;
            cursor: pointer;
            border: none;
            box-shadow: 0 4px 10px rgba(42, 82, 152, 0.4);
        }
        
        .labels {
            display: flex;
            justify-content: space-between;
            margin-top: 10px;
            color: #666;
            font-size: 12px;
        }
        
        .status {
            margin-top: 20px;
            padding: 12px;
            border-radius: 8px;
            background: #f0f4f8;
            color: #333;
            font-size: 14px;
            transition: all 0.3s;
        }
        
        .status.active {
            background: #d4edda;
            color: #155724;
        }
        
        .led-indicator {
            width: 60px;
            height: 60px;
            border-radius: 50%;
            margin: 20px auto;
            background: #333;
            box-shadow: inset 0 2px 10px rgba(0,0,0,0.3);
            transition: all 0.1s ease;
            position: relative;
        }
        
        .led-indicator::after {
            content: '';
            position: absolute;
            top: 10%;
            left: 20%;
            width: 30%;
            height: 30%;
            background: rgba(255,255,255,0.4);
            border-radius: 50%;
        }
        
        .icon {
            font-size: 40px;
            margin-bottom: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="icon">💡</div>
        <h1>ESP32 无极调光器</h1>
        <p class="subtitle">拖动滑块调节LED亮度</p>
        
        <div class="led-indicator" id="ledIndicator"></div>
        
        <div class="brightness-display">
            <span id="brightnessValue">0</span>%
        </div>
        
        <div class="slider-container">
            <input type="range" id="brightnessSlider" min="0" max="255" value="0">
            <div class="labels">
                <span>关闭</span>
                <span>最亮</span>
            </div>
        </div>
        
        <div class="status" id="status">准备就绪</div>
    </div>

    <script>
        const slider = document.getElementById('brightnessSlider');
        const brightnessValue = document.getElementById('brightnessValue');
        const statusDiv = document.getElementById('status');
        const ledIndicator = document.getElementById('ledIndicator');
        let lastSentValue = -1;
        
        // 更新滑块背景渐变
        function updateSliderBackground(value) {
            const percentage = (value / 255) * 100;
            slider.style.background = `linear-gradient(to right, #2a5298 0%, #2a5298 ${percentage}%, #ddd ${percentage}%, #ddd 100%)`;
        }
        
        // 更新LED指示器
        function updateLedIndicator(value) {
            const percentage = value / 255;
            const intensity = Math.floor(percentage * 255);
            ledIndicator.style.background = `rgb(${intensity}, ${intensity}, ${Math.floor(intensity * 0.8)})`;
            ledIndicator.style.boxShadow = `0 0 ${20 + percentage * 30}px rgba(${intensity}, ${intensity}, ${Math.floor(intensity * 0.8)}, ${0.2 + percentage * 0.8})`;
        }
        
        // 发送亮度值到ESP32
        async function sendBrightness(value) {
            // 防抖：如果值与上次发送的相同，则不发送
            if (value === lastSentValue) return;
            
            try {
                statusDiv.textContent = '发送中...';
                statusDiv.classList.remove('active');
                
                // 使用fetch发送GET请求
                const response = await fetch(`/set?brightness=${value}`, {
                    method: 'GET',
                    // 添加超时处理
                    signal: AbortSignal.timeout(3000)
                });
                
                if (response.ok) {
                    lastSentValue = value;
                    statusDiv.textContent = `亮度已设置为 ${Math.round((value/255)*100)}%`;
                    statusDiv.classList.add('active');
                } else {
                    statusDiv.textContent = '发送失败';
                }
            } catch (error) {
                statusDiv.textContent = '连接错误';
                console.error('Error:', error);
            }
        }
        
        // 防抖函数，避免频繁发送请求
        function debounce(func, wait) {
            let timeout;
            return function executedFunction(...args) {
                const later = () => {
                    clearTimeout(timeout);
                    func(...args);
                };
                clearTimeout(timeout);
                timeout = setTimeout(later, wait);
            };
        }
        
        // 创建防抖版本的发送函数（100ms延迟）
        const debouncedSend = debounce(sendBrightness, 100);
        
        // 监听滑块变化
        slider.addEventListener('input', function(e) {
            const value = parseInt(e.target.value);
            const percentage = Math.round((value / 255) * 100);
            
            // 更新显示
            brightnessValue.textContent = percentage;
            updateSliderBackground(value);
            updateLedIndicator(value);
            
            // 发送数据到ESP32
            debouncedSend(value);
        });
        
        // 初始化
        updateSliderBackground(0);
        updateLedIndicator(0);
        
        // 页面加载时获取当前状态
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                if (data.brightness !== undefined) {
                    slider.value = data.brightness;
                    brightnessValue.textContent = Math.round((data.brightness / 255) * 100);
                    updateSliderBackground(data.brightness);
                    updateLedIndicator(data.brightness);
                }
            })
            .catch(() => {
                console.log('无法获取初始状态');
            });
    </script>
</body>
</html>
)rawliteral";

// ========== 处理函数 ==========

// 处理根路径请求，返回HTML页面
void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

// 处理设置亮度请求 /set?brightness=xxx
void handleSetBrightness() {
    if (server.hasArg("brightness")) {
        String brightnessStr = server.arg("brightness");
        int brightness = brightnessStr.toInt();
        
        // 限制范围0-255
        brightness = constrain(brightness, 0, 255);
        
        // 设置PWM占空比
        ledcWrite(pwmChannel, brightness);
        currentBrightness = brightness;
        
        // 返回JSON响应
        String json = "{\"success\":true,\"brightness\":" + String(brightness) + "}";
        server.send(200, "application/json", json);
        
        Serial.printf("亮度已设置为: %d (%.1f%%)\n", brightness, (brightness/255.0)*100);
    } else {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"缺少brightness参数\"}");
    }
}

// 处理获取状态请求 /status
void handleGetStatus() {
    String json = "{\"brightness\":" + String(currentBrightness) + "}";
    server.send(200, "application/json", json);
}

// 处理404错误
void handleNotFound() {
    server.send(404, "text/plain", "404: 页面未找到");
}

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // 配置PWM通道
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    // 将PWM通道绑定到GPIO引脚
    ledcAttachPin(ledPin, pwmChannel);
    // 初始关闭LED
    ledcWrite(pwmChannel, 0);
    
    Serial.println("\n\n=== ESP32 无极调光器启动 ===");
    
    // 连接WiFi
    WiFi.begin(ssid, password);
    Serial.print("正在连接WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi连接成功!");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
    
    // 设置HTTP路由
    server.on("/", HTTP_GET, handleRoot);
    server.on("/set", HTTP_GET, handleSetBrightness);
    server.on("/status", HTTP_GET, handleGetStatus);
    server.onNotFound(handleNotFound);
    
    // 启动服务器
    server.begin();
    Serial.println("HTTP服务器已启动");
    Serial.println("请在浏览器中访问上述IP地址");
}

void loop() {
    // 处理客户端请求
    server.handleClient();
}