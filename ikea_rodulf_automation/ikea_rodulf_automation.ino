#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

const char* ssid = "<SSID_HERE>";
const char* password = "<PASSWORD_HERE>";

const int relayUp = D5;    
const int relayDown = D6;

struct DeskConfig {
  unsigned long timeUpMs;      
  unsigned long timeDownMs;    
  bool isStanding;             
  bool autoModeActive;         
  unsigned long autoIntervalMs; 
};

DeskConfig myConfig = {9000, 9000, false, false, 1800000}; 
const char* CONFIG_PATH = "/desk_config.bin";

ESP8266WebServer server(80);
unsigned long movementStartTime = 0;   
unsigned long currentTargetDuration = 0; 
unsigned long lastAutoToggleTime = 0;   
bool isMoving = false;                 

// --- Persistence ---
void saveConfig() {
  File file = LittleFS.open(CONFIG_PATH, "w");
  if (file) {
    file.write((uint8_t*)&myConfig, sizeof(DeskConfig));
    file.close();
  }
}

void loadConfig() {
  if (LittleFS.exists(CONFIG_PATH)) {
    File file = LittleFS.open(CONFIG_PATH, "r");
    if (file) {
      file.read((uint8_t*)&myConfig, sizeof(DeskConfig));
      file.close();
    }
  }
}

// --- Motors ---
void stopMotors() {
  digitalWrite(relayUp, HIGH);
  digitalWrite(relayDown, HIGH);
  isMoving = false;
  movementStartTime = 0;
}

void startMovement(int pin, unsigned long duration, bool toStanding) {
  if (isMoving) return; 
  currentTargetDuration = duration;
  movementStartTime = millis();
  isMoving = true;
  myConfig.isStanding = toStanding;
  saveConfig(); 
  digitalWrite(pin, LOW); 
}

// --- Web Page HTML ---
void handleRoot() {
  String html = F(R"=====(
<!doctype html><html lang=en><meta charset=UTF-8><meta name=viewport content="width=device-width,initial-scale=1,viewport-fit=cover"><title>IKEA Rodulf Control Panel</title><link rel=preconnect href=https://fonts.googleapis.com><link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel=stylesheet><style>*,:before,:after{box-sizing:border-box;margin:0;padding:0}:root{--bg:#0a0a0f;--surface:#13131a;--surface-hover:#1a1a24;--border:#ffffff0f;--border-hover:#ffffff1f;--text:#e4e4e7;--text-dim:#8f8f9b;--accent:#6366f1;--accent-glow:#6366f140;--green:#22c55e;--green-dim:#22c55e1f;--amber:#f59e0b;--amber-dim:#f59e0b1f;--red:#ef4444;--red-dim:#ef44441a;--radius:16px;--radius-sm:10px}body{background:var(--bg);color:var(--text);min-height:-webkit-fill-available;padding:2rem 1rem;padding-top:max(2rem, env(safe-area-inset-top,0px));padding-bottom:max(2rem, env(safe-area-inset-bottom,0px));padding-left:max(1rem, env(safe-area-inset-left,0px));padding-right:max(1rem, env(safe-area-inset-right,0px));-webkit-font-smoothing:antialiased;font-family:Inter,-apple-system,BlinkMacSystemFont,sans-serif}.wrapper{max-width:720px;margin:0 auto}header{text-align:center;margin-bottom:2.5rem}header .logo{background:linear-gradient(135deg, var(--accent), #818cf8);width:56px;height:56px;box-shadow:0 0 32px var(--accent-glow);border-radius:16px;justify-content:center;align-items:center;margin-bottom:1rem;display:inline-flex}header .logo svg{color:#fff;width:28px;height:28px}header h1{letter-spacing:-.025em;margin-bottom:.25rem;font-size:1.5rem;font-weight:700}header p{color:var(--text-dim);font-size:.875rem;font-weight:400}.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:1.5rem;transition:border-color .2s}.card:hover{border-color:var(--border-hover)}.card+.card{margin-top:1rem}.card-title{text-transform:uppercase;letter-spacing:.06em;color:var(--text-dim);align-items:center;gap:.5rem;margin-bottom:1.25rem;font-size:.75rem;font-weight:600;display:flex}.card-title svg{opacity:.5;width:16px;height:16px}.grid-2{grid-template-columns:1fr 1fr;gap:1rem;display:grid}.badge{letter-spacing:.03em;border-radius:999px;align-items:center;gap:6px;padding:4px 12px;font-size:.75rem;font-weight:600;display:inline-flex}.badge .dot{border-radius:50%;width:7px;height:7px;display:inline-block}.badge--active{background:var(--green-dim);color:var(--green)}.badge--active .dot{background:var(--green);box-shadow:0 0 6px var(--green)}.badge--inactive{color:var(--text-dim);background:#71717a1f}.badge--inactive .dot{background:var(--text-dim)}.badge--accent{background:var(--accent-glow);color:#a5b4fc}.badge--accent .dot{background:var(--accent);box-shadow:0 0 6px var(--accent)}.status-row{justify-content:space-between;align-items:center;margin-bottom:1.25rem;display:flex}.status-row span:first-child{color:var(--text-dim);font-size:.875rem}.btn{border-radius:var(--radius-sm);cursor:pointer;border:none;outline:none;justify-content:center;align-items:center;gap:8px;width:100%;padding:.7rem 1.25rem;font-family:inherit;font-size:.8125rem;font-weight:600;transition:all .15s;display:inline-flex}.btn svg{flex-shrink:0;width:16px;height:16px}.btn--stand{background:var(--green-dim);color:var(--green)}.btn--stand:hover{background:#22c55e38}.btn--sit{background:var(--amber-dim);color:var(--amber)}.btn--sit:hover{background:#f59e0b38}.btn--stop{background:var(--red-dim);color:var(--red);text-transform:uppercase;letter-spacing:.06em;grid-column:1/-1;font-size:.75rem}.btn--stop:hover{background:#ef444433}.btn--accent{background:var(--accent);color:#fff}.btn--accent:hover{box-shadow:0 0 20px var(--accent-glow);background:#4f46e5}.btn--ghost{color:var(--text-dim);border:1px solid var(--border);background:#ffffff0a}.btn--ghost:hover{color:var(--text);border-color:var(--border-hover);background:#ffffff14}.btn--sm{padding:.5rem 1rem;font-size:.75rem}.btn-group{gap:.5rem;display:flex}.field{flex-direction:column;gap:.375rem;display:flex}.field label{text-transform:uppercase;letter-spacing:.05em;color:var(--text-dim);font-size:.6875rem;font-weight:500}.input{color:var(--text);border:1px solid var(--border);border-radius:var(--radius-sm);background:#ffffff0a;outline:none;width:100%;padding:.6rem .875rem;font-family:inherit;font-size:.875rem;font-weight:500;transition:border-color .2s,box-shadow .2s}.input:focus{border-color:var(--accent);box-shadow:0 0 0 3px var(--accent-glow)}.input::placeholder{color:var(--text-dim)}.input::-webkit-inner-spin-button,.input::-webkit-outer-spin-button{-webkit-appearance:none;margin:0}.input[type=number]{-moz-appearance:textfield}.inline-field{gap:.5rem;display:flex}.inline-field .input{flex:1}.inline-field .btn{white-space:nowrap;width:auto}.timing-grid{grid-template-columns:1fr 1fr auto;align-items:end;gap:1rem;display:grid}.toast{background:var(--surface);border:1px solid var(--border-hover);border-radius:var(--radius-sm);color:var(--text);opacity:0;pointer-events:none;z-index:100;padding:.6rem 1.25rem;font-size:.8125rem;font-weight:500;transition:all .3s cubic-bezier(.16,1,.3,1);position:fixed;bottom:1.5rem;left:50%;transform:translate(-50%)translateY(100%);box-shadow:0 8px 32px #0006}.toast.show{opacity:1;transform:translate(-50%)translateY(0)}.toast{bottom:max(1.5rem, env(safe-area-inset-bottom,0px))}@media (width<=600px){.timing-grid{grid-template-columns:1fr 1fr}.timing-grid .btn{grid-column:1/-1}}@keyframes pulse{0%,to{opacity:1}50%{opacity:.4}}.pulse{animation:1.5s ease-in-out infinite pulse}@supports not (min-height:100dvh){body{min-height:calc(var(--vh,1vh) * 100)}}</style><div class=wrapper><header><div class=logo><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M3 7.5 7.5 3m0 0L12 7.5M7.5 3v13.5m13.5-3L16.5 18m0 0L12 13.5m4.5 4.5V4.5"/></svg></div><h1>Rodulf Control</h1><p>IKEA smart desk controller</header><div class=card><div class=card-title><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor" aria-hidden="true"><path stroke-linecap="round" stroke-linejoin="round" d="M3 9h18v3H3V9Zm3 3v8m12-8v8M9 14h6m-6 3h6"/></svg> Desk Position</div><div class=status-row><span>Current position</span> <span id=heightState class="badge badge--accent pulse"><span class=dot></span>CONNECTING</span></div><div class=grid-2><button onclick='cmd("/standing")' class="btn btn--stand"><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M4.5 10.5 12 3m0 0 7.5 7.5M12 3v18"/></svg> Standing</button> <button onclick='cmd("/seated")' class="btn btn--sit"><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M19.5 13.5 12 21m0 0-7.5-7.5M12 21V3"/></svg> Seated</button> <button onclick='cmd("/stop")' class="btn btn--stop"><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M12 9v3.75m-9.303 3.376c-.866 1.5.217 3.374 1.948 3.374h14.71c1.73 0 2.813-1.874 1.948-3.374L13.949 3.378c-.866-1.5-3.032-1.5-3.898 0L2.697 16.126ZM12 15.75h.007v.008H12v-.008Z"/></svg> Emergency Stop</button></div></div><div class=card><div class=card-title><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M16.023 9.348h4.992v-.001M2.985 19.644v-4.992m0 0h4.992m-4.993 0 3.181 3.183a8.25 8.25 0 0 0 13.803-3.7M4.031 9.865a8.25 8.25 0 0 1 13.803-3.7l3.181 3.182"/></svg> Auto Mode</div><div class=status-row><span>Rotation status</span> <span id=autoState class="badge badge--inactive"><span class=dot></span>...</span></div><div class=field><label>Interval (minutes)</label><div class=inline-field style=margin-bottom:.75rem><input type=number id=autoMinutes class=input> <button onclick=setAuto() class="btn btn--accent btn--sm" style=width:auto>Apply</button></div></div><div class=btn-group><button onclick=toggleAuto(1) class="btn btn--stand btn--sm">Enable</button> <button onclick=toggleAuto(0) class="btn btn--ghost btn--sm">Disable</button></div></div><div class=card><div class=card-title><svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M12 6v6h4.5m4.5 0a9 9 0 1 1-18 0 9 9 0 0 1 18 0Z"/></svg> Motor Timings</div><div class=timing-grid><div class=field><label>Up duration (ms)</label> <input type=number id=timeUp class=input placeholder="e.g. 7500"></div><div class=field><label>Down duration (ms)</label> <input type=number id=timeDown class=input placeholder="e.g. 7500"></div><button onclick=saveTime() class="btn btn--accent" style=height:42px>Save</button></div></div></div><div id=toast class=toast></div><script>function showToast(e){const t=document.getElementById("toast");t.textContent=e,t.classList.add("show"),setTimeout(()=>t.classList.remove("show"),2200)}function updateStatus(){fetch("/status").then(e=>e.json()).then(e=>{const t=document.getElementById("heightState"),n=e.heightState.toUpperCase();t.innerHTML=`<span class="dot"></span>${n}`,t.className="STANDING"===n?"badge badge--active":"SEATED"===n?"badge badge--accent":"badge badge--inactive";const a=document.getElementById("autoState");a.innerHTML=e.autoMode?'<span class="dot"></span>ENABLED':'<span class="dot"></span>DISABLED',a.className=e.autoMode?"badge badge--active":"badge badge--inactive",document.getElementById("timeUp").value||(document.getElementById("timeUp").value=e.timeUpMs,document.getElementById("timeDown").value=e.timeDownMs,document.getElementById("autoMinutes").value=e.intervalMinutes)}).catch(()=>{document.getElementById("heightState").innerHTML='<span class="dot"></span>OFFLINE',document.getElementById("heightState").className="badge badge--inactive"})}function cmd(e){fetch(e).then(()=>{showToast(e.includes("standing")?"Moving up…":e.includes("seated")?"Moving down…":"Stopped!"),setTimeout(updateStatus,500)})}function toggleAuto(e){fetch(`/auto?enable=${e}`).then(()=>{showToast(e?"Auto mode enabled":"Auto mode disabled"),updateStatus()})}function setAuto(){const e=document.getElementById("autoMinutes").value;fetch(`/auto?interval=${e}`).then(()=>{showToast(`Interval set to ${e} min`),updateStatus()})}function saveTime(){const e=document.getElementById("timeUp").value,t=document.getElementById("timeDown").value;fetch(`/config/set?up=${e}&down=${t}`).then(()=>{showToast("Timings saved"),updateStatus()})}function setVh(){const e=window.visualViewport?window.visualViewport.height:window.innerHeight;document.documentElement.style.setProperty("--vh",.01*e+"px")}setVh(),window.addEventListener("resize",setVh),window.visualViewport&&window.visualViewport.addEventListener("resize",setVh),setInterval(updateStatus,3e3),updateStatus()</script>

)=====");
  server.send(200, "text/html", html);
}

// --- API Endpoints ---
void handleStatus() {
  String json = "{";
  json += "\"heightState\":\"" + String(myConfig.isStanding ? "standing" : "seated") + "\",";
  json += "\"autoMode\":" + String(myConfig.autoModeActive ? "true" : "false") + ",";
  json += "\"intervalMinutes\":" + String(myConfig.autoIntervalMs / 1000 / 60) + ",";
  json += "\"timeUpMs\":" + String(myConfig.timeUpMs) + ",";
  json += "\"timeDownMs\":" + String(myConfig.timeDownMs) + ",";
  json += "\"isMoving\":" + String(isMoving ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleAuto() {
  if (server.hasArg("enable")) {
    myConfig.autoModeActive = (server.arg("enable") == "1");
    lastAutoToggleTime = millis(); 
    saveConfig();
  }
  if (server.hasArg("interval")) {
    myConfig.autoIntervalMs = (unsigned long)server.arg("interval").toInt() * 60000UL;
    saveConfig();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetConfig() {
  if (server.hasArg("up")) myConfig.timeUpMs = (unsigned long)server.arg("up").toInt();
  if (server.hasArg("down")) myConfig.timeDownMs = (unsigned long)server.arg("down").toInt();
  saveConfig();
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  digitalWrite(relayUp, HIGH);
  digitalWrite(relayDown, HIGH);
  pinMode(relayUp, OUTPUT);
  pinMode(relayDown, OUTPUT);

  LittleFS.begin();
  loadConfig();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(500);
  }

  Serial.println("Connected to WiFi");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/auto", HTTP_GET, handleAuto); 
  server.on("/standing", HTTP_GET, [](){ startMovement(relayUp, myConfig.timeUpMs, true); server.send(200); });
  server.on("/seated", HTTP_GET, [](){ startMovement(relayDown, myConfig.timeDownMs, false); server.send(200); });
  server.on("/config/set", HTTP_GET, handleSetConfig);
  server.on("/stop", HTTP_GET, [](){ stopMotors(); server.send(200); });

  server.begin();
  lastAutoToggleTime = millis();

  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient();
  if (isMoving && (millis() - movementStartTime >= currentTargetDuration)) stopMotors();
  if (myConfig.autoModeActive && !isMoving && (millis() - lastAutoToggleTime >= myConfig.autoIntervalMs)) {
    if (myConfig.isStanding) startMovement(relayDown, myConfig.timeDownMs, false);
    else startMovement(relayUp, myConfig.timeUpMs, true);
    lastAutoToggleTime = millis(); 
  }
}