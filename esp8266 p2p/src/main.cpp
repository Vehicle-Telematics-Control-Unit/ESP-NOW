#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "esp_now_8266_fix.h"

#define VERBOSE false

/**
 * @brief makes a printable string from a uint8_t mac address array
 *
 * @param macAddr uint8_t array contains mac address parts
 * @param buffer char * to put the printable string of the mac address into
 */
void formatMacAddress(const uint8_t *macAddr, char *buffer)
{
  snprintf(buffer, 12, "%02x%02x%02x%02x%02x%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

/**
 * @brief A function called whenever esp recieves a valid Packet
 * @param macAddr mac address of the sender of the packet
 * @param data data recieved from the sender of the mentioned above mac address
 * @param dataLen length of the data recieved
 */
void receiveCallback(u8 *macAddr, u8 *data, u8 dataLen) // Called when data is received
{
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, (int)dataLen);
  strncpy(buffer, (const char *)data, msgLen);

  // Ensure we are null terminated
  buffer[msgLen] = 0;

  // Format the MAC address, put into printable form
  char macStr[13];
  formatMacAddress(macAddr, macStr);

  // Send Debug log message to the serial port
  Serial.printf("%c%s%s\n", dataLen + 12, macStr, buffer);
}

/**
 * @brief A function to call when data is sent
 *
 * @param macAddr destination mac address
 * @param status send status. Available values: ESP_NOW_SEND_{SUCCESS, Failed}
 */
void sentCallback(u8 *macAddr, u8 status)
{
#if VERBOSE == true
  char macStr[18];
  formatMacAddress(macAddr, macStr);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
#endif
}

/**
 * @brief Broadcast a message to all Surrounders,
 * Sends message to FF:FF:FF:FF:FF:FF *a psuedo broadcast*
 *
 * @param message information to be sent to every device
 */
void broadcast(char *message, int length)
{
  // digitalWrite(LED_BUILTIN, HIGH);
  // Broadcast message to every device in range
  // uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  u8 broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // esp_now_peer_info_t peerInfo = {};
  // memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    // esp_now_add_peer(&peerInfo);
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
  }
  // Send message

  int result = esp_now_send(broadcastAddress, (u8 *)message, length);
  // Print results to serial monitor
  if (result == ESP_OK)
  {
#if VERBOSE
    Serial.println("Broadcast message success");
#endif
    // digitalWrite(LED_BUILTIN, LOW);
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    Serial.println("ESP-NOW not Init.");
  }
  else if (result == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (result == ESP_ERR_ESPNOW_INTERNAL)
  {
    Serial.println("Internal Error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Unknown error");
  }
}

void setup()
{
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, LOW);

  // Set up Serial Monitor
  Serial.begin(115200);
  // Set ESP32 in STA mode to begin with
  WiFi.mode(WIFI_STA);
#if VERBOSE == true
  Serial.println("ESP-NOW Broadcast Mode");

  // Print MAC address
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
#endif
  // Disconnect from WiFi
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK)
  {
#if VERBOSE == true
    Serial.println("ESP-NOW Init Success");
#endif
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("ESP-NOW Init Failed");
    delay(10000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    ESP.restart();
  }

  /* other setup codes here */
}

byte data_length;
char arr[256];

void loop()
{
  while (!Serial.available())
  {
  }

  data_length = Serial.read();
  Serial.readBytes(arr, data_length);

#if VERBOSE == true
  Serial.println(arr);
#endif
  broadcast(arr, data_length);

  memset(arr, 0, data_length + 1);
}