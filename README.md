# 📡 ESP32 SoftAP Web Manager

Un portal de administración ligero y autónomo para microcontroladores **ESP32** que permite configurar dinámicamente un Punto de Acceso Wi-Fi (SoftAP) mediante una interfaz web integrada, almacenando los parámetros en memoria no volátil (NVS) y monitoreando los clientes conectados en tiempo real.

---

## ✨ Características Principales

* **Configuración Persistente (NVS):** Utiliza la librería nativa `Preferences.h` para guardar el SSID, la contraseña y la dirección IP estática en la partición NVS de la memoria Flash (sin necesidad de usar la antigua librería `EEPROM`).
* **Interfaz Web Responsiva:** Panel de control embebido en memoria de programa (`PROGMEM`) con diseño adaptable a dispositivos móviles y de escritorio.
* **Monitoreo de Clientes (MAC):** Implementa `<esp_wifi.h>` a bajo nivel para listar en tiempo real el número de estaciones conectadas y sus direcciones físicas (MAC).
* **Validación contra Bloqueos:** Verifica la validez de los octetos IPv4 y la longitud de las credenciales WPA2 tanto en el navegador como en el *backend* antes de aplicar cambios.
* **Restauración de Fábrica:** Botón integrado en el panel para limpiar la memoria NVS y volver a los parámetros por defecto en un clic.

---

## 🛠️ Requisitos Previos

* **Hardware:** Placa de desarrollo basada en ESP32 (ESP32-WROOM, NodeMCU-32S, etc.).
* **Entorno de Desarrollo:** 
  * [Arduino IDE](https://www.arduino.cc/en/software) (con el core oficial `esp32` de Espressif instalado) o [PlatformIO](https://platformio.org/).
* **Librerías:** No requiere instalar librerías externas. Todas las dependencias (`WiFi.h`, `WebServer.h`, `Preferences.h`, `esp_wifi.h`) vienen incluidas en el SDK oficial de Espressif para ESP32.

---

## 🚀 Instalación y Uso

1. **Clonar el repositorio:**
   ```bash
   git clone [https://github.com/tu-usuario/esp32-softap-manager.git](https://github.com/tu-usuario/esp32-softap-manager.git)
   ```
2. **Cargar el Firmware:**
   * Abre el archivo `.ino` en Arduino IDE (o impórtalo en PlatformIO como `main.cpp` agregando `#include <Arduino.h>` al inicio).
   * Selecciona tu placa ESP32 y el puerto COM/ttyUSB correspondiente.
   * Compila y sube el código (Velocidad del monitor serie: `115200 baudios`).

3. **Primera Conexión (Valores por Defecto):**
   * Busca la red Wi-Fi desde tu teléfono o PC y conéctate con las siguientes credenciales:
     * **SSID:** `Router-ESP`
     * **Contraseña:** `claveSegura`
   * Abre tu navegador web e ingresa a la dirección:
     * **URL:** `http://192.168.200.1`

---

## 🖥️ Estructura de Endpoints HTTP

| Ruta | Método | Descripción |
| :--- | :---: | :--- |
| `/` | `GET` | Sirve el panel de administración HTML con los valores actuales y la tabla de clientes. |
| `/save` | `POST` | Valida y guarda los parámetros (`ssid`, `password`, `ip`) en la NVS y reinicia el ESP32. |
| `/reset` | `POST` | Formatea el espacio de nombres `router-cfg` en la NVS y restaura la configuración inicial. |

---

## ⚠️ Notas Técnicas

* **Seguridad WPA2:** El estándar IEEE 802.11 en el SDK de Espressif requiere que las contraseñas tengan entre **8 y 63 caracteres**. Si se guarda una contraseña en blanco (o menor a 8 caracteres), el ESP32 levantará la red como **Abierta** (sin contraseña).
* **Límite de Clientes:** Por defecto, el modo SoftAP del ESP32 soporta un máximo de **4 conexiones simultáneas** de estaciones (configurable hasta 10 mediante `WiFi.softAP(ssid, pass, channel, hidden, max_connection)`).

---

## 📄 Licencia

Este proyecto está bajo la Licencia MIT. Siéntete libre de utilizarlo, modificarlo y distribuirlo en tus propios desarrollos.
