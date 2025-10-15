#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Định nghĩa các tham số màn hình OLED
#define SCREEN_WIDTH 128    // Chiều rộng màn hình OLED
#define SCREEN_HEIGHT 64    // Chiều cao màn hình OLED
#define OLED_RESET -1       // Reset mặc định cho màn hình OLED (không dùng)

// Khởi tạo màn hình OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Định nghĩa chân cảm biến và thiết bị
const int DHT_PIN = 15;      // Chân cảm biến DHT22
const int DHT_TYPE = DHT22;  // Loại cảm biến DHT22
const int photoresistorPin = 33; // Chân cảm biến quang
const int trigPin = 5;       // Chân Trig của cảm biến siêu âm
const int echoPin = 18;      // Chân Echo của cảm biến siêu âm

const int LED = 12;  // Chân kết nối đèn LED

// Định nghĩa chân nút bấm và relay
const int button_temp = 26;   // Nút bấm điều khiển nhiệt độ
const int button_hum = 27;    // Nút bấm điều khiển độ ẩm
const int button_light = 14;  // Nút bấm điều khiển ánh sáng
const int button_bom = 25;    // Nút bấm điều khiển máy bơm
const int button_Mode = 32;   // Nút bấm thay đổi chế độ

const int relay_temp = 23;    // Relay điều khiển theo nhiệt độ
const int relay_hum = 19;     // Relay điều khiển theo độ ẩm
const int relay_light = 17;   // Relay điều khiển theo ánh sáng
const int relay_dis = 16;     // Relay điều khiển thiết bị khác

// Định nghĩa các ngưỡng giá trị
float tempThreshold = 32;     // Ngưỡng nhiệt độ
float humThreshold = 65;      // Ngưỡng độ ẩm
float lightThreshold = 1000;  // Ngưỡng ánh sáng (lux)
float waterThreshold = 50;    // Ngưỡng mức nước (cm)
float tankHeight = 100;       // Chiều cao bể chứa nước (cm)

volatile bool relayTemp_State = false;  // Trạng thái relay điều khiển nhiệt độ
volatile bool relayHum_State = false;   // Trạng thái relay điều khiển độ ẩm
volatile bool relayLight_State = false; // Trạng thái relay điều khiển ánh sáng
volatile bool relayDis_State = false;   // Trạng thái relay điều khiển bơm
volatile bool Mode_State = false;       // Biến lưu trạng thái của chế độ

// Biến lưu thời gian debounce cho các nút điều khiển (tránh rung nút)
volatile unsigned long lastDebounceTimeTemp = 0;
volatile unsigned long lastDebounceTimeHum = 0;
volatile unsigned long lastDebounceTimeLight = 0;
volatile unsigned long lastDebounceTimeDis = 0;
volatile unsigned long lastDebounceTimeMode = 0;

const unsigned long debounceDelay = 200;  // Thời gian chống rung nút 200ms

unsigned long previousMillis = 0;         // Lưu thời gian lần cập nhật trước cho OLED
const long intervaloled = 100;             // Thời gian cập nhật OLED mỗi 100ms

DHT dht(DHT_PIN, DHT_TYPE);  // Khởi tạo đối tượng DHT với chân cảm biến và loại DHT

// Khai báo thông tin MQTT
#define MQTT_CLIENT_ID "clientidweather"    // ID của client khi kết nối với MQTT broker
#define MQTT_BROKER    "mqtt-dashboard.com" // Địa chỉ MQTT broker
#define MQTT_TOPIC     "SMART8"             // Chủ đề MQTT cho dữ liệu gửi đi
#define MQTT_TOPICDK   "SMART8DK"           // Chủ đề MQTT cho điều khiển

const char* ssid = "Wokwi-GUEST";           // Tên mạng WiFi
const char* password = "";                  // Mật khẩu WiFi (trong trường hợp này không cần)

WiFiClient espClient;         // Khởi tạo đối tượng WiFiClient cho ESP32
PubSubClient client(espClient);  // Khởi tạo đối tượng PubSubClient cho MQTT, sử dụng WiFiClient

// Hàm kết nối WiFi
void connectWiFi() 
{
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);  // Bắt đầu kết nối WiFi với SSID và password
    while (WiFi.status() != WL_CONNECTED)  // Kiểm tra trạng thái kết nối
    {
      delay(500);
      Serial.print(".");  // In ra dấu "." trong khi chờ kết nối
    }
    Serial.println(" Connected!");  // Thông báo đã kết nối thành công
}

// Hàm kết nối lại MQTT
void reconnectMQTT() 
{
    while (!client.connected()) {  // Kiểm tra trạng thái kết nối MQTT
      Serial.print("Connecting to MQTT...");
      if (client.connect(MQTT_CLIENT_ID)) {  // Thử kết nối với broker MQTT
        Serial.println(" Connected!");  // Kết nối thành công
        client.subscribe(MQTT_TOPICDK);  // Đăng ký chủ đề để nhận lệnh điều khiển
      } else {
        Serial.print(" failed, rc=");
        Serial.print(client.state());  // In ra mã lỗi nếu kết nối thất bại
        Serial.println(" try again in 5 seconds");
        delay(5000);  // Chờ 5 giây trước khi thử lại
      }
    }
}

// Hàm gửi dữ liệu lên MQTT
void sendToMQTT(float temperature, float humidity, float waterLevel, float lux) {
  String payload = "{\"temperature\":" + String(temperature) +
                   ",\"humidity\":" + String(humidity) +
                   ",\"waterLevel\":" + String(waterLevel) + 
                   ",\"lightIntensity\":" + String(lux) +  
                   ",\"relay_temp\":" + (relayTemp_State ? "ON" : "OFF") + 
                   ",\"relay_hum\":" + (relayHum_State ? "ON" : "OFF") + 
                   ",\"relay_light\":" + (relayLight_State ? "ON" : "OFF") + 
                   ",\"relay_dis\":" + (relayDis_State ? "ON" : "OFF") +
                   ",\"MODE\":" + (Mode_State ? "AUTO" : "HAND") +  "}";  // Tạo payload JSON
  client.publish(MQTT_TOPIC, payload.c_str());  // Gửi payload lên chủ đề MQTT
}

// Hàm điều khiển relay dựa trên dữ liệu nhận được từ MQTT
void controlRelay(char* message) {
  if (!Mode_State) {  // Chỉ cho phép điều khiển relay khi ở chế độ HAND (thủ công)
    if (strcmp(message, "TEMP_ON") == 0) {
      relayTemp_State = true;
      digitalWrite(relay_temp, HIGH);  // Bật relay nhiệt độ
    } else if (strcmp(message, "TEMP_OFF") == 0) {
      relayTemp_State = false;
      digitalWrite(relay_temp, LOW);   // Tắt relay nhiệt độ
    } else if (strcmp(message, "HUM_ON") == 0) {
      relayHum_State = true;
      digitalWrite(relay_hum, HIGH);   // Bật relay độ ẩm
    } else if (strcmp(message, "HUM_OFF") == 0) {
      relayHum_State = false;
      digitalWrite(relay_hum, LOW);    // Tắt relay độ ẩm
    } else if (strcmp(message, "LIGH_ON") == 0) {
      relayLight_State = true;
      digitalWrite(relay_light, HIGH); // Bật relay ánh sáng
    } else if (strcmp(message, "LIGH_OFF") == 0) {
      relayLight_State = false;
      digitalWrite(relay_light, LOW);  // Tắt relay ánh sáng
    } else if (strcmp(message, "DIS_ON") == 0) {
      relayDis_State = true;
      digitalWrite(relay_dis, HIGH);   // Bật relay bơm
    } else if (strcmp(message, "DIS_OFF") == 0) {
      relayDis_State = false;
      digitalWrite(relay_dis, LOW);    // Tắt relay bơm
    } 
  }
  
  // Điều khiển chuyển đổi chế độ giữa AUTO và HAND
  if (strcmp(message, "MODE_AUTO") == 0) {
    Mode_State = true;    // Chuyển sang chế độ tự động
    digitalWrite(LED, HIGH);  // Bật đèn LED báo chế độ AUTO
  } else if (strcmp(message, "MODE_HAND") == 0) {
    Mode_State = false;   // Chuyển sang chế độ thủ công
    digitalWrite(LED, LOW);   // Tắt đèn LED báo chế độ HAND
  }
}

// Hàm xử lý tin nhắn MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';  // Kết thúc chuỗi payload
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println((char*)payload);  // In ra tin nhắn nhận được
  controlRelay((char*)payload);    // Gọi hàm điều khiển relay dựa trên tin nhắn
}

// Ngắt ngoài để điều khiển relay 1 (nhiệt độ)
void IRAM_ATTR buttonTempInterrupt() {
  if ((millis() - lastDebounceTimeTemp) > debounceDelay && !Mode_State) {  // Kiểm tra chống rung và chế độ
    relayTemp_State = !relayTemp_State;  // Thay đổi trạng thái relay nhiệt độ
    digitalWrite(relay_temp, relayTemp_State ? HIGH : LOW);  // Bật/tắt relay
    lastDebounceTimeTemp = millis();  // Cập nhật thời gian debounce
  }
}

// Ngắt ngoài để điều khiển relay 2 (độ ẩm)
void IRAM_ATTR buttonHumInterrupt() {
  if ((millis() - lastDebounceTimeHum) > debounceDelay && !Mode_State) {
    relayHum_State = !relayHum_State;  // Thay đổi trạng thái relay độ ẩm
    digitalWrite(relay_hum, relayHum_State ? HIGH : LOW);
    lastDebounceTimeHum = millis();
  }
}

// Ngắt ngoài để điều khiển relay 3 (ánh sáng)
void IRAM_ATTR buttonLightInterrupt() {
  if ((millis() - lastDebounceTimeLight) > debounceDelay && !Mode_State) {
    relayLight_State = !relayLight_State;  // Thay đổi trạng thái relay ánh sáng
    digitalWrite(relay_light, relayLight_State ? HIGH : LOW);
    lastDebounceTimeLight = millis();
  }
}

// Ngắt ngoài để điều khiển bơm
void IRAM_ATTR buttonDisInterrupt() {
  if ((millis() - lastDebounceTimeDis) > debounceDelay && !Mode_State) {
    relayDis_State = !relayDis_State;  // Thay đổi trạng thái relay bơm
    digitalWrite(relay_dis, relayDis_State ? HIGH : LOW);
    lastDebounceTimeDis = millis();
  }
}

// Ngắt ngoài để điều khiển chế độ (Mode)
void IRAM_ATTR buttonModeInterrupt() {
  if ((millis() - lastDebounceTimeMode) > debounceDelay) {
    Mode_State = !Mode_State;  // Thay đổi trạng thái chế độ (AUTO/HAND)
    digitalWrite(LED, Mode_State ? HIGH : LOW);  // Bật/tắt LED báo hiệu chế độ
    lastDebounceTimeMode = millis();
  }
}

// Khởi tạo các cảm biến và thiết bị
void setup() {
  Serial.begin(115200);  // Khởi tạo cổng Serial

  // Cấu hình chân cảm biến và relay
  pinMode(photoresistorPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(relay_dis, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(button_temp, INPUT_PULLUP);  
  pinMode(button_hum, INPUT_PULLUP);  
  pinMode(button_light, INPUT_PULLUP);  
  pinMode(button_Mode, INPUT_PULLUP); 
  pinMode(button_bom, INPUT_PULLUP);  
  pinMode(relay_temp, OUTPUT);
  pinMode(relay_hum, OUTPUT);
  pinMode(relay_light, OUTPUT);

  dht.begin();  // Khởi động cảm biến DHT

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Khởi động màn hình OLED
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  display.display();

  // Kết nối WiFi và MQTT
  connectWiFi();
  client.setServer(MQTT_BROKER, 1883);  // Thiết lập MQTT broker
  client.setCallback(mqttCallback);     // Đặt hàm callback cho MQTT

  // Thiết lập ngắt ngoài cho các nút bấm
  attachInterrupt(digitalPinToInterrupt(button_temp), buttonTempInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(button_hum), buttonHumInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(button_light), buttonLightInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(button_bom), buttonDisInterrupt, FALLING);  
  attachInterrupt(digitalPinToInterrupt(button_Mode), buttonModeInterrupt, FALLING);
}

// Hàm đọc nhiệt độ và độ ẩm và điều khiển Relay 
void readTemperatureAndControlLED(float &temperature, float &humidity) {
  temperature = dht.readTemperature();  // Đọc nhiệt độ từ cảm biến
  humidity = dht.readHumidity();        // Đọc độ ẩm từ cảm biến

  if (Mode_State) {  // Chỉ điều khiển trong chế độ AUTO
    if (temperature >= tempThreshold) {
      digitalWrite(relay_temp, HIGH); relayTemp_State = true;  // Bật relay nhiệt độ nếu vượt ngưỡng
    } else if (temperature < tempThreshold) {
      digitalWrite(relay_temp, LOW); relayTemp_State = false;  // Tắt relay nhiệt độ nếu dưới ngưỡng
    }
  
    if (humidity <= humThreshold) {
      digitalWrite(relay_hum, HIGH); relayHum_State = true;  // Bật relay độ ẩm nếu dưới ngưỡng
    } else if (humidity > humThreshold) {
      digitalWrite(relay_hum, LOW); relayHum_State = false;  // Tắt relay độ ẩm nếu vượt ngưỡng
    }
  }
}

// Hàm đọc cường độ ánh sáng và điều khiển Relay
void readLightAndControlLED(float &lux) {
  int lightIntensity = analogRead(photoresistorPin); // Đọc giá trị ánh sáng từ chân cảm biến
  float voltage = lightIntensity / 4096.0 * 3.3; // Tính điện áp từ giá trị đọc
  float resistance = 2000 * voltage / (1 - voltage / 3.3); // Tính điện trở từ điện áp
  lux = pow(33 * 1e3 * pow(10, 0.7) / resistance, (1 / 0.7)); // Chuyển đổi điện trở thành cường độ ánh sáng (lux)
  
  if (Mode_State) {  // Chỉ điều khiển trong chế độ AUTO
    if (lux < lightThreshold) { // Kiểm tra xem cường độ ánh sáng có dưới ngưỡng không
      digitalWrite(relay_light, HIGH); relayLight_State = true; // Bật relay nếu cường độ ánh sáng thấp
    } else {
      digitalWrite(relay_light, LOW); relayLight_State = false; // Tắt relay nếu cường độ ánh sáng đủ
    }
  }
}

// Hàm đọc khoảng cách và điều khiển Relay bơm
void readDistanceAndControlPump(float &waterLevel) {
  digitalWrite(trigPin, LOW); // Bắt đầu bằng cách tắt tín hiệu trig
  delayMicroseconds(2); // Đợi 2 micro giây
  digitalWrite(trigPin, HIGH); // Gửi tín hiệu trig
  delayMicroseconds(10); // Đợi 10 micro giây
  digitalWrite(trigPin, LOW); // Tắt tín hiệu trig
  long duration = pulseIn(echoPin, HIGH); // Đo thời gian tín hiệu echo quay về
  float distanceCm = duration * 0.034 / 2; // Tính toán khoảng cách (cm)
  waterLevel = tankHeight - distanceCm; // Tính toán mức nước
  if (waterLevel < 0) { // Kiểm tra nếu mức nước âm
    waterLevel = 0; // Đặt lại mức nước về 0
  }

  if (Mode_State) {  // Chỉ điều khiển trong chế độ AUTO
    if (waterLevel < waterThreshold) { // Nếu mức nước dưới ngưỡng
      digitalWrite(relay_dis, HIGH); relayDis_State = true; // Bật bơm
    } else if (waterLevel >= tankHeight - 10) { // Nếu mức nước gần đầy
      digitalWrite(relay_dis, LOW); relayDis_State = false; // Tắt bơm
    }
  }
}

// Hàm hiển thị dữ liệu lên OLED
void displayData(float temp, float humid, float lux, float waterLevel) {
  display.clearDisplay(); // Xóa màn hình OLED
  display.setTextSize(0.8); // Thiết lập kích thước chữ
  display.setTextColor(WHITE); // Thiết lập màu chữ
  display.setCursor(0, 0); // Đặt vị trí con trỏ trên màn hình
  display.print("Temp: "); // In ra tiêu đề nhiệt độ
  display.print((int)temp); // In ra giá trị nhiệt độ
  display.print((char)247); // In ký hiệu độ C
  display.print("C");
  display.print(", Hum: "); // In ra tiêu đề độ ẩm
  display.print((int)humid); // In ra giá trị độ ẩm
  display.println("%"); // Kết thúc dòng
  display.print("Light: "); // In ra tiêu đề ánh sáng
  display.print((int)lux); // In ra giá trị cường độ ánh sáng
  display.println(" lux"); // Kết thúc dòng
  display.print("waterLevel: "); // In ra tiêu đề mức nước
  display.print((int)waterLevel); // In ra giá trị mức nước
  display.println(" cm"); // Kết thúc dòng
  display.print("MODE: "); // In ra chế độ hoạt động
  display.println(Mode_State ? "AUTO" : "HAND"); // Hiển thị chế độ AUTO hoặc HAND
  display.print("relay_temp: "); // In ra trạng thái relay nhiệt độ
  display.println(relayTemp_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  display.print("relay_hum: "); // In ra trạng thái relay độ ẩm
  display.println(relayHum_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  display.print("relay_light: "); // In ra trạng thái relay ánh sáng
  display.println(relayLight_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  display.print("relay_dis: "); // In ra trạng thái relay bơm
  display.println(relayDis_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  display.display(); // Cập nhật màn hình
}

// Hàm hiển thị trạng thái hệ thống và relay lên Serial
void displayStatusToSerial(float temp, float humid, float lux, float waterLevel) {
  Serial.println("===== System Status ====="); // In tiêu đề trạng thái
  Serial.print("Temperature: "); // In ra tiêu đề nhiệt độ
  Serial.print(temp); // In ra giá trị nhiệt độ
  Serial.println(" °C"); // Kết thúc dòng
  Serial.print("Humidity: "); // In ra tiêu đề độ ẩm
  Serial.print(humid); // In ra giá trị độ ẩm
  Serial.println(" %"); // Kết thúc dòng
  Serial.print("Light Intensity: "); // In ra tiêu đề cường độ ánh sáng
  Serial.print(lux); // In ra giá trị cường độ ánh sáng
  Serial.println(" lux"); // Kết thúc dòng
  Serial.print("Water Level: "); // In ra tiêu đề mức nước
  Serial.print(waterLevel); // In ra giá trị mức nước
  Serial.println(" cm"); // Kết thúc dòng
  Serial.print("Mode: "); // In ra chế độ hoạt động
  Serial.println(Mode_State ? "AUTO" : "HAND"); // Hiển thị chế độ
  Serial.print("Relay Temp: "); // In ra trạng thái relay nhiệt độ
  Serial.println(relayTemp_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  Serial.print("Relay Hum: "); // In ra trạng thái relay độ ẩm
  Serial.println(relayHum_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  Serial.print("Relay Light: "); // In ra trạng thái relay ánh sáng
  Serial.println(relayLight_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  Serial.print("Relay Pump: "); // In ra trạng thái relay bơm
  Serial.println(relayDis_State ? "ON" : "OFF"); // Hiển thị ON/OFF
  Serial.println("=========================="); // In dòng ngăn cách
}

// Vòng lặp chính
void loop() {
  unsigned long currentMillis = millis(); // Lấy thời gian hiện tại

  // Nếu đã đến lúc cập nhật (500ms trôi qua)
  if (currentMillis - previousMillis >= intervaloled) {
    previousMillis = currentMillis; // Cập nhật lại thời gian lần trước

    float temp, humid, lux, waterLevel; // Khai báo biến cho các cảm biến

    // Đọc cảm biến và điều khiển LED
    readTemperatureAndControlLED(temp, humid); // Đọc nhiệt độ và độ ẩm
    readLightAndControlLED(lux); // Đọc cường độ ánh sáng
    readDistanceAndControlPump(waterLevel); // Đọc mức nước và điều khiển bơm

    // Gửi dữ liệu lên MQTT
    sendToMQTT(temp, humid, waterLevel, lux); // Gửi tất cả dữ liệu cảm biến

    // Hiển thị dữ liệu lên màn hình OLED
    displayData(temp, humid, lux, waterLevel); // Hiển thị dữ liệu trên OLED

    // Hiển thị trạng thái lên Serial
    displayStatusToSerial(temp, humid, lux, waterLevel); // Hiển thị trạng thái hệ thống trên Serial
  }

  // Kiểm tra kết nối MQTT
  if (!client.connected()) {
    reconnectMQTT(); // Kết nối lại nếu mất kết nối
  }
  client.loop(); // Gọi hàm xử lý MQTT
}
