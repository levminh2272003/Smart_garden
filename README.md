# Smart Greenhouse – Hệ Thống Vườn Nhà Kính Thông Minh

## Giới thiệu

Dự án **Smart Greenhouse** được xây dựng nhằm giám sát và điều khiển tự động các thông số môi trường như **nhiệt độ, độ ẩm, ánh sáng, và mực nước** trong nhà kính.  
Hệ thống sử dụng **ESP32** làm bộ xử lý trung tâm, kết hợp cảm biến, relay, màn hình OLED và giao thức **MQTT** để điều khiển và giám sát từ xa.

### Mục tiêu hệ thống
- Tự động duy trì điều kiện phù hợp cho cây trồng.  
- Giảm công sức chăm sóc thủ công.  
- Theo dõi dữ liệu thời gian thực qua Internet.  
- Tăng hiệu quả và hiện đại hóa nông nghiệp.

---

## Thành phần phần cứng

| Thành phần | Chức năng chính |
|-------------|----------------|
| **ESP32** | Vi điều khiển trung tâm, kết nối WiFi và giao tiếp MQTT |
| **Cảm biến DHT22** | Đo nhiệt độ và độ ẩm không khí |
| **Cảm biến HC-SR04** | Đo mực nước trong bể chứa |
| **Cảm biến LDR** | Đo cường độ ánh sáng |
| **Màn hình OLED SSD1306** | Hiển thị thông tin cảm biến và trạng thái hệ thống |
| **Relay Module** | Bật/tắt thiết bị (đèn, quạt, bơm, phun sương) |
| **Button (Nút nhấn)** | Chuyển đổi chế độ hoặc điều khiển thủ công |
| **LED cảnh báo** | Hiển thị trạng thái hoạt động của hệ thống |

---

## Phần mềm và công cụ

- **Arduino IDE** – Viết và nạp chương trình cho ESP32.  
- **Wokwi Simulator** – Mô phỏng hệ thống phần cứng.  
- **MQTT Broker (HiveMQ / Mosquitto)** – Truyền nhận dữ liệu giữa thiết bị và ứng dụng.  
- **FreeRTOS** – Quản lý đa tác vụ song song trên ESP32.  
- **Ngôn ngữ lập trình** – C/C++.

---

## Nguyên lý hoạt động

### 1. Giám sát môi trường

ESP32 liên tục đọc dữ liệu từ các cảm biến:
- **DHT22**: nhiệt độ, độ ẩm.  
- **LDR**: cường độ ánh sáng.  
- **HC-SR04**: mực nước.  

Kết quả hiển thị trên **màn hình OLED** và gửi qua **MQTT Broker** để theo dõi từ xa.

---

### 2. Hai chế độ hoạt động

#### Chế độ **Tự động (AUTO)**
| Điều kiện | Hành động |
|------------|-----------|
| Nhiệt độ > ngưỡng | Bật **quạt làm mát** |
| Độ ẩm < ngưỡng | Bật **phun sương/tưới** |
| Ánh sáng < ngưỡng | Bật **đèn chiếu sáng** |
| Mực nước thấp | Cảnh báo hoặc bật **bơm cấp nước** |

Dữ liệu được đồng bộ lên **MQTT** mỗi 10 giây.

#### Chế độ **Thủ công (MANUAL)**
- Người dùng có thể bật/tắt relay bằng **nút nhấn** hoặc **lệnh MQTT**.  
- Màn hình OLED vẫn hiển thị dữ liệu cảm biến theo thời gian thực.

---

## Hướng dẫn vận hành

### 1️. Khởi động hệ thống
- Cấp nguồn cho **ESP32**.  
- **Màn hình OLED** hiển thị trạng thái kết nối **WiFi** và **MQTT**.  

---

### 2️. Trong quá trình hoạt động
- Hệ thống **tự động điều khiển relay** khi ở chế độ **AUTO**.  
- Dữ liệu cảm biến được **gửi lên MQTT Broker** mỗi **10 giây**.  

---

### 3️. Điều khiển thủ công
- **Nhấn nút** để bật/tắt relay hoặc **chuyển sang chế độ MANUAL**.  
- Có thể điều khiển qua **ứng dụng MQTT Dashboard**.  

---

## Kết quả đạt được

-  **Hệ thống hoạt động ổn định**: Chính xác ở cả hai chế độ **AUTO** và **MANUAL**.  
-  **Đồng bộ dữ liệu qua MQTT**: Truyền nhận thời gian thực, ổn định và tin cậy.  
-  **Hiển thị rõ ràng trên màn hình OLED**: Hiển thị đầy đủ thông tin cảm biến và trạng thái relay.  
-  **Khả năng mở rộng cao**: Dễ dàng tích hợp thêm cảm biến hoặc thiết bị điều khiển khác.  
-  **Tiết kiệm năng lượng**: Phù hợp cho ứng dụng thực tế, vận hành dài lâu.  

---

## Hướng phát triển

-  **Tích hợp AI/PID** để tối ưu điều khiển môi trường.  
-  **Bổ sung cảm biến CO₂, pH đất, ánh sáng chuyên dụng.**  
-  **Phát triển Web Dashboard / Ứng dụng di động** để giám sát toàn diện.  
-  **Nâng cao bảo mật MQTT** bằng xác thực người dùng và chứng chỉ **SSL/TLS**.  
-  **Kết hợp năng lượng mặt trời** để hệ thống hoạt động độc lập, tiết kiệm điện.
