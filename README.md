# Sistem Pengusir Hama Otomatis Berbasis IoT 🌾[cite: 1]

Sistem Pengusir Hama Otomatis ini mendeteksi kehadiran hama dengan menggabungkan sensor PIR (deteksi gerakan) dan sensor ultrasonik HC-SR04 (pengukuran jarak) menggunakan mikrokontroler ESP32[cite: 2].

## 👥 Tim Pengembang (Universitas Jenderal Soedirman)
* Lulu Waskito Adi | H1H024001[cite: 1]
* Muhammad Faizal Khabibi | H1H024003[cite: 1]
* Bahtiar Rizqi Efendy | H1H024006[cite: 1]

## 🛠️ Komponen Perangkat Keras
| Komponen | Fungsi | Koneksi Pin |
| :--- | :--- | :--- |
| **ESP32** | Pengendali utama, membaca sensor, koneksi Wi-Fi & MQTT[cite: 4] | - |
| **Sensor PIR** | Mendeteksi gerakan di area lahan[cite: 4] | GPIO 27[cite: 4] |
| **HC-SR04** | Mengukur jarak objek yang bergerak[cite: 4] | TRIG: GPIO 25, ECHO: GPIO 26[cite: 4] |
| **Buzzer** | Alarm suara saat hama terdeteksi[cite: 4] | GPIO 14[cite: 4] |
| **LED Merah** | Indikator visual saat hama terdeteksi[cite: 4] | GPIO 13[cite: 4] |
| **ESP32-CAM** | Mengambil gambar untuk mengenali jenis hama[cite: 4] | *Dalam perancangan*[cite: 4] |

## 💻 Perangkat Lunak & Layanan
* **Wokwi:** Simulator untuk merancang dan menguji rangkaian ESP32 beserta program[cite: 4].
* **Pemrograman C++ (Arduino):** Menggunakan pustaka WiFi dan PubSubClient[cite: 4].
* **Broker MQTT HiveMQ:** Berperan sebagai perantara pesan antara ESP32 dan aplikasi pemantau pada `broker.hivemq.com` port `1883`[cite: 4].
* **MQTT Explorer:** Digunakan untuk memantau status dan mengirim perintah kontrol saat pengujian[cite: 4].

## ⚙️ Mekanisme Kerja & Mode
Hama dianggap terdeteksi apabila terdapat gerakan dan objek berada pada jarak 50 cm atau kurang dari sensor[cite: 2].
* **Mode AUTO:** LED dan buzzer menyala (frekuensi 4000 Hz) secara otomatis selama hama terdeteksi, dan mati saat hama tidak terdeteksi[cite: 8].
* **Mode MANUAL:** LED dan buzzer mengikuti perintah yang diterima melalui MQTT[cite: 8].

## 📡 Komunikasi MQTT
### 1. Publish Status (ESP32 -> Broker)
Dikirim setiap 1 detik untuk memantau status perangkat di lahan[cite: 8].
* **Topik:** `pertanian/hama/status`[cite: 9]
* **Contoh Payload (JSON):**
  ```json
  {"gerakan": true, "jarak_cm": 32.5, "status": "HAMA DETECTED", "alarm": "ACTIVE"}
  ```[cite: 9]

### 2. Subscribe Kontrol (Broker -> ESP32)
Digunakan untuk beralih mode dan mengontrol perangkat aktuator dari jarak jauh[cite: 8].
* **Topik:** `pertanian/hama/kontrol`[cite: 9]
* **Contoh Payload (JSON):**
  ```json
  {"mode": "MANUAL", "buzzer": "ON", "led": "OFF"}
  ```[cite: 9]
