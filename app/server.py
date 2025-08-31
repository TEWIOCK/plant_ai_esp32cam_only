from fastapi import FastAPI, UploadFile, File
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware
import requests, io, time, os
from PIL import Image
import numpy as np

# ===== LINE Notify token (create at https://notify-bot.line.me/my/) =====
LINE_TOKEN = "YOUR_LINE_NOTIFY_TOKEN"

app = FastAPI(title="Plant Disease API + LINE Alert")
app.add_middleware(CORSMiddleware, allow_origins=["*"], allow_methods=["*"], allow_headers=["*"])

def send_line_message(msg: str, img_bytes: bytes = None):
    url = "https://notify-api.line.me/api/notify"
    headers = {"Authorization": f"Bearer {LINE_TOKEN}"}
    files = {"imageFile": ("leaf.jpg", img_bytes, "image/jpeg")} if img_bytes else None
    data = {"message": msg}
    try:
        r = requests.post(url, headers=headers, data=data, files=files)
        print("LINE response:", r.status_code, r.text)
    except Exception as e:
        print("LINE send error:", e)

def heuristic_check(img: Image.Image):
    small = img.convert("RGB").resize((128,128))
    arr = np.array(small).astype(np.float32)
    r,g,b = arr[...,0], arr[...,1], arr[...,2]
    green_ratio = float(np.mean(g / (r+b+1e-6)))
    healthy = green_ratio >= 0.6
    label = "healthy" if healthy else "possibly_diseased"
    score = min(0.99, max(0.01, (green_ratio-0.4)/0.5))
    return label, score, healthy

@app.post("/predict")
async def predict(file: UploadFile = File(...)):
    raw = await file.read()
    try:
        img = Image.open(io.BytesIO(raw))
    except Exception:
        return JSONResponse(status_code=400, content={"error": "invalid image"})

    t0 = time.time()
    label, score, healthy = heuristic_check(img)

    result = {
        "label": label,
        "score": round(score,4),
        "healthy": healthy,
        "latency_ms": round((time.time()-t0)*1000,1),
        "w": img.width, "h": img.height
    }

    # ถ้าเจอโรค → ส่งแจ้งเตือน LINE พร้อมรูป
    if not healthy:
        send_line_message(f"⚠️ พบพืชอาจป่วย: {label} (score={score:.2f})", raw)

    return result
