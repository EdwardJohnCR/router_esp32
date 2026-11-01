/**
 * @file ESP32_SoftAP_Manager.ino
 * @brief Portal de administración web para configurar un Punto de Acceso (SoftAP) en ESP32.
 * @details Permite modificar SSID, contraseña e IP estática almacenando los datos en memoria 
 *          no volátil (NVS), además de monitorear las direcciones MAC de los clientes conectados.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_wifi.h> // Librería nativa del SDK de Espressif para funciones avanzadas de Wi-Fi

// ============================================================================
// INSTANCIAS Y CONSTANTES GLOBALES
// ============================================================================
WebServer server(80);       // Servidor web en el puerto estándar HTTP (80)
Preferences preferences;    // Manejo de memoria no volátil (NVS)

// Valores de configuración por defecto (Fallback)
const char* DEFAULT_SSID = "Router-ESP";
const char* DEFAULT_PASS = "claveSegura";
const char* DEFAULT_IP   = "192.168.200.1";

// Variables de estado actual en tiempo de ejecución
String ssid;
String password;
String ipAddress;

// Objetos para configuración de capa de red
IPAddress local_IP;
IPAddress gateway;
IPAddress subnet(255, 255, 255, 0); // Máscara de subred estándar /24

// Prototipos de funciones (Necesario para compatibilidad directa con PlatformIO)
void handleRoot();
void handleSave();
void handleReset();

// ============================================================================
// INTERFAZ WEB (Almacenada en Flash con PROGMEM)
// ============================================================================
const char* html_page PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Admin - Router ESP32</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f4f4f9; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; padding: 20px; box-sizing: border-box; }
    .container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); width: 100%; max-width: 420px; }
    h2, h3 { color: #333; text-align: center; margin-top: 0; }
    h3 { margin-top: 25px; border-top: 1px solid #eee; padding-top: 20px; font-size: 17px; }
    label { font-weight: 600; color: #555; display: block; margin-bottom: 5px; font-size: 14px; }
    .hint { font-size: 12px; color: #777; margin-top: -15px; margin-bottom: 15px; display: block; }
    input[type="text"], input[type="password"] { width: 100%; padding: 10px; margin-bottom: 18px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; font-size: 14px; }
    .pass-wrapper { position: relative; }
    .toggle-pass { position: absolute; right: 10px; top: 10px; cursor: pointer; font-size: 12px; color: #007bff; user-select: none; }
    input[type="submit"], .btn-danger { background-color: #007bff; color: white; border: none; padding: 12px; border-radius: 4px; cursor: pointer; width: 100%; font-size: 15px; font-weight: bold; transition: background 0.3s; margin-bottom: 10px; display: block; text-align: center; text-decoration: none; box-sizing: border-box; }
    input[type="submit"]:hover { background-color: #0056b3; }
    .btn-danger { background-color: #dc3545; font-size: 13px; padding: 8px; margin-top: 15px; }
    .btn-danger:hover { background-color: #a71d2a; }
    ul { list-style-type: none; padding: 0; margin: 0; }
    li { background: #f8f9fa; margin-bottom: 8px; padding: 10px; border-radius: 4px; font-family: monospace; text-align: center; color: #333; border: 1px solid #e1e1e1; font-size: 14px; }
    .btn-refresh { display: block; text-align: center; margin-top: 12px; color: #28a745; text-decoration: none; font-weight: bold; font-size: 14px; }
    .btn-refresh:hover { text-decoration: underline; }
    .footer { text-align: center; margin-top: 25px; font-size: 12px; color: #888; }
  </style>
</head>
<body>
  <div class="container">
    <h2>Configuración de Red</h2>
    <form action="/save" method="POST">
      <label for="ssid">Nombre de la Red (SSID):</label>
      <input type="text" id="ssid" name="ssid" value="%SSID%" maxlength="31" required>
      
      <label for="password">Contraseña:</label>
      <div class="pass-wrapper">
        <input type="password" id="password" name="password" value="%PASSWORD%" maxlength="63">
        <span class="toggle-pass" onclick="togglePass()">Mostrar</span>
      </div>
      <span class="hint">Mínimo 8 caracteres. Dejar en blanco para red abierta.</span>
      
      <label for="ip">Dirección IP del Router:</label>
      <input type="text" id="ip" name="ip" value="%IP%" required pattern="^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)\.?\b){4}$" title="Ingresa una dirección IPv4 válida (ej. 192.168.200.1)">
      
      <input type="submit" value="Guardar y Reiniciar">
    </form>

    <h3>Dispositivos Conectados (%NUM%)</h3>
    %CLIENTS%
    <a href="/" class="btn-refresh">↻ Actualizar Lista</a>
    
    <form action="/reset" method="POST" onsubmit="return confirm('¿Restaurar valores de fábrica?');">
      <button type="submit" class="btn-danger">Restablecer valores de fábrica</button>
    </form>

    <div class="footer">ESP32 SoftAP Network Manager</div>
  </div>

  <script>
    function togglePass() {
      var x = document.getElementById("password");
      var t = document.querySelector(".toggle-pass");
      if (x.type === "password") { x.type = "text"; t.textContent = "Ocultar"; } 
      else { x.type = "password"; t.textContent = "Mostrar"; }
    }
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// CONFIGURACIÓN INICIAL (SETUP)
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Iniciar espacio de nombres "router-cfg" en memoria NVS en modo lectura/escritura (false)
  preferences.begin("router-cfg", false);
  
  // 2. Recuperar configuraciones almacenadas o cargar valores por defecto si no existen
  ssid      = preferences.getString("ssid", DEFAULT_SSID);
  password  = preferences.getString("password", DEFAULT_PASS);
  ipAddress = preferences.getString("ip", DEFAULT_IP);

  Serial.println("\n=========================================");
  Serial.println("      Iniciando ESP32 SoftAP Manager     ");
  Serial.println("=========================================");
  
  // 3. Validar y configurar la dirección IP estática del punto de acceso
  if (!local_IP.fromString(ipAddress)) {
    Serial.println("[ERROR] IP almacenada inválida. Aplicando IP por defecto...");
    ipAddress = DEFAULT_IP;
    local_IP.fromString(ipAddress);
  }
  gateway = local_IP; // En modo SoftAP aislado, el propio ESP32 actúa como puerta de enlace

  // Definir explícitamente el modo Punto de Acceso
  WiFi.mode(WIFI_AP);
  
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("[ERROR] Falló la asignación de red del SoftAP.");
  }

  // 4. Levantar el Punto de Acceso (El estándar WPA2 exige mínimo 8 caracteres)
  if (password.length() >= 8) {
    WiFi.softAP(ssid.c_str(), password.c_str());
    Serial.println("[INFO] Red iniciada con seguridad WPA2-PSK.");
  } else {
    WiFi.softAP(ssid.c_str());
    Serial.println("[WARN] Contraseña menor a 8 caracteres. Red iniciada como ABIERTA.");
  }

  Serial.println("[INFO] SSID: " + ssid);
  Serial.println("[INFO] Panel de control: http://" + WiFi.softAPIP().toString());

  // 5. Definir endpoints (rutas) del servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);

  // 6. Iniciar el servidor HTTP
  server.begin();
  Serial.println("[OK] Servidor Web en ejecución.");
}

// ============================================================================
// BUCLE PRINCIPAL (LOOP)
// ============================================================================
void loop() {
  // Escucha y despacha las peticiones HTTP entrantes de forma continua
  server.handleClient();
}

// ============================================================================
// MANEJADORES DE RUTAS HTTP (CALLBACKS)
// ============================================================================

/**
 * @brief Renderiza la página principal inyectando las variables actuales y la lista de clientes.
 */
void handleRoot() {
  String page = html_page;
  
  // Reemplazo de marcadores de posición con las variables actuales
  page.replace("%SSID%", ssid);
  page.replace("%PASSWORD%", password);
  page.replace("%IP%", ipAddress);

  // Obtener la estructura de estaciones (clientes) conectadas a la interfaz AP
  wifi_sta_list_t stationList;
  esp_wifi_ap_get_sta_list(&stationList);
  
  page.replace("%NUM%", String(stationList.num));

  // Construir dinámicamente la lista HTML de direcciones MAC
  String clientHTML = "<ul>";
  if (stationList.num == 0) {
    clientHTML += "<li>Ningún dispositivo conectado</li>";
  } else {
    for (int i = 0; i < stationList.num; i++) {
      char macStr[18];
      uint8_t* mac = stationList.sta[i].mac;
      // Formateo directo de los 6 octetos en hexadecimal
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      
      clientHTML += "<li>MAC: " + String(macStr) + "</li>";
    }
  }
  clientHTML += "</ul>";

  page.replace("%CLIENTS%", clientHTML);
  
  // Enviar respuesta HTTP 200 con el documento procesado
  server.send(200, "text/html", page);
}

/**
 * @brief Procesa el formulario POST para actualizar las credenciales y dirección IP.
 */
void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("ip")) {
    String new_ssid = server.arg("ssid");
    String new_password = server.arg("password");
    String new_ip = server.arg("ip");

    new_ssid.trim();
    new_ip.trim();

    // Validación 1: SSID no vacío y dentro del límite del estándar IEEE 802.11 (32 bytes)
    if (new_ssid.length() == 0 || new_ssid.length() > 31) {
      server.send(400, "text/plain; charset=utf-8", "Error: El SSID debe tener entre 1 y 31 caracteres.");
      return;
    }

    // Validación 2: Contraseña vacía (red abierta) o entre 8 y 63 caracteres (WPA2)
    if (new_password.length() > 0 && new_password.length() < 8) {
      server.send(400, "text/plain; charset=utf-8", "Error: La contraseña debe tener al menos 8 caracteres, o dejarse en blanco.");
      return;
    }

    // Validación 3: Comprobar que la IP sea un formato IPv4 válido antes de guardarla
    IPAddress testIP;
    if (!testIP.fromString(new_ip)) {
      server.send(400, "text/plain; charset=utf-8", "Error: El formato de la dirección IP no es válido.");
      return;
    }

    // Guardar en memoria no volátil (NVS)
    preferences.putString("ssid", new_ssid);
    preferences.putString("password", new_password);
    preferences.putString("ip", new_ip);
    preferences.end(); // Cerrar NVS de forma segura antes de reiniciar

    // Enviar pantalla de confirmación con redirección automática a la nueva IP
    String response = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>";
    response += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    response += "<meta http-equiv='refresh' content='6;url=http://" + new_ip + "/'>";
    response += "<style>body{font-family:sans-serif;text-align:center;padding:40px;background:#f4f4f9;}"
                ".box{background:white;padding:25px;border-radius:8px;max-width:400px;margin:auto;box-shadow:0 4px 8px rgba(0,0,0,0.1);}</style>";
    response += "</head><body><div class='box'>";
    response += "<h2 style='color:#28a745;'>¡Configuración Guardada!</h2>";
    response += "<p>El ESP32 se está reiniciando para aplicar los cambios.</p>";
    response += "<p><strong>1.</strong> Conéctate a la red Wi-Fi: <b>" + new_ssid + "</b></p>";
    response += "<p><strong>2.</strong> Ingresa a: <a href='http://" + new_ip + "'>http://" + new_ip + "</a></p>";
    response += "</div></body></html>";
    
    server.send(200, "text/html", response);
    
    Serial.println("[INFO] Configuración actualizada. Reiniciando el sistema...");
    delay(2000); // Dar tiempo al navegador para recibir el paquete HTTP completo
    ESP.restart(); 
  } else {
    server.send(400, "text/plain; charset=utf-8", "Error: Faltan parámetros requeridos.");
  }
}

/**
 * @brief Borra las credenciales guardadas en NVS y restaura los valores por defecto.
 */
void handleReset() {
  preferences.clear();
  preferences.end();
  
  String response = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>";
  response += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  response += "<style>body{font-family:sans-serif;text-align:center;padding:40px;background:#f4f4f9;}</style></head><body>";
  response += "<h2>Valores de fábrica restaurados</h2>";
  response += "<p>Reiniciando... Conéctate a <b>" + String(DEFAULT_SSID) + "</b> e ingresa a <b>http://" + String(DEFAULT_IP) + "</b></p>";
  response += "</body></html>";

  server.send(200, "text/html", response);
  Serial.println("[WARN] Memoria NVS limpiada. Restaurando valores de fábrica...");
  delay(2000);
  ESP.restart();
}
