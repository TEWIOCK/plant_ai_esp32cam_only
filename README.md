# ESP32-CAM → Phone Hotspot → Server (Plant Disease)

ภาพรวม
```
ESP32-CAM  --Wi‑Fi (Phone Hotspot)-->  FastAPI Server  --> JSON {label, score, healthy}
```

## 1) Server
```bash
python -m venv venv
# Windows: venv\Scripts\activate
# macOS/Linux: source venv/bin/activate

pip install -r requirements-tflite-runtime.txt   # เบา (แนะนำ)
# หรือ: pip install -r requirements-tf.txt      # เต็ม

uvicorn app.server:app --host 0.0.0.0 --port 8000
# เปิด http://<SERVER_IP>:8000/docs
```
- ถ้ามีโมเดล `model.tflite` ให้วางที่ `app/model.tflite` และแก้ `labels.txt`
- ถ้าไม่มีโมเดล ระบบจะใช้ heuristic “ดูความเขียว” เพื่อลองระบบให้ก่อน

## 2) เชื่อมต่อผ่าน Hotspot
- เปิด **Phone Hotspot** และจด `SSID` / `Password`  
- ให้ **คอมพ์/โน้ตบุ๊ก** ที่รัน server **เชื่อมเข้า Hotspot เดียวกัน**  
- ดู IP เครื่องเซิร์ฟเวอร์ (เช่น 192.168.43.100 บน Android) แล้วนำไปใส่ในสเก็ตช์

## 3) ESP32-CAM
- เปิด `esp32cam_client/esp32cam_client.ino` ใน Arduino IDE
- ตั้งค่า:
  - `WIFI_SSID` = ชื่อ Hotspot
  - `WIFI_PASS` = รหัส Hotspot
  - `SERVER_IP` = IP ของคอมพ์ที่รันเซิร์ฟเวอร์ (ซึ่งต่อ Hotspot เดียวกัน)
- บอร์ด: `AI Thinker ESP32-CAM`  
- อัปโหลด (ต่อ `IO0 -> GND` เพื่อเข้าโหมดแฟลช) แล้ว **รีเซ็ต**  
- เปิด Serial Monitor จะเห็นผล JSON ที่ส่งกลับจาก `/predict`

## 4) ปรับแต่งคุณภาพ/ความเร็ว
- ลด `frame_size` เป็น `QVGA` หรือเพิ่ม `jpeg_quality` เพื่อลดขนาดไฟล์ → ส่งเร็วขึ้น
- เพิ่มช่วงส่ง `delay()` หากเซิร์ฟเวอร์รับไม่ทัน

## 5) API ผลลัพธ์
`POST /predict` (multipart/form-data, field `file`) → ตอบกลับ
```json
{ "label": "healthy|disease_x", "score": 0.92, "healthy": true, "latency_ms": 120.5, "w": 640, "h": 480 }
```

## Troubleshooting
- ต่อ Hotspot แล้ว **ESP32-CAM ควรเห็น IP ของตัวเอง** ใน Serial; ถ้าไม่ ข้อมูล SSID/PASS อาจผิด
- ถ้า `Connect server failed` ให้ตรวจ `SERVER_IP` และไฟร์วอลล์ Windows
- ภาพไม่ขึ้นหรือดีเลย์สูง: ลดความละเอียด/เพิ่มการบีบอัด

โชคดีครับ!
