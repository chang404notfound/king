#include <WiFi.h>
#include <WebServer.h>

// ========== 配置区域 ==========
const char* ssid = "OPPO Find X7";      // 修改为你的WiFi名称
const char* password = "czf123456";  // 修改为你的WiFi密码

// 引脚定义
const int ledPin = 2;           // LED引脚（GPIO2为板载LED）
const int touchPin = T0;        // 触摸传感器引脚（GPIO4，对应T0）

// 系统状态枚举
enum SecurityStatus {
    DISARMED = 0,   // 撤防状态
    ARMED = 1,      // 布防状态
    ALARMING = 2    // 报警状态（锁定）
};

// 全局变量
volatile SecurityStatus systemStatus = DISARMED;  // 当前系统状态
bool ledState = false;          // LED当前亮灭状态
unsigned long lastBlinkTime = 0; // 上次闪烁时间
const int blinkInterval = 100;   // 报警闪烁间隔（毫秒，100ms = 10Hz高频）

// 触摸检测阈值（ESP32触摸值通常范围：无触摸>50，有触摸<20）
const int touchThreshold = 30;   
bool touchDetected = false;      // 触摸检测标志

// 创建Web服务器
WebServer server(80);

// ========== HTML页面 ==========
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 安防报警系统</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
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
            box-shadow: 0 20px 60px rgba(0,0,0,0.5);
            width: 100%;
            max-width: 450px;
            text-align: center;
        }
        
        h1 {
            color: #1a1a2e;
            margin-bottom: 10px;
            font-size: 26px;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
        }
        
        .shield-icon {
            font-size: 32px;
        }
        
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        
        /* 状态显示区域 */
        .status-panel {
            background: #f8f9fa;
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 30px;
            border: 3px solid #ddd;
            transition: all 0.3s ease;
        }
        
        .status-panel.disarmed {
            border-color: #28a745;
            background: linear-gradient(135deg, #d4edda 0%, #f8f9fa 100%);
        }
        
        .status-panel.armed {
            border-color: #ffc107;
            background: linear-gradient(135deg, #fff3cd 0%, #f8f9fa 100%);
        }
        
        .status-panel.alarming {
            border-color: #dc3545;
            background: linear-gradient(135deg, #f8d7da 0%, #f8f9fa 100%);
            animation: pulse 0.5s infinite alternate;
        }
        
        @keyframes pulse {
            from { box-shadow: 0 0 0 0 rgba(220, 53, 69, 0.4); }
            to { box-shadow: 0 0 0 15px rgba(220, 53, 69, 0); }
        }
        
        .status-icon {
            font-size: 60px;
            margin-bottom: 15px;
            transition: all 0.3s;
        }
        
        .status-text {
            font-size: 24px;
            font-weight: bold;
            margin-bottom: 5px;
        }
        
        .status-desc {
            font-size: 14px;
            color: #666;
        }
        
        /* LED指示器 */
        .led-indicator {
            width: 80px;
            height: 80px;
            border-radius: 50%;
            margin: 20px auto;
            background: #333;
            box-shadow: inset 0 4px 15px rgba(0,0,0,0.4), 0 0 0 4px #ddd;
            transition: all 0.1s;
            position: relative;
        }
        
        .led-indicator.on {
            background: #ff3333;
            box-shadow: 0 0 30px #ff3333, inset 0 4px 15px rgba(255,255,255,0.3);
        }
        
        .led-indicator.alarming {
            animation: blink 0.1s infinite;
        }
        
        @keyframes blink {
            0%, 50% { 
                background: #ff3333; 
                box-shadow: 0 0 50px #ff3333, 0 0 100px #ff6666;
            }
            51%, 100% { 
                background: #330000; 
                box-shadow: inset 0 4px 15px rgba(0,0,0,0.4);
            }
        }
        
        /* 控制按钮 */
        .controls {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 20px;
        }
        
        button {
            padding: 18px 24px;
            border: none;
            border-radius: 12px;
            font-size: 18px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        
        button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        
        button:hover:not(:disabled) {
            transform: translateY(-2px);
            box-shadow: 0 8px 20px rgba(0,0,0,0.2);
        }
        
        button:active:not(:disabled) {
            transform: translateY(0);
        }
        
        .btn-arm {
            background: linear-gradient(135deg, #ffc107 0%, #ff9800 100%);
            color: #333;
        }
        
        .btn-disarm {
            background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
            color: white;
        }
        
        /* 日志区域 */
        .log-panel {
            margin-top: 25px;
            background: #2d2d2d;
            border-radius: 10px;
            padding: 15px;
            text-align: left;
            max-height: 150px;
            overflow-y: auto;
        }
        
        .log-title {
            color: #aaa;
            font-size: 12px;
            margin-bottom: 10px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .log-entry {
            color: #0f0;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            margin: 5px 0;
            padding: 3px 0;
            border-bottom: 1px solid #444;
        }
        
        .log-entry.alarm {
            color: #ff3333;
            font-weight: bold;
        }
        
        .log-entry.system {
            color: #0ff;
        }
        
        /* 触摸检测提示 */
        .touch-sensor {
            margin-top: 15px;
            padding: 10px;
            background: #e9ecef;
            border-radius: 8px;
            font-size: 13px;
            color: #666;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        
        .touch-active {
            color: #dc3545;
            font-weight: bold;
            animation: shake 0.5s infinite;
        }
        
        @keyframes shake {
            0%, 100% { transform: translateX(0); }
            25% { transform: translateX(-3px); }
            75% { transform: translateX(3px); }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1><span class="shield-icon">🛡️</span> ESP32 安防系统</h1>
        <p class="subtitle">物联网智能安防报警器</p>
        
        <div class="status-panel disarmed" id="statusPanel">
            <div class="status-icon" id="statusIcon">🔓</div>
            <div class="led-indicator" id="ledIndicator"></div>
            <div class="status-text" id="statusText">已撤防</div>
            <div class="status-desc" id="statusDesc">系统未启动，触摸传感器无效</div>
        </div>
        
        <div class="controls">
            <button class="btn-arm" id="btnArm" onclick="armSystem()">
                🔒 布防
            </button>
            <button class="btn-disarm" id="btnDisarm" onclick="disarmSystem()" disabled>
                🔓 撤防
            </button>
        </div>
        
        <div class="touch-sensor" id="touchSensor">
            <span>👆</span> 触摸传感器状态: <span id="touchStatus">未检测到触摸</span>
        </div>
        
        <div class="log-panel" id="logPanel">
            <div class="log-title">📋 系统日志</div>
            <div id="logContent">
                <div class="log-entry system">系统初始化完成...</div>
            </div>
        </div>
    </div>

    <script>
        let currentStatus = 0; // 0:撤防, 1:布防, 2:报警
        let statusCheckInterval;
        let logCount = 0;
        
        // 添加日志
        function addLog(message, type = 'normal') {
            const logContent = document.getElementById('logContent');
            const entry = document.createElement('div');
            entry.className = `log-entry ${type}`;
            const time = new Date().toLocaleTimeString('zh-CN');
            entry.textContent = `[${time}] ${message}`;
            logContent.insertBefore(entry, logContent.firstChild);
            logCount++;
            
            // 限制日志数量
            if (logCount > 20) {
                logContent.removeChild(logContent.lastChild);
            }
        }
        
        // 更新UI状态
        function updateUI(status, touchActive, ledOn) {
            const panel = document.getElementById('statusPanel');
            const icon = document.getElementById('statusIcon');
            const text = document.getElementById('statusText');
            const desc = document.getElementById('statusDesc');
            const led = document.getElementById('ledIndicator');
            const btnArm = document.getElementById('btnArm');
            const btnDisarm = document.getElementById('btnDisarm');
            const touchStatus = document.getElementById('touchStatus');
            const touchSensor = document.getElementById('touchSensor');
            
            currentStatus = status;
            
            // 重置样式
            panel.className = 'status-panel';
            led.className = 'led-indicator';
            
            // 触摸状态显示
            if (touchActive) {
                touchStatus.textContent = '⚠️ 检测到触摸！';
                touchStatus.className = 'touch-active';
                touchSensor.style.background = '#f8d7da';
            } else {
                touchStatus.textContent = '未检测到触摸';
                touchStatus.className = '';
                touchSensor.style.background = '#e9ecef';
            }
            
            // 根据系统状态更新
            switch(status) {
                case 0: // 撤防
                    panel.classList.add('disarmed');
                    icon.textContent = '🔓';
                    text.textContent = '已撤防';
                    desc.textContent = '系统安全，触摸传感器无效';
                    btnArm.disabled = false;
                    btnDisarm.disabled = true;
                    if (ledOn) led.classList.add('on');
                    break;
                    
                case 1: // 布防
                    panel.classList.add('armed');
                    icon.textContent = '🔒';
                    text.textContent = '已布防';
                    desc.textContent = '监控中... 检测到触摸将触发报警';
                    btnArm.disabled = true;
                    btnDisarm.disabled = false;
                    if (ledOn) led.classList.add('on');
                    break;
                    
                case 2: // 报警
                    panel.classList.add('alarming');
                    icon.textContent = '🚨';
                    text.textContent = '报警中！';
                    desc.textContent = '入侵检测！点击撤防解除警报';
                    btnArm.disabled = true;
                    btnDisarm.disabled = false;
                    led.classList.add('alarming');
                    break;
            }
        }
        
        // 布防
        async function armSystem() {
            try {
                const response = await fetch('/arm', { method: 'POST' });
                const data = await response.json();
                
                if (data.success) {
                    addLog('🔒 系统已布防', 'system');
                    updateUI(1, false, false);
                }
            } catch (error) {
                addLog('❌ 布防失败: ' + error.message, 'alarm');
            }
        }
        
        // 撤防
        async function disarmSystem() {
            try {
                const response = await fetch('/disarm', { method: 'POST' });
                const data = await response.json();
                
                if (data.success) {
                    addLog('🔓 系统已撤防', 'system');
                    updateUI(0, false, false);
                }
            } catch (error) {
                addLog('❌ 撤防失败: ' + error.message, 'alarm');
            }
        }
        
        // 获取状态
        async function checkStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                
                // 如果状态变化或正在报警，更新UI
                if (data.status !== currentStatus || data.status === 2) {
                    updateUI(data.status, data.touch, data.led);
                    
                    // 如果刚进入报警状态，记录日志
                    if (data.status === 2 && currentStatus !== 2) {
                        addLog('🚨 报警触发！检测到入侵！', 'alarm');
                    }
                }
            } catch (error) {
                console.error('状态检查失败:', error);
            }
        }
        
        // 启动状态轮询（每200ms检查一次，确保报警实时性）
        statusCheckInterval = setInterval(checkStatus, 200);
        
        // 初始检查
        checkStatus();
        addLog('🌐 网页端已连接', 'system');
    </script>
</body>
</html>
)rawliteral";

// ========== 处理函数 ==========

// 处理根路径
void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

// 处理布防请求
void handleArm() {
    if (systemStatus == DISARMED) {
        systemStatus = ARMED;
        ledState = false;
        digitalWrite(ledPin, LOW);
        Serial.println("[系统] 🔒 已布防 - 监控启动");
        server.send(200, "application/json", "{\"success\":true,\"status\":1}");
    } else {
        server.send(200, "application/json", "{\"success\":false,\"error\":\"系统已在运行中\"}");
    }
}

// 处理撤防请求
void handleDisarm() {
    systemStatus = DISARMED;
    ledState = false;
    digitalWrite(ledPin, LOW);
    Serial.println("[系统] 🔓 已撤防 - 系统重置");
    server.send(200, "application/json", "{\"success\":true,\"status\":0}");
}

// 处理状态查询
void handleStatus() {
    // 读取触摸状态（但不触发状态转换，只在loop中处理）
    int touchValue = touchRead(touchPin);
    bool isTouched = (touchValue < touchThreshold);
    
    // 构建JSON响应
    String json = "{";
    json += "\"status\":" + String(systemStatus) + ",";
    json += "\"touch\":" + String(isTouched ? "true" : "false") + ",";
    json += "\"led\":" + String(ledState ? "true" : "false") + ",";
    json += "\"touchValue\":" + String(touchValue);
    json += "}";
    
    server.send(200, "application/json", json);
}

// 处理404
void handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // 配置GPIO
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    // 触摸传感器无需pinMode，使用touchRead()直接读取
    
    Serial.println("\n\n========================================");
    Serial.println("    ESP32 物联网安防报警器");
    Serial.println("========================================");
    Serial.println("触摸引脚: GPIO4 (T0)");
    Serial.println("LED引脚: GPIO2");
    Serial.println("========================================\n");
    
    // 连接WiFi
    WiFi.begin(ssid, password);
    Serial.print("正在连接WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\n✓ WiFi连接成功!");
    Serial.print("✓ IP地址: http://");
    Serial.println(WiFi.localIP());
    Serial.println("========================================\n");
    
    // 设置HTTP路由
    server.on("/", HTTP_GET, handleRoot);
    server.on("/arm", HTTP_POST, handleArm);
    server.on("/disarm", HTTP_POST, handleDisarm);
    server.on("/status", HTTP_GET, handleStatus);
    server.onNotFound(handleNotFound);
    
    // 启动服务器
    server.begin();
    Serial.println("✓ HTTP服务器已启动");
    Serial.println("✓ 等待客户端连接...\n");
}

// ========== 主循环 ==========
void loop() {
    // 处理Web客户端请求
    server.handleClient();
    
    // 读取触摸传感器值
    int touchValue = touchRead(touchPin);
    bool isCurrentlyTouched = (touchValue < touchThreshold);
    
    // 状态机处理
    switch (systemStatus) {
        case DISARMED:
            // 撤防状态：LED保持熄灭，触摸无效
            if (ledState) {
                ledState = false;
                digitalWrite(ledPin, LOW);
            }
            break;
            
        case ARMED:
            // 布防状态：监控触摸
            if (isCurrentlyTouched) {
                // 检测到触摸，切换到报警状态！
                systemStatus = ALARMING;
                touchDetected = true;
                lastBlinkTime = millis();
                Serial.println("[警报] 🚨 入侵检测！触摸引脚触发！");
                Serial.println("[警报] 触摸值: " + String(touchValue) + " (阈值: " + String(touchThreshold) + ")");
            }
            break;
            
        case ALARMING:
            // 报警状态：高频闪烁LED（锁定状态，只能通过撤防解除）
            unsigned long currentTime = millis();
            
            // 100ms间隔闪烁（10Hz高频）
            if (currentTime - lastBlinkTime >= blinkInterval) {
                lastBlinkTime = currentTime;
                ledState = !ledState;
                digitalWrite(ledPin, ledState ? HIGH : LOW);
                
                // 串口输出报警信息
                if (ledState) {
                    Serial.println("[警报] 💡 LED ON  | 系统锁定 - 等待撤防");
                }
            }
            // 注意：在此状态下触摸检测仍然进行，但不影响报警状态
            // 报警状态只能通过Web端的撤防按钮解除
            break;
    }
    
    // 调试输出（每2秒输出一次状态，避免刷屏）
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 2000) {
        lastDebugTime = millis();
        String statusStr;
        switch(systemStatus) {
            case DISARMED: statusStr = "撤防"; break;
            case ARMED: statusStr = "布防"; break;
            case ALARMING: statusStr = "报警!!!"; break;
        }
        Serial.println("[调试] 状态: " + statusStr + " | 触摸值: " + String(touchValue) + 
                      " | 触摸检测: " + String(isCurrentlyTouched ? "是" : "否"));
    }
    
    delay(10); // 短暂延时，防止看门狗复位
}