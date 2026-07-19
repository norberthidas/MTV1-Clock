#include <ESP_8_BIT_GFX.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>
#include <math.h>
#include "mtv_testcard_rgb565.h"

// false = PAL
// 8 = RGB332 framebuffer
ESP_8_BIT_GFX videoOut(false, 8);

// ==================================================
// Wi-Fi and NTP
// ==================================================

//const char *WIFI_SSID = "Xiaomi_F68D";
//const char *WIFI_PASSWORD = "Jardo72471";

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_PORTAL_TIMEOUT_MS = 300000;

const char *SETUP_AP_SSID = "MTV1_SETUP";
constexpr uint16_t DNS_PORT = 53;

WebServer webServer(80);
DNSServer dnsServer;
Preferences wifiPreferences;

String portalSSID;
String portalPassword;
bool portalCredentialsReceived = false;

const char *NTP_SERVER_1 = "pool.ntp.org";
const char *NTP_SERVER_2 = "time.cloudflare.com";
const char *NTP_SERVER_3 = "time.google.com";

// Slovakia – automatic standard time and daylight saving time
const char *TIME_ZONE =
  "CET-1CEST,M3.5.0/2,M10.5.0/3";

constexpr uint32_t WIFI_TIMEOUT_MS = 20000;
constexpr uint32_t NTP_TIMEOUT_MS = 20000;

// ==================================================
// Video output
// ==================================================

constexpr int SCREEN_WIDTH = 256;
constexpr int SCREEN_HEIGHT = 240;

constexpr int X_OFFSET = 0;
constexpr int Y_OFFSET = 0;

// The clock center is slightly below the geometric screen center
constexpr int CLOCK_X = 128 + X_OFFSET;
constexpr int CLOCK_Y = 127 + Y_OFFSET;

constexpr int MARK_RADIUS = 77;

// Monochrome output
constexpr uint8_t BLACK = 0x00;
constexpr uint8_t WHITE = 0xFF;

constexpr int MTV_LOGO_WIDTH = 48;
constexpr int MTV_LOGO_HEIGHT = 36;

const uint8_t MTV_LOGO_BITMAP[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0,
  0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x10,
  0x10, 0x00, 0x00, 0x00, 0x00, 0x08,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x04,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,

  0x47, 0x80, 0xE7, 0xFE, 0xF0, 0x72,
  0x43, 0xC1, 0xC4, 0x62, 0x60, 0x22,
  0x42, 0xC1, 0xC4, 0x62, 0x30, 0x42,
  0x42, 0xE2, 0xC0, 0x60, 0x30, 0x42,
  0x42, 0xE2, 0xC0, 0x60, 0x30, 0x42,
  0x42, 0x62, 0xC0, 0x60, 0x38, 0x42,
  0x42, 0x74, 0xC0, 0x60, 0x18, 0x82,
  0x42, 0x34, 0xC0, 0x60, 0x18, 0x82,
  0x42, 0x38, 0xC0, 0x60, 0x0D, 0x02,
  0x42, 0x18, 0xC0, 0x60, 0x0D, 0x02,
  0x42, 0x18, 0xC0, 0x60, 0x0D, 0x02,
  0x42, 0x00, 0xC0, 0x60, 0x0F, 0x02,
  0x47, 0x01, 0xE0, 0xF0, 0x06, 0x02,

  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x04,
  0x10, 0x00, 0x00, 0x00, 0x00, 0x08,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x10,
  0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0,
  0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void drawWiFiConnectedContent()
{
  videoOut.fillScreen(BLACK);

  videoOut.drawRect(
    32,
    91,
    192,
    56,
    WHITE
  );

  videoOut.setTextColor(WHITE);
  videoOut.setTextSize(1);
  videoOut.setTextWrap(false);

  videoOut.setCursor(77, 107);
  videoOut.print("WI-FI CONNECTED");

  videoOut.setCursor(80, 127);
  videoOut.print("SETTING TIME");
}

void drawWiFiConnectedScreen()
{
  videoOut.waitForFrame();
  drawWiFiConnectedContent();

  videoOut.waitForFrame();
  drawWiFiConnectedContent();

  delay(500);
}

void drawWiFiSetupContent()
{
  videoOut.fillScreen(BLACK);

  videoOut.drawRect(
    12,
    66,
    232,
    108,
    WHITE
  );

  videoOut.setTextColor(WHITE);
  videoOut.setTextSize(1);
  videoOut.setTextWrap(false);

  videoOut.setCursor(72, 78);
  videoOut.print("WI-FI SETUP");

  videoOut.setCursor(37, 100);
  videoOut.print("CONNECT TO NETWORK:");

  videoOut.setCursor(79, 118);
  videoOut.print("MTV1_SETUP");

  videoOut.setCursor(32, 140);
  videoOut.print("OPEN 192.168.4.1");

  videoOut.setCursor(59, 158);
  videoOut.print("IN YOUR BROWSER");
}

void drawWiFiSetupScreen()
{
  // Write the same static image to both framebuffers.
  videoOut.waitForFrame();
  drawWiFiSetupContent();

  videoOut.waitForFrame();
  drawWiFiSetupContent();
}

void drawFlatHand(
  float angleDegrees,
  float frontLength,
  float backLength,
  float width,
  uint8_t color) {
  float angle =
    (angleDegrees - 90.0f) * DEG_TO_RAD;

  float radialX = cosf(angle);
  float radialY = sinf(angle);

  // The perpendicular vector defines the hand width
  float tangentX = -radialY;
  float tangentY = radialX;

  float halfWidth = width * 0.5f;

  // Center of the flat front edge
  float frontX =
    CLOCK_X + radialX * frontLength;

  float frontY =
    CLOCK_Y + radialY * frontLength;

  // Center of the rear edge
  float backX =
    CLOCK_X - radialX * backLength;

  float backY =
    CLOCK_Y - radialY * backLength;

  // Four corners of the rectangular hand
  int x0 = static_cast<int>(
    backX + tangentX * halfWidth);

  int y0 = static_cast<int>(
    backY + tangentY * halfWidth);

  int x1 = static_cast<int>(
    frontX + tangentX * halfWidth);

  int y1 = static_cast<int>(
    frontY + tangentY * halfWidth);

  int x2 = static_cast<int>(
    frontX - tangentX * halfWidth);

  int y2 = static_cast<int>(
    frontY - tangentY * halfWidth);

  int x3 = static_cast<int>(
    backX - tangentX * halfWidth);

  int y3 = static_cast<int>(
    backY - tangentY * halfWidth);

  fillQuad(
    x0, y0,
    x1, y1,
    x2, y2,
    x3, y3,
    color);
}

// ==================================================
// State
// ==================================================

bool timeValid = false;
int lastDisplayedSecond = -1;

// ==================================================
// Helper functions
// ==================================================

void drawThickLine(
  int x1,
  int y1,
  int x2,
  int y2,
  int thickness,
  uint8_t color) {
  int half = thickness / 2;

  for (int offset = -half; offset <= half; offset++) {
    videoOut.drawLine(
      x1 + offset,
      y1,
      x2 + offset,
      y2,
      color);

    videoOut.drawLine(
      x1,
      y1 + offset,
      x2,
      y2 + offset,
      color);
  }
}

// Filled quadrilateral
void fillQuad(
  int x0,
  int y0,
  int x1,
  int y1,
  int x2,
  int y2,
  int x3,
  int y3,
  uint8_t color) {
  videoOut.fillTriangle(
    x0, y0,
    x1, y1,
    x2, y2,
    color);

  videoOut.fillTriangle(
    x0, y0,
    x2, y2,
    x3, y3,
    color);
}

// ==================================================
// Wi-Fi configuration portal
// ==================================================

String htmlEscape(const String &value)
{
  String escaped;
  escaped.reserve(value.length() + 16);

  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];

    if (c == '&') escaped += F("&amp;");
    else if (c == '<') escaped += F("&lt;");
    else if (c == '>') escaped += F("&gt;");
    else if (c == '\"') escaped += F("&quot;");
    else escaped += c;
  }

  return escaped;
}

String createPortalPage()
{
  String page;
  page.reserve(7000);

  page += F(
    "<!DOCTYPE html><html lang='en'><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>MTV1 Wi-Fi</title>"
    "<style>"
    "body{font-family:Arial,sans-serif;background:#111;color:#fff;margin:0;padding:24px}"
    ".box{max-width:460px;margin:auto;background:#222;padding:24px;border-radius:14px}"
    "h1{font-size:25px;margin-top:0}"
    "p{line-height:1.45;color:#ddd}"
    "label{display:block;margin-top:18px;margin-bottom:7px}"
    "select,input,button{width:100%;box-sizing:border-box;font-size:17px;padding:13px;border-radius:8px;border:1px solid #777}"
    "input,select{background:#fff;color:#000}"
    "button{margin-top:22px;background:#fff;color:#000;font-weight:bold;cursor:pointer}"
    ".small{font-size:13px;color:#aaa;margin-top:18px}"
    "</style></head><body><div class='box'>"
    "<h1>MTV1 Wi-Fi Setup</h1>"
    "<p>Select your 2.4 GHz Wi-Fi network and enter its password.</p>"
    "<form method='POST' action='/save'>"
    "<label for='ssid'>Wi-Fi network</label>"
    "<select id='ssid' name='ssid' required>"
  );

  int networkCount = WiFi.scanNetworks(false, true);

  if (networkCount <= 0) {
    page += F("<option value=''>No networks found</option>");
  } else {
    for (int i = 0; i < networkCount; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;

      page += F("<option value=\"");
      page += htmlEscape(ssid);
      page += F("\">");
      page += htmlEscape(ssid);
      page += F(" (");
      page += String(WiFi.RSSI(i));
      page += F(" dBm)</option>");
    }
  }

  WiFi.scanDelete();

  page += F(
    "</select>"
    "<label for='password'>Password</label>"
    "<input id='password' name='password' type='password' autocomplete='current-password'>"
    "<button type='submit'>Connect MTV1</button>"
    "</form>"
    "<div class='small'>ESP32 supports 2.4 GHz Wi-Fi networks only.</div>"
    "</div></body></html>"
  );

  return page;
}

void redirectToPortal()
{
  webServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  webServer.send(302, "text/plain", "");
}

bool startWiFiPortal()
{
  Serial.println();
  Serial.println("Starting Wi-Fi configuration portal...");

  drawWiFiSetupScreen();
  delay(300);

  WiFi.disconnect(true, false);
  delay(300);
  WiFi.mode(WIFI_AP_STA);
  delay(200);

  if (!WiFi.softAP(SETUP_AP_SSID)) {
    Serial.println("Failed to create the MTV1_SETUP Wi-Fi network.");
    WiFi.mode(WIFI_STA);
    return false;
  }

  IPAddress portalIP = WiFi.softAPIP();

  Serial.print("Setup network: ");
  Serial.println(SETUP_AP_SSID);
  Serial.print("Portal address: http://");
  Serial.println(portalIP);

  portalCredentialsReceived = false;
  portalSSID = "";
  portalPassword = "";

  dnsServer.start(DNS_PORT, "*", portalIP);

  webServer.on("/", HTTP_GET, []() {
    webServer.send(200, "text/html; charset=utf-8", createPortalPage());
  });

  webServer.on("/save", HTTP_POST, []() {
    portalSSID = webServer.arg("ssid");
    portalPassword = webServer.arg("password");
    portalSSID.trim();

    if (portalSSID.length() == 0) {
      webServer.send(400, "text/html; charset=utf-8",
        "<html><body><h2>SSID cannot be empty.</h2><a href='/'>Back</a></body></html>");
      return;
    }

    webServer.send(200, "text/html; charset=utf-8",
      "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "</head><body style='font-family:Arial;text-align:center;padding:30px'>"
      "<h2>Credentials received</h2>"
      "<p>MTV1 is now trying to connect to Wi-Fi.</p>"
      "<p>You may close this page.</p></body></html>");

    portalCredentialsReceived = true;
  });

  // Captive portal detection URLs used by iPhone, Android, and Windows.
  webServer.on("/hotspot-detect.html", HTTP_GET, redirectToPortal);
  webServer.on("/generate_204", HTTP_GET, redirectToPortal);
  webServer.on("/gen_204", HTTP_GET, redirectToPortal);
  webServer.on("/connecttest.txt", HTTP_GET, redirectToPortal);
  webServer.on("/ncsi.txt", HTTP_GET, redirectToPortal);
  webServer.onNotFound(redirectToPortal);

  webServer.begin();

  uint32_t portalStart = millis();

  while (!portalCredentialsReceived) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    delay(2);

    if (millis() - portalStart >= WIFI_PORTAL_TIMEOUT_MS) {
      Serial.println("Wi-Fi configuration portal timed out.");
      webServer.stop();
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      return false;
    }
  }

  // Allow enough time for the browser response to be transmitted.
  uint32_t responseWait = millis();
  while (millis() - responseWait < 1200) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    delay(2);
  }

  webServer.stop();
  dnsServer.stop();

  Serial.print("Trying to connect to: ");
  Serial.println(portalSSID);

  WiFi.begin(portalSSID.c_str(), portalPassword.c_str());

  uint32_t connectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");

    if (millis() - connectStart >= WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println();
      Serial.println("Connection with the new credentials failed.");

      WiFi.disconnect(true, false);
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      return false;
    }
  }

  Serial.println();
  Serial.print("Wi-Fi connected, IP: ");
  Serial.println(WiFi.localIP());

  // Store only credentials that were successfully verified.
  wifiPreferences.begin("wifi", false);
  wifiPreferences.putString("ssid", portalSSID);
  wifiPreferences.putString("pass", portalPassword);
  wifiPreferences.end();

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  drawWiFiConnectedScreen();
  return true;
}

bool connectWiFi()
{
  wifiPreferences.begin("wifi", true);
  String savedSSID = wifiPreferences.getString("ssid", "");
  String savedPassword = wifiPreferences.getString("pass", "");
  wifiPreferences.end();

  if (savedSSID.length() == 0) {
    Serial.println("Wi-Fi credentials have not been saved yet.");
    return startWiFiPortal();
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  Serial.print("Trying saved Wi-Fi network: ");
  Serial.println(savedSSID);

  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

  uint32_t startTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");

    if (millis() - startTime >= WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println();
      Serial.println("Saved Wi-Fi network is not available.");

      WiFi.disconnect(true, false);
      delay(200);
      return startWiFiPortal();
    }
  }

  Serial.println();
  Serial.print("Wi-Fi connected, IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

// ==================================================
// NTP
// ==================================================

bool synchronizeNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  configTzTime(
    TIME_ZONE,
    NTP_SERVER_1,
    NTP_SERVER_2,
    NTP_SERVER_3);

  Serial.print("Waiting for NTP time");

  struct tm timeInfo;
  uint32_t startTime = millis();

  while (!getLocalTime(&timeInfo, 1000)) {
    Serial.print(".");

    if (millis() - startTime >= NTP_TIMEOUT_MS) {
      Serial.println();
      Serial.println("NTP synchronization failed.");
      return false;
    }
  }

  Serial.println();

  Serial.printf(
    "NTP cas: %02d:%02d:%02d  %02d.%02d.%04d\n",
    timeInfo.tm_hour,
    timeInfo.tm_min,
    timeInfo.tm_sec,
    timeInfo.tm_mday,
    timeInfo.tm_mon + 1,
    timeInfo.tm_year + 1900);

  timeValid = true;
  return true;
}

// ==================================================
// Startup test card
// ==================================================
void drawMonoscope() {
  videoOut.waitForFrame();
  videoOut.fillScreen(BLACK);

  videoOut.drawRGBBitmap(
    0,
    0,
    MTV_TESTCARD_RGB565,
    MTV_TESTCARD_RGB565_WIDTH,
    MTV_TESTCARD_RGB565_HEIGHT);
}

// ==================================================
// NTP synchronization screen
// ==================================================

void drawTimeSyncContent()
{
  videoOut.fillScreen(BLACK);

  videoOut.drawRect(
    23,
    88,
    210,
    60,
    WHITE
  );

  videoOut.setTextColor(WHITE);
  videoOut.setTextSize(1);
  videoOut.setTextWrap(false);

  videoOut.setCursor(77, 106);
  videoOut.print("SETTING TIME");

  videoOut.setCursor(95, 126);
  videoOut.print("MTV 1");
}

void drawTimeSyncScreen()
{
  videoOut.waitForFrame();
  drawTimeSyncContent();

  videoOut.waitForFrame();
  drawTimeSyncContent();
}

// ==================================================
// MTV logo in the upper-left corner
// ==================================================


void drawMTVLogo() {
  constexpr int LOGO_X = 3;
  constexpr int LOGO_Y = 13;

  constexpr int ORIGINAL_HEIGHT = 36;
  constexpr int DISPLAY_HEIGHT = 39;

  drawBitmapVerticalScaled(
    LOGO_X,
    LOGO_Y,
    MTV_LOGO_BITMAP,
    MTV_LOGO_WIDTH,
    ORIGINAL_HEIGHT,
    DISPLAY_HEIGHT,
    WHITE);
}


// ==================================================
// Striped channel number 1 in the upper-right corner
// ==================================================

void drawChannelOne() {
  constexpr int BASE_X = 235;
  constexpr int TOP_Y = 17;
  constexpr int HEIGHT = 39;

  // Five thin vertical stripes
  for (int i = 0; i < 5; i++) {
    int x = BASE_X + i * 2;

    videoOut.drawLine(
      x,
      TOP_Y + 5,
      x,
      TOP_Y + HEIGHT,
      WHITE);

    // Slanted top of the number one
    videoOut.drawLine(
      x - 13,
      TOP_Y + 12 - i,
      x,
      TOP_Y + 5,
      WHITE);
  }

  // Short finishing segment of the slanted top
  videoOut.drawLine(
    BASE_X - 14,
    TOP_Y + 12,
    BASE_X - 10,
    TOP_Y + 10,
    WHITE);
}


// ==================================================
// One rotated rectangular hour marker
// ==================================================

void drawRotatedHourMark(
  float angleDegrees,
  float centerRadius,
  float markLength,
  float markWidth,
  float sidewaysOffset = 0.0f) {
  float angle =
    (angleDegrees - 90.0f) * DEG_TO_RAD;

  // Radial direction from the clock center
  float radialX = cosf(angle);
  float radialY = sinf(angle);

  // Direction perpendicular to the marker
  float tangentX = -radialY;
  float tangentY = radialX;

  float centerX =
    CLOCK_X + radialX * centerRadius + tangentX * sidewaysOffset;

  float centerY =
    CLOCK_Y + radialY * centerRadius + tangentY * sidewaysOffset;

  float halfLength = markLength * 0.5f;
  float halfWidth = markWidth * 0.5f;

  int x0 = static_cast<int>(
    centerX - radialX * halfLength - tangentX * halfWidth);

  int y0 = static_cast<int>(
    centerY - radialY * halfLength - tangentY * halfWidth);

  int x1 = static_cast<int>(
    centerX + radialX * halfLength - tangentX * halfWidth);

  int y1 = static_cast<int>(
    centerY + radialY * halfLength - tangentY * halfWidth);

  int x2 = static_cast<int>(
    centerX + radialX * halfLength + tangentX * halfWidth);

  int y2 = static_cast<int>(
    centerY + radialY * halfLength + tangentY * halfWidth);

  int x3 = static_cast<int>(
    centerX - radialX * halfLength + tangentX * halfWidth);

  int y3 = static_cast<int>(
    centerY - radialY * halfLength + tangentY * halfWidth);

  fillQuad(
    x0, y0,
    x1, y1,
    x2, y2,
    x3, y3,
    WHITE);
}


// ==================================================
// MTV1 hour markers
// ==================================================
void drawHourMarks() {
  for (int hour = 0; hour < 12; hour++) {
    float angleDegrees = hour * 30.0f;

    // 12 o’clock position – two narrow bars side by side
    if (hour == 0) {
      drawRotatedHourMark(  // first bar at 12 o’clock
        angleDegrees,
        MARK_RADIUS + 14,  // radial position
        40.0f,             // length
        3.0f,              // width of one bar
        -3.5f              // shift left
      );

      drawRotatedHourMark(  // second bar at 12 o’clock
        angleDegrees,
        MARK_RADIUS + 14,  // radial position
        40.0f,             // length
        3.0f,              // width of one bar
        3.5f               // shift right
      );

      continue;
    }

    // Main markers at 3, 6, and 9 o’clock
    if (hour == 3 || hour == 6 || hour == 9) {
      drawRotatedHourMark(
        angleDegrees,
        MARK_RADIUS + 14,  // radial position
        40.0f,             // length
        3.0f               // width
      );
    } else {
      // Remaining hour markers
      drawRotatedHourMark(
        angleDegrees,
        MARK_RADIUS + 3,  // radial position
        25.0f,            // length
        3.0f              // length
      );
    }
  }
}

// ==================================================
// Polygonal hour/minute hand
// ==================================================

void drawTaperedHand(
  float angleDegrees,
  float frontLength,
  float backLength,
  float baseHalfWidth,
  float tipHalfWidth,
  uint8_t color) {
  float angle =
    (angleDegrees - 90.0f) * DEG_TO_RAD;

  float sideAngle =
    angle + PI / 2.0f;

  int backX =
    CLOCK_X - static_cast<int>(cosf(angle) * backLength);

  int backY =
    CLOCK_Y - static_cast<int>(sinf(angle) * backLength);

  int baseLeftX =
    CLOCK_X + static_cast<int>(cosf(sideAngle) * baseHalfWidth);

  int baseLeftY =
    CLOCK_Y + static_cast<int>(sinf(sideAngle) * baseHalfWidth);

  int baseRightX =
    CLOCK_X - static_cast<int>(cosf(sideAngle) * baseHalfWidth);

  int baseRightY =
    CLOCK_Y - static_cast<int>(sinf(sideAngle) * baseHalfWidth);

  float tipBodyLength = frontLength - 5.0f;

  int tipLeftX =
    CLOCK_X + static_cast<int>(cosf(angle) * tipBodyLength) + static_cast<int>(cosf(sideAngle) * tipHalfWidth);

  int tipLeftY =
    CLOCK_Y + static_cast<int>(sinf(angle) * tipBodyLength) + static_cast<int>(sinf(sideAngle) * tipHalfWidth);

  int tipRightX =
    CLOCK_X + static_cast<int>(cosf(angle) * tipBodyLength) - static_cast<int>(cosf(sideAngle) * tipHalfWidth);

  int tipRightY =
    CLOCK_Y + static_cast<int>(sinf(angle) * tipBodyLength) - static_cast<int>(sinf(sideAngle) * tipHalfWidth);

  int tipX =
    CLOCK_X + static_cast<int>(cosf(angle) * frontLength);

  int tipY =
    CLOCK_Y + static_cast<int>(sinf(angle) * frontLength);

  // Rear part of the hand
  videoOut.fillTriangle(
    backX,
    backY,
    baseLeftX,
    baseLeftY,
    baseRightX,
    baseRightY,
    color);

  // Main body of the hand
  fillQuad(
    baseLeftX,
    baseLeftY,
    tipLeftX,
    tipLeftY,
    tipRightX,
    tipRightY,
    baseRightX,
    baseRightY,
    color);

  // Tip of the hand
  videoOut.fillTriangle(
    tipLeftX,
    tipLeftY,
    tipX,
    tipY,
    tipRightX,
    tipRightY,
    color);
}


// ==================================================
// Second hand
// ==================================================

void drawSecondHand(float angleDegrees) {
  float angle =
    (angleDegrees - 90.0f) * DEG_TO_RAD;

  constexpr float FRONT_LENGTH = 105.0f;  // second hand length
  constexpr float BACK_LENGTH = 16.0f;

  int tipX =
    CLOCK_X + static_cast<int>(cosf(angle) * FRONT_LENGTH);

  int tipY =
    CLOCK_Y + static_cast<int>(sinf(angle) * FRONT_LENGTH);

  int backX =
    CLOCK_X - static_cast<int>(cosf(angle) * BACK_LENGTH);

  int backY =
    CLOCK_Y - static_cast<int>(sinf(angle) * BACK_LENGTH);

  // Thin second hand
  videoOut.drawLine(
    backX,
    backY,
    tipX,
    tipY,
    WHITE);

  // Small counterweight behind the center
  /*
  int weightX =
    CLOCK_X - static_cast<int>(cosf(angle) * 14.0f);

  int weightY =
    CLOCK_Y - static_cast<int>(sinf(angle) * 14.0f);

  videoOut.drawCircle(
    weightX,
    weightY,
    2,
    WHITE);
*/
}



// ==================================================
// Complete MTV1 clock
// ==================================================

void drawMTV1Clock(const struct tm &timeInfo) {
  int hour = timeInfo.tm_hour;
  int minute = timeInfo.tm_min;
  int second = timeInfo.tm_sec;

  videoOut.waitForFrame();
  videoOut.fillScreen(BLACK);

  drawMTVLogo();
  drawChannelOne();
  drawHourMarks();

  float secondAngle =
    second * 6.0f;

  float minuteAngle =
    minute * 6.0f + second * 0.1f;

  float hourAngle =
    (hour % 12) * 30.0f + minute * 0.5f + second / 120.0f;

  // Hour hand – shorter and wider
  drawFlatHand(
    hourAngle,
    70.0f,  // length dopredu
    16.0f,  // length dozadu
    5.0f,   // total width
    WHITE);

  // Minute hand – longer and narrower
  drawFlatHand(
    minuteAngle,
    98.0f,  // length dopredu
    16.0f,  // length dozadu
    5.0f,   // total width
    WHITE);

  drawSecondHand(secondAngle);

  videoOut.fillCircle(
    CLOCK_X,
    CLOCK_Y,
    6,
    WHITE);

  videoOut.fillCircle(
    CLOCK_X,
    CLOCK_Y,
    2,
    BLACK);
}

// ==================================================
// Time synchronization error
// ==================================================

void drawTimeError() {
  videoOut.waitForFrame();
  videoOut.fillScreen(BLACK);

  videoOut.drawRect(28, 92, 200, 54, WHITE);

  videoOut.setTextColor(WHITE);
  videoOut.setTextSize(1);
  videoOut.setTextWrap(false);

  videoOut.setCursor(88, 115);
  videoOut.print("CHYBA CASU");
}

void drawBitmapVerticalScaled(
  int x,
  int y,
  const uint8_t *bitmap,
  int sourceWidth,
  int sourceHeight,
  int outputHeight,
  uint8_t color) {
  const int bytesPerRow = (sourceWidth + 7) / 8;

  for (int outputY = 0; outputY < outputHeight; outputY++) {
    // Map the output row to the original bitmap row
    int sourceY =
      (outputY * sourceHeight) / outputHeight;

    for (int sourceX = 0; sourceX < sourceWidth; sourceX++) {
      int byteIndex =
        sourceY * bytesPerRow + sourceX / 8;

      uint8_t bitmapByte =
        pgm_read_byte(&bitmap[byteIndex]);

      uint8_t bitMask =
        0x80 >> (sourceX & 7);

      if (bitmapByte & bitMask) {
        videoOut.drawPixel(
          x + sourceX,
          y + outputY,
          color);
      }
    }
  }
}

// ==================================================
// Setup
// ==================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  videoOut.begin();
  videoOut.fillScreen(BLACK);

  // Startup test card
  drawMonoscope();
  delay(5000);

  drawTimeSyncScreen();

  if (connectWiFi()) {
    synchronizeNTP();
  }

  if (!timeValid) {
    drawTimeError();
  }

  lastDisplayedSecond = -1;
}

// ==================================================
// Loop
// ==================================================

void loop() {
  if (!timeValid) {
    delay(1000);
    return;
  }

  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 50)) {
    Serial.println("Failed to read local time.");
    delay(1000);
    return;
  }

  if (timeInfo.tm_sec != lastDisplayedSecond) {
    lastDisplayedSecond = timeInfo.tm_sec;
    drawMTV1Clock(timeInfo);
  }

  delay(10);
}