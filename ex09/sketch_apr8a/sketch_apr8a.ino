#include <WiFi.h>
#include <WebServer.h>

// ========== 配置区域 ==========
const char* ssid = "OPPO Find X7";      // 修改为你的WiFi名称
const char* password = "czf123456";  // 修改为你的WiFi密码

// 传感器配置
const int touchPin = T0;        // 触摸传感器引脚 GPIO4 (T0)
const int ledPin = 2;           // 状态指示灯

// 数据平滑处理
const int sampleCount = 10;     // 采样次数用于平滑
int samples[sampleCount];       // 采样缓冲区
int sampleIndex = 0;            // 当前采样位置

// Web服务器
WebServer server(80);

// 系统状态
unsigned long lastReadTime = 0;
const int readInterval = 50;    // 传感器读取间隔 50ms

// ========== HTML页面 (包含实时仪表盘) ==========
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 实时传感器仪表盘</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif;
            background: linear-gradient(135deg, #0f0c29 0%, #302b63 50%, #24243e 100%);
            min-height: 100vh;
            color: #fff;
            overflow-x: hidden;
        }
        
        /* 背景动画网格 */
        .bg-grid {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-image: 
                linear-gradient(rgba(0, 255, 255, 0.03) 1px, transparent 1px),
                linear-gradient(90deg, rgba(0, 255, 255, 0.03) 1px, transparent 1px);
            background-size: 50px 50px;
            pointer-events: none;
            z-index: 0;
        }
        
        .container {
            position: relative;
            z-index: 1;
            max-width: 900px;
            margin: 0 auto;
            padding: 40px 20px;
        }
        
        header {
            text-align: center;
            margin-bottom: 40px;
        }
        
        h1 {
            font-size: 2.5rem;
            font-weight: 300;
            letter-spacing: 3px;
            text-transform: uppercase;
            background: linear-gradient(90deg, #00d4ff, #7b2cbf);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 10px;
        }
        
        .subtitle {
            color: #888;
            font-size: 1rem;
            letter-spacing: 2px;
        }
        
        /* 主仪表盘区域 */
        .dashboard {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 30px;
            padding: 50px;
            box-shadow: 0 25px 50px rgba(0, 0, 0, 0.5);
        }
        
        /* 数值显示 - 中央大数字 */
        .value-display {
            text-align: center;
            margin: 40px 0;
            position: relative;
        }
        
        .main-value {
            font-size: 8rem;
            font-weight: 100;
            font-family: 'Courier New', monospace;
            color: #00d4ff;
            text-shadow: 0 0 30px rgba(0, 212, 255, 0.5);
            transition: all 0.3s ease;
            display: inline-block;
            position: relative;
        }
        
        .main-value.touching {
            color: #ff006e;
            text-shadow: 0 0 50px rgba(255, 0, 110, 0.8);
            transform: scale(1.05);
        }
        
        .value-label {
            font-size: 1.2rem;
            color: #888;
            margin-top: 15px;
            letter-spacing: 3px;
            text-transform: uppercase;
        }
        
        .unit {
            font-size: 2rem;
            color: #666;
            margin-left: 10px;
        }
        
        /* 状态指示器 */
        .status-bar {
            display: flex;
            justify-content: center;
            align-items: center;
            gap: 30px;
            margin: 30px 0;
            flex-wrap: wrap;
        }
        
        .status-item {
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 12px 24px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 25px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .status-dot {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #333;
            box-shadow: 0 0 10px currentColor;
            transition: all 0.3s;
        }
        
        .status-dot.active {
            background: #00ff88;
            box-shadow: 0 0 20px #00ff88;
            animation: pulse 1s infinite;
        }
        
        .status-dot.touch {
            background: #ff006e;
            box-shadow: 0 0 20px #ff006e;
        }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        /* 可视化图表区域 */
        .chart-container {
            margin-top: 40px;
            background: rgba(0, 0, 0, 0.3);
            border-radius: 20px;
            padding: 20px;
            position: relative;
            height: 250px;
        }
        
        .chart-title {
            position: absolute;
            top: 15px;
            left: 20px;
            font-size: 0.9rem;
            color: #666;
            text-transform: uppercase;
            letter-spacing: 2px;
        }
        
        #waveformCanvas {
            width: 100%;
            height: 100%;
        }
        
        /* 统计信息 */
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 20px;
            margin-top: 30px;
        }
        
        .stat-card {
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 20px;
            text-align: center;
            transition: all 0.3s;
        }
        
        .stat-card:hover {
            background: rgba(255, 255, 255, 0.08);
            transform: translateY(-5px);
        }
        
        .stat-label {
            font-size: 0.8rem;
            color: #888;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 8px;
        }
        
        .stat-value {
            font-size: 1.5rem;
            font-weight: 600;
            color: #fff;
            font-family: 'Courier New', monospace;
        }
        
        /* 连接状态 */
        .connection-status {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 10px 20px;
            border-radius: 20px;
            font-size: 0.85rem;
            display: flex;
            align-items: center;
            gap: 8px;
            transition: all 0.3s;
            z-index: 100;
        }
        
        .connection-status.online {
            background: rgba(0, 255, 136, 0.2);
            border: 1px solid #00ff88;
            color: #00ff88;
        }
        
        .connection-status.offline {
            background: rgba(255, 0, 110, 0.2);
            border: 1px solid #ff006e;
            color: #ff006e;
        }
        
        /* 响应式 */
        @media (max-width: 600px) {
            .main-value {
                font-size: 5rem;
            }
            h1 {
                font-size: 1.8rem;
            }
        }
        
        /* 加载动画 */
        .loading {
            display: inline-block;
            width: 20px;
            height: 20px;
            border: 3px solid rgba(255,255,255,.3);
            border-radius: 50%;
            border-top-color: #00d4ff;
            animation: spin 1s ease-in-out infinite;
        }
        
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="bg-grid"></div>
    
    <div class="connection-status online" id="connStatus">
        <span class="status-dot active"></span>
        <span id="connText">实时连接中</span>
    </div>
    
    <div class="container">
        <header>
            <h1>🔬 实时传感器监控</h1>
            <p class="subtitle">ESP32 Touch Sensor Data Acquisition System</p>
        </header>
        
        <div class="dashboard">
            <!-- 主数值显示 -->
            <div class="value-display">
                <div class="main-value" id="mainValue">
                    <span id="valueNumber">----</span>
                    <span class="unit" id="valueUnit"></span>
                </div>
                <div class="value-label">触摸传感器原始值</div>
            </div>
            
            <!-- 状态栏 -->
            <div class="status-bar">
                <div class="status-item">
                    <div class="status-dot active" id="acqDot"></div>
                    <span>数据采集</span>
                </div>
                <div class="status-item">
                    <div class="status-dot" id="touchDot"></div>
                    <span>触摸检测</span>
                </div>
                <div class="status-item">
                    <span style="color: #00d4ff;">📡</span>
                    <span id="updateRate">0 Hz</span>
                </div>
            </div>
            
            <!-- 实时波形图 -->
            <div class="chart-container">
                <div class="chart-title">实时波形</div>
                <canvas id="waveformCanvas"></canvas>
            </div>
            
            <!-- 统计信息 -->
            <div class="stats-grid">
                <div class="stat-card">
                    <div class="stat-label">最小值</div>
                    <div class="stat-value" id="minValue">--</div>
                </div>
                <div class="stat-card">
                    <div class="stat-label">最大值</div>
                    <div class="stat-value" id="maxValue">--</div>
                </div>
                <div class="stat-card">
                    <div class="stat-label">平均值</div>
                    <div class="stat-value" id="avgValue">--</div>
                </div>
                <div class="stat-card">
                    <div class="stat-label">采样次数</div>
                    <div class="stat-value" id="sampleCount">0</div>
                </div>
            </div>
        </div>
    </div>

    <script>
        // 数据历史记录（用于波形显示）
        const maxDataPoints = 200;
        let dataHistory = new Array(maxDataPoints).fill(0);
        let timeLabels = new Array(maxDataPoints).fill('');
        
        // 统计信息
        let stats = {
            min: Infinity,
            max: -Infinity,
            sum: 0,
            count: 0,
            values: []
        };
        
        // 更新频率计算
        let lastUpdateTime = Date.now();
        let updateCount = 0;
        let currentHz = 0;
        
        // Canvas设置
        const canvas = document.getElementById('waveformCanvas');
        const ctx = canvas.getContext('2d');
        
        // 设置Canvas实际分辨率
        function resizeCanvas() {
            const rect = canvas.getBoundingClientRect();
            canvas.width = rect.width;
            canvas.height = rect.height;
        }
        resizeCanvas();
        window.addEventListener('resize', resizeCanvas);
        
        // 绘制波形图
        function drawWaveform() {
            const width = canvas.width;
            const height = canvas.height;
            
            // 清空画布
            ctx.fillStyle = 'rgba(0, 0, 0, 0.1)';
            ctx.fillRect(0, 0, width, height);
            
            // 计算数据范围
            const minVal = Math.min(...dataHistory, 50);
            const maxVal = Math.max(...dataHistory, 100);
            const range = maxVal - minVal || 1;
            
            // 绘制网格线
            ctx.strokeStyle = 'rgba(0, 212, 255, 0.1)';
            ctx.lineWidth = 1;
            for (let i = 0; i < 5; i++) {
                const y = (height / 4) * i;
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(width, y);
                ctx.stroke();
            }
            
            // 绘制波形
            ctx.strokeStyle = '#00d4ff';
            ctx.lineWidth = 3;
            ctx.shadowColor = '#00d4ff';
            ctx.shadowBlur = 15;
            ctx.beginPath();
            
            for (let i = 0; i < dataHistory.length; i++) {
                const x = (width / (maxDataPoints - 1)) * i;
                const normalizedValue = (dataHistory[i] - minVal) / range;
                const y = height - (normalizedValue * height * 0.8 + height * 0.1);
                
                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            }
            ctx.stroke();
            ctx.shadowBlur = 0;
            
            // 绘制当前值指示线
            const currentY = height - ((dataHistory[dataHistory.length - 1] - minVal) / range * height * 0.8 + height * 0.1);
            ctx.strokeStyle = '#ff006e';
            ctx.lineWidth = 2;
            ctx.setLineDash([5, 5]);
            ctx.beginPath();
            ctx.moveTo(0, currentY);
            ctx.lineTo(width, currentY);
            ctx.stroke();
            ctx.setLineDash([]);
        }
        
        // 更新统计信息
        function updateStats(value) {
            stats.values.push(value);
            if (stats.values.length > 100) stats.values.shift();
            
            stats.min = Math.min(...stats.values);
            stats.max = Math.max(...stats.values);
            stats.sum = stats.values.reduce((a, b) => a + b, 0);
            stats.count++;
            
            document.getElementById('minValue').textContent = stats.min.toFixed(0);
            document.getElementById('maxValue').textContent = stats.max.toFixed(0);
            document.getElementById('avgValue').textContent = (stats.sum / stats.values.length).toFixed(1);
            document.getElementById('sampleCount').textContent = stats.count;
        }
        
        // 更新显示
        function updateDisplay(data) {
            const value = data.value;
            const isTouching = data.touch;
            
            // 更新主数值
            const valueEl = document.getElementById('valueNumber');
            const mainValueEl = document.getElementById('mainValue');
            
            // 数字动画效果
            valueEl.textContent = value;
            
            // 根据触摸状态改变样式
            if (isTouching) {
                mainValueEl.classList.add('touching');
                document.getElementById('touchDot').classList.add('touch');
            } else {
                mainValueEl.classList.remove('touching');
                document.getElementById('touchDot').classList.remove('touch');
            }
            
            // 更新数据历史
            dataHistory.push(value);
            dataHistory.shift();
            
            // 更新时间标签
            const now = new Date();
            timeLabels.push(now.toLocaleTimeString('zh-CN', {hour12: false, hour: '2-digit', minute:'2-digit', second:'2-digit'}));
            timeLabels.shift();
            
            // 更新统计
            updateStats(value);
            
            // 绘制波形
            drawWaveform();
            
            // 计算更新频率
            updateCount++;
            const nowTime = Date.now();
            if (nowTime - lastUpdateTime >= 1000) {
                currentHz = updateCount;
                document.getElementById('updateRate').textContent = currentHz + ' Hz';
                updateCount = 0;
                lastUpdateTime = nowTime;
            }
        }
        
        // 数据采集循环
        async function dataAcquisition() {
            const connStatus = document.getElementById('connStatus');
            const connText = document.getElementById('connText');
            const acqDot = document.getElementById('acqDot');
            
            try {
                const response = await fetch('/api/sensor', {
                    method: 'GET',
                    headers: { 'Accept': 'application/json' }
                });
                
                if (!response.ok) throw new Error('HTTP ' + response.status);
                
                const data = await response.json();
                
                // 更新连接状态
                connStatus.classList.remove('offline');
                connStatus.classList.add('online');
                connText.textContent = '实时连接中 (' + currentHz + 'Hz)';
                acqDot.classList.add('active');
                
                // 更新显示
                updateDisplay(data);
                
            } catch (error) {
                console.error('采集失败:', error);
                connStatus.classList.remove('online');
                connStatus.classList.add('offline');
                connText.textContent = '连接中断';
                acqDot.classList.remove('active');
            }
        }
        
        // 启动实时采集 (每100ms一次 = 10Hz)
        setInterval(dataAcquisition, 100);
        
        // 初始绘制
        drawWaveform();
        
        // 页面加载完成提示
        console.log('🚀 ESP32 实时传感器仪表盘已启动');
        console.log('📡 数据采集频率: 10Hz');
        console.log('📊 波形缓冲区: 200点 (20秒)');
    </script>
</body>
</html>
)rawliteral";

// ========== API处理函数 ==========

// 处理根路径 - 返回仪表盘页面
void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

// 处理传感器数据API - 返回JSON格式的实时数据
void handleSensorAPI() {
    // 读取当前触摸值（原始值，0-255范围，越小表示触摸越强）
    int rawValue = touchRead(touchPin);
    
    // 数据平滑处理（滑动平均）
    samples[sampleIndex] = rawValue;
    sampleIndex = (sampleIndex + 1) % sampleCount;
    
    // 计算平均值
    long sum = 0;
    for (int i = 0; i < sampleCount; i++) {
        sum += samples[i];
    }
    int smoothedValue = sum / sampleCount;
    
    // 判断触摸状态（阈值可根据实际情况调整）
    // 典型值：无触摸>50，轻触30-50，重触<30
    bool isTouching = (smoothedValue < 40);
    
    // 构建JSON响应
    String json = "{";
    json += "\"value\":" + String(smoothedValue) + ",";
    json += "\"raw\":" + String(rawValue) + ",";
    json += "\"touch\":" + String(isTouching ? "true" : "false") + ",";
    json += "\"timestamp\":" + String(millis()) + ",";
    json += "\"unit\":\"raw\"";
    json += "}";
    
    // 添加CORS头，允许跨域（可选）
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Content-Type", "application/json");
    server.send(200, "application/json", json);
    
    // 硬件端指示灯反馈（可选）
    if (isTouching) {
        digitalWrite(ledPin, HIGH);  // 触摸时LED亮
    } else {
        digitalWrite(ledPin, LOW);   // 无触摸时LED灭
    }
}

// 处理预检请求（CORS）
void handleCORS() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200, "text/plain", "");
}

// 处理404
void handleNotFound() {
    server.send(404, "application/json", "{\"error\":\"Not Found\"}");
}

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // 初始化GPIO
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    // 初始化采样缓冲区
    for (int i = 0; i < sampleCount; i++) {
        samples[i] = touchRead(touchPin);
    }
    
    Serial.println("\n\n========================================");
    Serial.println("    ESP32 实时传感器Web仪表盘");
    Serial.println("========================================");
    Serial.println("📡 功能: 数据采集 + 实时上报");
    Serial.println("📊 接口: /api/sensor (JSON API)");
    Serial.println("🌐 页面: / (数据可视化仪表盘)");
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
    server.on("/api/sensor", HTTP_GET, handleSensorAPI);
    server.on("/api/sensor", HTTP_OPTIONS, handleCORS);  // CORS预检
    server.onNotFound(handleNotFound);
    
    // 启动服务器
    server.begin();
    Serial.println("✓ HTTP服务器已启动 (端口: 80)");
    Serial.println("✓ API端点: http://" + WiFi.localIP().toString() + "/api/sensor");
    Serial.println("✓ 等待客户端连接...\n");
}

// ========== 主循环 ==========
void loop() {
    // 处理Web客户端请求
    server.handleClient();
    
    // 可选：后台持续采样以提高数据质量
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime >= readInterval) {
        lastReadTime = currentTime;
        
        // 后台采样更新缓冲区
        samples[sampleIndex] = touchRead(touchPin);
        sampleIndex = (sampleIndex + 1) % sampleCount;
    }
    
    delay(1);  // 短暂延时防止看门狗复位
}