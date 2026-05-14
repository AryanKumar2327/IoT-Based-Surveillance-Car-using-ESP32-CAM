#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_http_server.h"

// ===================== WIFI =====================
const char* ssid = "ESP32_CAR";
const char* password = "12345678";

WebServer server(80);
httpd_handle_t stream_httpd = NULL;

// ===================== MOTOR PINS =====================
#define IN1 12
#define IN2 13
#define IN3 14
#define IN4 15

// ===================== PWM CONFIGURATION =====================
#define PWM_CHANNEL_LEFT 0
#define PWM_CHANNEL_RIGHT 1
#define PWM_FREQUENCY 5000
#define PWM_RESOLUTION 8
#define MAX_SPEED 255

// ===================== FLASH LED =====================
#define FLASH_LED 4

// ===================== CAMERA CONFIG (AI THINKER) =====================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===================== FAILSAFE =====================
unsigned long lastCommandTime = 0;
const unsigned long COMMAND_TIMEOUT = 500;
String currentCommand = "STOP";

// ===================== MOTOR TUNING =====================
// If one side moves opposite, change false to true for that side only
bool LEFT_MOTOR_INVERT  = false;
bool RIGHT_MOTOR_INVERT = false;

// ===================== STREAM =====================
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
static const char* STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
static const unsigned long CAMERA_TIMEOUT = 1000; // 1 second timeout for camera capture

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[128]; // Increased from 64 to 128 for safety

  res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    Serial.println("Failed to set response type");
    return res;
  }

  while (true) {
    // Check for client disconnect
    if (!httpd_req_is_connected(req)) {
      Serial.println("Client disconnected from stream");
      break;
    }

    unsigned long captureStart = millis();
    fb = esp_camera_fb_get();
    
    // Timeout check for camera capture
    if (!fb) {
      if (millis() - captureStart > CAMERA_TIMEOUT) {
        Serial.println("Camera capture timeout");
        res = ESP_FAIL;
        break; // Exit on persistent camera failure
      }
      continue; // Retry capture
    }

    // Validate frame size
    if (fb->len == 0) {
      Serial.println("Invalid frame size");
      esp_camera_fb_return(fb);
      fb = NULL;
      continue;
    }

    // Send frame boundary
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res != ESP_OK) {
      Serial.println("Failed to send boundary");
      esp_camera_fb_return(fb);
      fb = NULL;
      break;
    }

    // Send frame header
    size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, fb->len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res != ESP_OK) {
      Serial.println("Failed to send header");
      esp_camera_fb_return(fb);
      fb = NULL;
      break;
    }

    // Send frame data
    res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    fb = NULL;

    if (res != ESP_OK) {
      Serial.println("Failed to send frame data");
      break;
    }
  }

  // Cleanup on exit
  if (fb) {
    esp_camera_fb_return(fb);
  }

  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.max_open_sockets = 2; // Limit concurrent connections to save memory

  httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("Camera stream started on port 81");
  } else {
    Serial.println("ERROR: Camera stream failed to start");
  }
}

// ===================== MOTOR CONTROL (WITH PWM) =====================
void initMotorPWM() {
  // Setup PWM for left motor
  ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(IN1, PWM_CHANNEL_LEFT);

  // Setup PWM for right motor
  ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(IN3, PWM_CHANNEL_RIGHT);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_LEFT, 0);
  ledcWrite(PWM_CHANNEL_RIGHT, 0);
}

void setLeftMotor(int dir, int speed = MAX_SPEED) {
  // Clamp speed value
  speed = constrain(speed, 0, MAX_SPEED);

  if (LEFT_MOTOR_INVERT) dir = -dir;

  if (dir == 1) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(PWM_CHANNEL_LEFT, speed);
  } else if (dir == -1) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(PWM_CHANNEL_LEFT, speed);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(PWM_CHANNEL_LEFT, 0);
  }
}

void setRightMotor(int dir, int speed = MAX_SPEED) {
  // Clamp speed value
  speed = constrain(speed, 0, MAX_SPEED);

  if (RIGHT_MOTOR_INVERT) dir = -dir;

  if (dir == 1) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_RIGHT, speed);
  } else if (dir == -1) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(PWM_CHANNEL_RIGHT, speed);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_RIGHT, 0);
  }
}

void moveCar(int leftDir, int rightDir, String cmd, int speed = MAX_SPEED) {
  setLeftMotor(leftDir, speed);
  setRightMotor(rightDir, speed);
  currentCommand = cmd;
  lastCommandTime = millis();
  Serial.println("CMD: " + cmd + " (Speed: " + String(speed) + ")");
}

void forward(int speed = MAX_SPEED)   { moveCar( 1,  1, "FORWARD", speed); }
void back(int speed = MAX_SPEED)      { moveCar(-1, -1, "BACK", speed); }
void turnLeft(int speed = MAX_SPEED)  { moveCar( 0,  1, "LEFT", speed); }
void turnRight(int speed = MAX_SPEED) { moveCar( 1,  0, "RIGHT", speed); }

void stopCar() {
  stopMotors();
  currentCommand = "STOP";
  lastCommandTime = millis();
  Serial.println("CMD: STOP");
}

// ===================== WEB PAGE =====================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>ESP32 CAM Car</title>
  <style>
    * { box-sizing: border-box; touch-action: manipulation; }
    body {
      margin: 0;
      padding: 12px;
      background: #111;
      color: #fff;
      font-family: Arial, sans-serif;
      text-align: center;
      user-select: none;
    }
    h2 { margin: 8px 0 12px; font-size: 1.3rem; }
    #stream {
      width: 95%;
      max-width: 420px;
      border-radius: 12px;
      border: 2px solid #333;
      margin-bottom: 14px;
      background: #000;
      display: block;
    }
    .status {
      font-size: 0.9rem;
      color: #aaa;
      margin-bottom: 10px;
      min-height: 20px;
    }
    .status.error {
      color: #ff6666;
    }
    .row { margin: 6px 0; }
    button {
      width: 95px;
      height: 58px;
      margin: 5px;
      font-size: 16px;
      font-weight: bold;
      border: none;
      border-radius: 10px;
      background: #2b2b2b;
      color: #fff;
      cursor: pointer;
      -webkit-tap-highlight-color: transparent;
      transition: background 0.1s;
    }
    button:active, .pressed {
      background: #0077cc;
    }
    .stop-btn {
      background: #800000;
    }
    .stop-btn:active {
      background: #cc0000;
    }
    .light-btn {
      width: 110px;
      font-size: 14px;
    }
    .speed-control {
      margin: 10px 0;
      padding: 10px;
      background: #1a1a1a;
      border-radius: 8px;
    }
    .speed-control label {
      display: block;
      margin-bottom: 8px;
      font-size: 0.9rem;
    }
    .speed-control input {
      width: 200px;
      max-width: 80%;
      height: 8px;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <h2>ESP32 CAM Car</h2>
  <img id="stream" alt="Camera stream">
  <div class="status" id="status">Ready</div>

  <div class="speed-control">
    <label>Speed: <span id="speedValue">100</span>%</label>
    <input type="range" id="speedSlider" min="0" max="255" value="255">
  </div>

  <div class="row">
    <button id="btn-fwd" data-cmd="/forward">▲ Forward</button>
  </div>

  <div class="row">
    <button id="btn-left" data-cmd="/left">◀ Left</button>
    <button id="btn-stop" class="stop-btn" data-cmd="/stop">STOP</button>
    <button id="btn-right" data-cmd="/right">Right ▶</button>
  </div>

  <div class="row">
    <button id="btn-back" data-cmd="/back">▼ Back</button>
  </div>

  <div class="row">
    <button id="btn-lon" class="light-btn">Light ON</button>
    <button id="btn-loff" class="light-btn">Light OFF</button>
  </div>

  <script>
    let host = window.location.hostname;
    document.getElementById('stream').src = 'http://' + host + ':81/stream';

    let currentSpeed = 255;

    function sendCmd(url) {
      fetch(url)
        .then(res => res.text())
        .then(data => {
          console.log('Command response:', data);
        })
        .catch(err => {
          console.error('Command failed:', err);
          setStatus('Connection error', true);
        });
    }

    function setStatus(text, isError = false) {
      const statusEl = document.getElementById('status');
      statusEl.textContent = text;
      if (isError) {
        statusEl.classList.add('error');
      } else {
        statusEl.classList.remove('error');
      }
    }

    // Speed slider
    document.getElementById('speedSlider').addEventListener('input', (e) => {
      currentSpeed = parseInt(e.target.value);
      const percent = Math.round((currentSpeed / 255) * 100);
      document.getElementById('speedValue').textContent = percent;
    });

    let holdInterval = null;
    let currentBtn = null;

    function startHold(btn) {
      stopHold(false);
      currentBtn = btn;
      btn.classList.add('pressed');
      let cmd = btn.getAttribute('data-cmd');

      // Add speed parameter to movement commands
      if (cmd !== '/stop' && (cmd === '/forward' || cmd === '/back' || cmd === '/left' || cmd === '/right')) {
        cmd += '?speed=' + currentSpeed;
      }

      sendCmd(cmd);
      setStatus(btn.innerText);

      if (cmd !== '/stop') {
        holdInterval = setInterval(() => {
          sendCmd(cmd);
        }, 150);
      }
    }

    function stopHold(sendStop = true) {
      if (holdInterval) {
        clearInterval(holdInterval);
        holdInterval = null;
      }

      if (currentBtn) {
        currentBtn.classList.remove('pressed');
        currentBtn = null;
      }

      if (sendStop) {
        sendCmd('/stop');
        setStatus('Stopped');
      }
    }

    ['btn-fwd', 'btn-back', 'btn-left', 'btn-right'].forEach(id => {
      let btn = document.getElementById(id);

      btn.addEventListener('mousedown', e => { e.preventDefault(); startHold(btn); });
      btn.addEventListener('touchstart', e => { e.preventDefault(); startHold(btn); });
    });

    document.addEventListener('mouseup', () => stopHold(true));
    document.addEventListener('touchend', () => stopHold(true));
    document.addEventListener('touchcancel', () => stopHold(true));

    document.getElementById('btn-stop').addEventListener('click', () => {
      sendCmd('/stop');
      setStatus('Stopped');
      stopHold(false);
    });

    document.getElementById('btn-lon').addEventListener('click', () => {
      sendCmd('/lighton');
      setStatus('Light ON');
    });

    document.getElementById('btn-loff').addEventListener('click', () => {
      sendCmd('/lightoff');
      setStatus('Light OFF');
    });
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ===================== ROUTES =====================
void setupRoutes() {
  server.on("/", handleRoot);

  server.on("/forward", []() {
    int speed = MAX_SPEED;
    if (server.hasArg("speed")) {
      speed = constrain(server.arg("speed").toInt(), 0, MAX_SPEED);
    }
    forward(speed);
    server.send(200, "text/plain", "FORWARD");
  });

  server.on("/back", []() {
    int speed = MAX_SPEED;
    if (server.hasArg("speed")) {
      speed = constrain(server.arg("speed").toInt(), 0, MAX_SPEED);
    }
    back(speed);
    server.send(200, "text/plain", "BACK");
  });

  server.on("/left", []() {
    int speed = MAX_SPEED;
    if (server.hasArg("speed")) {
      speed = constrain(server.arg("speed").toInt(), 0, MAX_SPEED);
    }
    turnLeft(speed);
    server.send(200, "text/plain", "LEFT");
  });

  server.on("/right", []() {
    int speed = MAX_SPEED;
    if (server.hasArg("speed")) {
      speed = constrain(server.arg("speed").toInt(), 0, MAX_SPEED);
    }
    turnRight(speed);
    server.send(200, "text/plain", "RIGHT");
  });

  server.on("/stop", []() {
    stopCar();
    server.send(200, "text/plain", "STOP");
  });

  server.on("/lighton", []() {
    digitalWrite(FLASH_LED, HIGH);
    server.send(200, "text/plain", "LIGHT ON");
  });

  server.on("/lightoff", []() {
    digitalWrite(FLASH_LED, LOW);
    server.send(200, "text/plain", "LIGHT OFF");
  });
}

// ===================== CAMERA INIT =====================
void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    Serial.println("PSRAM found - using QVGA resolution");
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 16;
    config.fb_count = 1;
    Serial.println("No PSRAM - using QQVGA resolution");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("ERROR: Camera init failed with error code: 0x%x\n", err);
    Serial.println("Check camera connections and GPIO pins");
  } else {
    Serial.println("Camera initialized successfully");
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("ESP32 CAM Car - Starting Initialization");
  Serial.println("========================================");

  // Initialize GPIO pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(FLASH_LED, OUTPUT);

  // Initialize motor PWM
  initMotorPWM();
  stopMotors();
  digitalWrite(FLASH_LED, LOW);
  Serial.println("✓ GPIO and Motors initialized");

  // Initialize camera
  startCamera();

  // Setup WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("✓ WiFi AP started - SSID: ");
  Serial.println(ssid);
  Serial.print("  AP IP Address: ");
  Serial.println(apIP);
  Serial.print("  Connect from your phone/PC to access: http://");
  Serial.println(apIP);

  // Setup web server routes
  setupRoutes();
  server.begin();
  Serial.println("✓ Web Server started on port 80");

  // Setup camera streaming server
  startCameraServer();

  Serial.println("========================================");
  Serial.println("System ready! Connect and navigate.");
  Serial.println("========================================\n");
}

// ===================== MAIN LOOP =====================
void loop() {
  server.handleClient();

  // Failsafe: Stop car if no command received within timeout
  if (currentCommand != "STOP" && millis() - lastCommandTime > COMMAND_TIMEOUT) {
    stopCar();
  }

  // Small delay to prevent watchdog issues
  delay(1);
}
