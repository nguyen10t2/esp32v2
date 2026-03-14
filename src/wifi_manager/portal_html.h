#ifndef PORTAL_HTML_H
#define PORTAL_HTML_H

/**
 * @brief HTML trang cấu hình Wi-Fi (Captive Portal).
 *
 * Người dùng kết nối vào AP "ESP32", truy cập 192.168.4.1
 * và nhập SSID + Password của mạng Wi-Fi cần kết nối.
 */
static const char WIFI_PORTAL_HTML[] =
    "<!DOCTYPE html>"
    "<html lang='vi'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP32 WiFi Setup</title>"

    "<style>"
    "*{box-sizing:border-box;margin:0;padding:0}"

    "body{"
    "font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;"
    "background:#f4f6fb;"
    "color:#333;"
    "min-height:100vh;"
    "display:flex;"
    "align-items:center;"
    "justify-content:center"
    "}"

    ".card{"
    "background:#fff;"
    "border-radius:14px;"
    "padding:28px;"
    "width:90%%;"
    "max-width:420px;"
    "box-shadow:0 10px 30px rgba(0,0,0,0.12)"
    "}"

    ".logo{"
    "text-align:center;"
    "margin-bottom:16px"
    "}"

    ".logo svg{"
    "width:56px;"
    "height:56px;"
    "fill:#2196f3"
    "}"

    "h1{"
    "text-align:center;"
    "font-size:1.3em;"
    "margin-bottom:4px;"
    "color:#2196f3"
    "}"

    ".subtitle{"
    "text-align:center;"
    "font-size:0.85em;"
    "color:#666;"
    "margin-bottom:20px"
    "}"

    "label{"
    "display:block;"
    "font-size:0.85em;"
    "margin-top:14px;"
    "margin-bottom:4px"
    "}"

    "input[type=text],input[type=password]{"
    "width:100%%;"
    "padding:11px 12px;"
    "border:1px solid #ccc;"
    "border-radius:8px;"
    "font-size:0.95em;"
    "}"

    "input:focus{"
    "outline:none;"
    "border-color:#2196f3"
    "}"

    ".pw-wrap{position:relative}"

    ".toggle{"
    "position:absolute;"
    "right:10px;"
    "top:50%%;"
    "transform:translateY(-50%%);"
    "background:none;"
    "border:none;"
    "cursor:pointer;"
    "color:#777;"
    "font-size:1em"
    "}"

    "button[type=submit]{"
    "margin-top:20px;"
    "width:100%%;"
    "padding:12px;"
    "border:none;"
    "border-radius:8px;"
    "background:#2196f3;"
    "color:white;"
    "font-size:1em;"
    "cursor:pointer"
    "}"

    "button[type=submit]:hover{background:#1976d2}"

    ".scan-btn{"
    "margin-top:8px;"
    "padding:7px 12px;"
    "border:1px solid #2196f3;"
    "border-radius:8px;"
    "background:white;"
    "color:#2196f3;"
    "cursor:pointer;"
    "font-size:0.85em"
    "}"

    ".scan-btn:hover{"
    "background:#2196f3;"
    "color:white"
    "}"

    ".scan-list{"
    "margin-top:8px;"
    "max-height:150px;"
    "overflow-y:auto;"
    "border:1px solid #ddd;"
    "border-radius:8px;"
    "background:#fff"
    "}"

    ".scan-item{"
    "padding:9px 12px;"
    "border-bottom:1px solid #eee;"
    "cursor:pointer;"
    "font-size:0.9em"
    "}"

    ".scan-item:hover{background:#e3f2fd}"

    ".scan-item:last-child{border-bottom:none}"

    ".status{"
    "text-align:center;"
    "margin-top:14px;"
    "font-size:0.9em;"
    "min-height:1.2em"
    "}"

    ".ok{color:#2e7d32}"
    ".err{color:#c62828}"

    ".spinner{"
    "display:inline-block;"
    "width:14px;"
    "height:14px;"
    "border:2px solid #2196f3;"
    "border-top-color:transparent;"
    "border-radius:50%%;"
    "animation:spin .7s linear infinite;"
    "margin-right:6px;"
    "}"

    "@keyframes spin{to{transform:rotate(360deg)}}"

    "</style>"
    "</head>"

    "<body>"

    "<div class='card'>"

    "<div class='logo'>"
    "<svg viewBox='0 0 24 24'>"
    "<path d='M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z'/>"
    "</svg>"
    "</div>"

    "<h1>ESP32 WiFi Setup</h1>"
    "<p class='subtitle'>Cau hinh ket noi WiFi cho thiet bi</p>"

    "<form id='wf'>"

    "<label>SSID</label>"
    "<input id='ssid' name='ssid' type='text' placeholder='Nhap hoac chon mang WiFi' required>"

    "<button type='button' class='scan-btn' onclick='doScan()'>Quet mang WiFi</button>"
    "<div id='scanlist' class='scan-list' style='display:none'></div>"

    "<label>Mat khau</label>"

    "<div class='pw-wrap'>"
    "<input id='pass' name='password' type='password' placeholder='Nhap mat khau'>"
    "<button type='button' class='toggle' onclick='togglePw()'>&#128065;</button>"
    "</div>"

    "<button type='submit'>Ket noi</button>"

    "</form>"

    "<div id='status' class='status'></div>"

    "</div>"

    "<script>"

    "function togglePw(){"
    "var p=document.getElementById('pass');"
    "p.type=p.type==='password'?'text':'password'"
    "}"

    "function doScan(){"

    "var sl=document.getElementById('scanlist');"
    "sl.style.display='block';"
    "sl.innerHTML='<div class=\"scan-item\"><span class=\"spinner\"></span>Dang quet...</div>';"

    "fetch('/scan').then(r=>r.json()).then(d=>{"

    "if(!d.length){"
    "sl.innerHTML='<div class=\"scan-item\">Khong tim thay mang</div>';"
    "return"
    "}"

    "sl.innerHTML='';"

    "d.forEach(n=>{"

    "var div=document.createElement('div');"
    "div.className='scan-item';"
    "div.textContent=n.ssid+' ('+n.rssi+' dBm)';"

    "div.onclick=function(){"
    "document.getElementById('ssid').value=n.ssid;"
    "sl.style.display='none'"
    "};"

    "sl.appendChild(div)"

    "})"

    "}).catch(()=>{"
    "sl.innerHTML='<div class=\"scan-item\">Loi quet mang</div>'"
    "})"

    "}"

    "document.getElementById('wf').onsubmit=function(e){"

    "e.preventDefault();"

    "var st=document.getElementById('status');"
    "st.innerHTML='<span class=\"spinner\"></span>Dang ket noi...';"

    "var fd=new FormData(this);"

    "fetch('/connect',{"
    "method:'POST',"
    "body:new URLSearchParams(fd)"
    "})"

    ".then(r=>r.json())"
    ".then(d=>{"

    "if(d.status==='ok'){"
    "st.className='status ok';"
    "st.textContent='Ket noi thanh cong! IP: '+d.ip"
    "}"
    "else{"
    "st.className='status err';"
    "st.textContent='Loi: '+d.message"
    "}"

    "})"

    ".catch(()=>{"
    "st.className='status err';"
    "st.textContent='Khong the ket noi den ESP32'"
    "})"

    "}"

    "</script>"

    "</body>"
    "</html>";

#endif
