# Sistem Pengusir Hama Otomatis Berbasis IoT 

Sistem Pengusir Hama Otomatis ini mendeteksi kehadiran hama dengan menggabungkan sensor PIR (deteksi gerakan) dan sensor ultrasonik HC-SR04 (pengukuran jarak) menggunakan mikrokontroler ESP32.

## 👥 Tim Pengembang (Universitas Jenderal Soedirman)
* Lulu Waskito Adi | H1H024001
* Muhammad Faizal Khabibi | H1H024003
* Bahtiar Rizqi Efendy | H1H024006

## 🛠️ Komponen Perangkat Keras
| Komponen | Fungsi | Koneksi Pin |
| :--- | :--- | :--- |
| **ESP32** | Pengendali utama, membaca sensor, koneksi Wi-Fi & MQTT | - |
| **Sensor PIR** | Mendeteksi gerakan di area lahan | GPIO 27 |
| **HC-SR04** | Mengukur jarak objek yang bergerak | TRIG: GPIO 25, ECHO: GPIO 26 |
| **Buzzer** | Alarm suara saat hama terdeteksi | GPIO 14 |
| **LED Merah** | Indikator visual saat hama terdeteksi | GPIO 13 |
| **ESP32-CAM** | Mengambil gambar untuk mengenali jenis hama | *Dalam perancangan* |

## 💻 Perangkat Lunak & Layanan
* **Wokwi:** Simulator untuk merancang dan menguji rangkaian ESP32 beserta program.
* **Pemrograman C++ (Arduino):** Menggunakan pustaka WiFi dan PubSubClient.
* **Broker MQTT HiveMQ:** Berperan sebagai perantara pesan antara ESP32 dan aplikasi pemantau pada `broker.hivemq.com` port `1883`.
* **MQTT Explorer:** Digunakan untuk memantau status dan mengirim perintah kontrol saat pengujian.

## ⚙️ Mekanisme Kerja & Mode
Hama dianggap terdeteksi apabila terdapat gerakan dan objek berada pada jarak 50 cm atau kurang dari sensor.
* **Mode AUTO:** LED dan buzzer menyala (frekuensi 4000 Hz) secara otomatis selama hama terdeteksi, dan mati saat hama tidak terdeteksi.
* **Mode MANUAL:** LED dan buzzer mengikuti perintah yang diterima melalui MQTT.

## 📡 Komunikasi MQTT
### 1. Publish Status (ESP32 -> Broker)
Dikirim setiap 1 detik untuk memantau status perangkat di lahan.
* **Topik:** `pertanian/hama/status`
* **Contoh Payload (JSON):**
  ```json
  {"gerakan": true, "jarak_cm": 32.5, "status": "HAMA DETECTED", "alarm": "ACTIVE"}
  ```

### 2. Subscribe Kontrol (Broker -> ESP32)
Digunakan untuk beralih mode dan mengontrol perangkat aktuator dari jarak jauh.
* **Topik:** `pertanian/hama/kontrol`
* **Contoh Payload (JSON):**
  ```json
  {"mode": "MANUAL", "buzzer": "ON", "led": "OFF"}
  ```
