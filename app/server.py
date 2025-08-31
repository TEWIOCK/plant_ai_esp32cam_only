from fastapi import FastAPI, File, UploadFile
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from PIL import Image
import numpy as np, io, os, time

app = FastAPI(title="Plant Disease API (ESP32-CAM only)")
app.add_middleware(CORSMiddleware, allow_origins=["*"], allow_methods=["*"], allow_headers=["*"])

MODEL_PATH = "app/model.tflite"
USE_TFLITE = os.path.exists(MODEL_PATH)
if USE_TFLITE:
    try:
        from tensorflow.lite.python.interpreter import Interpreter
    except Exception:
        from tflite_runtime.interpreter import Interpreter
    interpreter = Interpreter(model_path=MODEL_PATH); interpreter.allocate_tensors()
    in_det = interpreter.get_input_details(); out_det = interpreter.get_output_details()

def preprocess(img, in_det):
    H = in_det[0]['shape'][1]; W = in_det[0]['shape'][2]
    x = img.convert("RGB").resize((W,H))
    arr = np.array(x, dtype=np.float32) / 255.0
    return np.expand_dims(arr, 0)

@app.post("/predict")
async def predict(file: UploadFile = File(...)):
    t0 = time.time()
    raw = await file.read()
    try:
        img = Image.open(io.BytesIO(raw))
    except Exception:
        return JSONResponse(status_code=400, content={"error":"invalid image"})
    if USE_TFLITE:
        arr = preprocess(img, in_det)
        interpreter.set_tensor(in_det[0]['index'], arr)
        interpreter.invoke()
        preds = interpreter.get_tensor(out_det[0]['index'])[0]
        idx = int(np.argmax(preds)); score=float(preds[idx])
        label=f"class_{idx}"
        healthy = "healthy" in label
    else:
        # heuristic (green ratio)
        small = img.convert("RGB").resize((128,128))
        arr = np.asarray(small).astype(np.float32)
        r,g,b = arr[...,0], arr[...,1], arr[...,2]
        green_ratio = float(np.mean(g / (r+b+1e-6)))
        healthy = green_ratio >= 0.6
        label = "healthy(heuristic)" if healthy else "possibly_diseased(heuristic)"
        score = min(0.99, max(0.01, (green_ratio-0.4)/0.5))
    return {"label":label, "score":round(score,4), "healthy":bool(healthy),
            "latency_ms": round((time.time()-t0)*1000,1), "w": img.width, "h": img.height}
