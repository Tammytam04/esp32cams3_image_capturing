#include <esp_camera.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

static const char *htmlContent PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="utf-8">
    <title>ESP32-CAM Image Collection</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { height: 100vh; width: 100vw; overflow: hidden; padding: 8px; display: flex; gap: 8px; }
        .wrapper { border: solid 1px #617b5b; padding: 4px; border-radius: 4px; height: fit-content; }
        .controls { margin-top: 8px; display: flex; flex-direction: column; gap: 8px; }
        #download, #snap, #clear { height: 32px; padding: 4px 8px; border: none; border-radius: 4px; background-color: #617b5b; color: white; font-size: 16px; cursor: pointer; }
        #prefix { height: 32px; border-radius: 4px; padding: 0 8px; border: solid 1px black; outline: none; }
        #prefix:focus { border: solid 2px #617b5b; }
        #snapshots { overflow-y: auto; max-height: 100%; display: flex; flex-wrap: wrap; gap: 8px; align-content: flex-start; }
        #snapshots:empty { display: none; }
        img { width: 240px; height: 240px; background: #000; border-radius: 4px; }
    </style>
  </head>
  <body>
    <div class="wrapper">
      <img id="live" src="/stream" alt="Live Stream"/>
      <div class="controls">
        <input id="prefix" type="text" />
        <button id="download">Download</button>
        <button id="snap">Capture</button>
        <button id="clear">Clear</button>
      </div>
    </div>
    <div>
    <div id="snapshots" class="wrapper"></div>

    <script>
      const clearBtn = document.getElementById('clear');
      const snapBtn = document.getElementById('snap');
      const dlAllBtn = document.getElementById('download');
      const gallery = document.getElementById('snapshots');
      const prefixInput = document.getElementById('prefix');

      let snapshots  = [];

      clearBtn.addEventListener('click', () => {
        snapshots = [];
        gallery.innerHTML = "";
      });

      snapBtn.addEventListener('click', async () => {
        try {
          const res = await fetch('/capture');
          if (!res.ok) throw new Error('Snapshot failed');
          const blob = await res.blob();
          const filename = `${Date.now()}.jpg`;
          snapshots.push({ name: filename, blob });

          const url = URL.createObjectURL(blob);
          const img = document.createElement('img');
          img.src = url; img.alt = filename;
          document.getElementById('snapshots').prepend(img);
        } catch (err) {
          console.error(err);
          alert('Error capturing snapshot');
        }
      });

      dlAllBtn.addEventListener('click', () => {
        if (!snapshots.length) {
          alert('No snapshots to download.');
          return;
        };
        const prefix = prefixInput.value;
        snapshots.forEach(({blob, name}) => {
          const url = URL.createObjectURL(blob);
          const a = document.createElement('a');
          a.style.display = 'none';
          a.href = url;
          a.download = prefix + "_" + name;
          document.body.appendChild(a);
          a.click();
          document.body.removeChild(a);
          URL.revokeObjectURL(url);
        });
      });
    </script>
  </body>
  </html>
  )rawliteral";

static const size_t htmlContentLength = strlen_P(htmlContent);

void handleIndex(AsyncWebServerRequest *req)
{
  req->send(200, "text/html", (uint8_t *)htmlContent, htmlContentLength);
}

void handleStream(AsyncWebServerRequest *req)
{
  AsyncWebServerResponse *response = req->beginChunkedResponse("multipart/x-mixed-replace; boundary=frame", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                                                               {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      return 0;
    }

    size_t contentLength = 0;
    String header = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\n\r\n";
    memcpy(buffer, header.c_str(), header.length());
    contentLength += header.length();
    if (contentLength + fb->len < maxLen) {
      memcpy(buffer + contentLength, fb->buf, fb->len);
      contentLength += fb->len;
      String tail = "\r\n";
      memcpy(buffer + contentLength, tail.c_str(), tail.length());
      contentLength += tail.length();
    }

    esp_camera_fb_return(fb);
    return contentLength; });
  req->send(response);
}

void handleCapture(AsyncWebServerRequest *req)
{
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    req->send(503, "text/plain", "Camera capture failed");
    return;
  }
  AsyncWebServerResponse *res = req->beginResponse(200, "image/jpeg", fb->buf, fb->len);
  res->addHeader("Content-Disposition", "inline; filename=snapshot.jpg");
  req->send(res);
  esp_camera_fb_return(fb);
}

void startCameraServer()
{
  server.on("/", HTTP_GET, handleIndex);

  server.on("/stream", HTTP_GET, handleStream);

  server.on("/capture", HTTP_GET, handleCapture);

  server.begin();
}