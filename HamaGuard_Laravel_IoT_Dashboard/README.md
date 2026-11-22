# HamaGuard — Laravel IoT Dashboard

Dashboard monitoring dan kontrol sistem pengusir hama ESP32 berbasis MQTT. Disusun mengikuti dokumen progres proyek: HiveMQ `broker.hivemq.com:1883`, topik status `pertanian/hama/status`, topik kontrol `pertanian/hama/kontrol`, format JSON, status AMAN/WASPADA/BAHAYA, mode AUTO/MANUAL.

## Arsitektur

`ESP32 --publish JSON--> HiveMQ --subscribe--> Laravel command --insert--> Database --HTTP polling--> Dashboard`

`Dashboard --POST HTTP--> Laravel controller --publish JSON--> HiveMQ --subscribe--> ESP32`

Laravel menjadi MQTT bridge: proses `mqtt:listen` berlangganan topik status dan menyimpan setiap payload ke tabel `hama_readings`. Dashboard mengambil data dari endpoint HTTP `/api/latest`. Tombol kontrol memanggil `/api/control`, lalu Laravel publish JSON ke topik kontrol.

## Kebutuhan

- PHP 8.2+ (disarankan PHP 8.3), Composer 2, ekstensi `sockets`, `pdo_sqlite` atau `pdo_pgsql`.
- Laragon/XAMPP atau PHP lokal.
- Akses internet ke broker HiveMQ publik port 1883.
- ESP32 sudah publish/subscribe sesuai topik pada `.env`.

## Instalasi Windows / Laragon (pemula)

1. Ekstrak ZIP ke folder, misalnya `C:\laragon\www\hama-iot-laravel`.
2. Buka Terminal/PowerShell di folder tersebut.
3. Pastikan PHP dan Composer terbaca:
   ```powershell
   php -v
   composer -V
   ```
4. Pasang dependensi Laravel dan MQTT:
   ```powershell
   composer install
   ```
5. Buat file konfigurasi lokal:
   ```powershell
   copy .env.example .env
   php artisan key:generate
   ```
6. Buat file database SQLite:
   ```powershell
   New-Item -ItemType File -Path database\database.sqlite -Force
   ```
   Pastikan `.env` memakai `DB_CONNECTION=sqlite` dan `DB_DATABASE=database/database.sqlite`.
7. Jalankan migrasi:
   ```powershell
   php artisan migrate
   ```
8. Periksa konfigurasi MQTT di `.env`:
   ```env
   MQTT_HOST=broker.hivemq.com
   MQTT_PORT=1883
   MQTT_STATUS_TOPIC=pertanian/hama/status
   MQTT_CONTROL_TOPIC=pertanian/hama/kontrol
   ```
   Broker publik ini tidak memakai autentikasi pada konfigurasi proyek ini. Jangan gunakan broker publik untuk data/perintah sensitif; untuk penggunaan nyata, gunakan broker privat dengan TLS dan kredensial.
9. Buka **Terminal pertama**, jalankan listener (biarkan tetap hidup):
   ```powershell
   php artisan mqtt:listen
   ```
10. Buka **Terminal kedua**, jalankan web Laravel:
    ```powershell
    php artisan serve
    ```
11. Buka `http://127.0.0.1:8000`. Dashboard akan memperbarui data setiap 2 detik.

## PostgreSQL (opsional)

Buat database `hama_iot`, lalu ubah `.env`:

```env
DB_CONNECTION=pgsql
DB_HOST=127.0.0.1
DB_PORT=5432
DB_DATABASE=hama_iot
DB_USERNAME=postgres
DB_PASSWORD=password_postgres_anda
```

Aktifkan ekstensi `pdo_pgsql` pada PHP yang dipakai, lalu jalankan `php artisan migrate`.

## Format MQTT yang didukung

### Status ESP32 → broker → Laravel

Topik: `pertanian/hama/status` (dikirim ESP32 sekitar setiap 1 detik).

```json
{"gerakan":true,"jarak_cm":32.5,"status":"BAHAYA","alarm":"ON","ir":"ON","buzzer_ok":true,"lonceng":"OFF"}
```

Field `jarak_cm` boleh `null` jika tidak ada pantulan. Listener juga menerima field tambahan; payload asli disimpan pada kolom `payload`. `mode` akan disimpan jika ESP32 menyertakannya.

### Kontrol Dashboard → broker → ESP32

Topik: `pertanian/hama/kontrol`.

```json
{"mode":"MANUAL","buzzer":"ON","led":"OFF","ir":"OFF","lonceng":"ON"}
```

Tombol aktuator mengirim `mode: MANUAL` bersama field yang diubah. Tombol AUTO mengirim `{"mode":"AUTO"}`.

## Cara menguji tanpa ESP32

1. Jalankan `php artisan mqtt:listen` dan `php artisan serve`.
2. Gunakan MQTT Explorer, hubungkan ke `broker.hivemq.com:1883`.
3. Publish ke `pertanian/hama/status` dengan payload JSON contoh di atas.
4. Dashboard seharusnya menampilkan status dan riwayat baru.
5. Klik tombol kontrol pada dashboard. Di MQTT Explorer, subscribe ke `pertanian/hama/kontrol` untuk melihat pesan kontrol yang dipublish Laravel.

Karena HiveMQ public broker adalah broker publik, gunakan topik unik jika ada risiko pesan tertukar dengan pengguna lain. Untuk uji kelas, ganti prefix topik pada `.env` dan firmware ESP32 secara bersamaan, misalnya `faizal/hama/status` dan `faizal/hama/kontrol`.

## Catatan integrasi firmware

Pastikan program ESP32 subscribe ke topik kontrol dan memproses key `mode`, `buzzer`, `led`, `ir`, `lonceng`. Payload status yang dikirim ESP32 perlu memakai key yang sama dengan contoh. Nilai `status` yang disarankan: `AMAN`, `WASPADA`, `BAHAYA`.

Dashboard menampilkan kondisi aktual dari pesan status yang diterima. Jika firmware tidak menyertakan `led` dalam payload status, indikator LED pada dashboard tampil `--`; hal ini tidak menghambat pengiriman perintah LED dari dashboard.

## Troubleshooting

- `Class ... MqttClient not found`: jalankan `composer install` atau `composer dump-autoload`.
- MQTT tidak connect: pastikan internet aktif, port 1883 tidak diblokir firewall, host dan port benar.
- Dashboard tidak menerima data: pastikan terminal `php artisan mqtt:listen` tetap berjalan dan ESP32 publish ke topik yang sama.
- Tombol gagal: cek apakah broker dapat dijangkau dan lihat pesan error di dashboard/`storage/logs/laravel.log`.
- Tidak ada tabel: jalankan `php artisan migrate`.
- `could not find driver`: aktifkan `pdo_sqlite` atau `pdo_pgsql` pada `php.ini` yang digunakan terminal.

## Perintah ringkas

```bash
composer install
copy .env.example .env
php artisan key:generate
php artisan migrate
php artisan mqtt:listen
# terminal kedua
php artisan serve
```
