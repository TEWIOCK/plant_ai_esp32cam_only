# ESP32-CAM → Wi-Fi (Phone Hotspot) → Server → LINE Alert

## Flow
```
ESP32-CAM  --Wi-Fi-->  FastAPI Server  --LINE Notify-->  User
```

## 1) Setup Server
```bash
python -m venv venv
source venv/bin/activate   # หรือ venv\Scripts\activate บน Windows
pip install -r requirements.txt
uvicorn app.server:app --host 0.0.0.0 --port 8000
```
- แก้ `LINE_TOKEN` ใน `app/server.py` เป็น Token ที่ได้จาก [LINE Notify](https://notify-bot.line.me/my/)

## 2) Setup ESP32-CAM
- เปิด `esp32cam_client/esp32cam_client.ino`
- ตั้งค่า `WIFI_SSID`, `WIFI_PASS` = Hotspot มือถือ
- ตั้ง `SERVER_IP` = IP ของคอมพ์ที่รัน server (ดูจาก `ipconfig`/`ifconfig`)
- เลือกบอร์ด: **AI Thinker ESP32-CAM**
- อัปโหลด (ต่อ IO0 → GND ขณะอัปโหลด แล้วรีเซ็ต)

## 3) ทำงาน
- ESP32-CAM จะถ่ายรูปทุก ๆ 5 วินาที → ส่งไปที่ `/predict`
- Server จะวิเคราะห์โรค (heuristic หรือโมเดล TFLite)
- ถ้าเจอว่าใบพืชอาจเป็นโรค → Server จะส่งแจ้งเตือน LINE:
  - ข้อความบอก label + score
  - แนบรูปถ่าย

## Example LINE Alert
```
⚠️ พบพืชอาจป่วย: possibly_diseased (score=0.72)
```
(พร้อมรูปจริงที่ ESP32-CAM ถ่าย)
