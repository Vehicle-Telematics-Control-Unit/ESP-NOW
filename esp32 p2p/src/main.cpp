#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define LED_BUILTIN 2
#define DEBUG false
// #define pln(x) Serial.println(x)

/**
 * @brief makes a printable string from a uint8_t mac address array
 *
 * @param macAddr uint8_t array contains mac address parts
 * @param buffer char * to put the printable string of the mac address into
 */
void formatMacAddress(const uint8_t *macAddr, char *buffer)
{
  snprintf(buffer, 13, "%02x%02x%02x%02x%02x%02x\0", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

/**
 * @brief A function called whenever esp recieves a valid Packet
 * @param macAddr mac address of the sender of the packet
 * @param data data recieved from the sender of the mentioned above mac address
 * @param dataLen length of the data recieved
 */
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) // Called when data is received
{
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, (int)dataLen);
  memcpy(buffer, data, msgLen);

  // Ensure we are null terminated
  buffer[msgLen] = 0;

  // Format the MAC address, put into printable form
  char macStr[13];
  formatMacAddress(macAddr, macStr);

#if DEBUG
  Serial.printf("datalen without mac: %d\n", dataLen);
  Serial.printf("msglen without mac: %d\n", msgLen);
#endif

  // Send Debug log message to the serial port
  char s[300];
  buffer[dataLen] = '\0';
  Serial.write((char)(msgLen+12));
  Serial.write(macStr, 12);
  Serial.write(buffer, msgLen);
  // sprintf(s, "%c%s%s", dataLen + 12, macStr, buffer);
  // Serial.write(s, dataLen + 12 + 1);

  // Serial.write(buffer, dataLen);
  // for (int i = 0; i < dataLen; i++)
  // {
  //   Serial.print(buffer[i]);
  // }
}

/**
 * @brief A function to call when data is sent
 *
 * @param macAddr destination mac address
 * @param status send status. Available values: ESP_NOW_SEND_{SUCCESS, Failed}
 */
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
#if DEBUG
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
  digitalWrite(LED_BUILTIN, HIGH);
  // Broadcast message to every device in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message, length);

  // Print results to serial monitor
  if (result == ESP_OK)
  {
#if DEBUG
    Serial.println("Broadcast message success");
#endif
    digitalWrite(LED_BUILTIN, LOW);
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
  pinMode(LED_BUILTIN, OUTPUT);
  // Set up Serial Monitor
  Serial.begin(115200);

  // Set ESP32 in STA mode to begin with
  WiFi.mode(WIFI_STA);
#if DEBUG == true
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
#if DEBUG == true
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
#if DEBUG
  Serial.print("data_length:");
  Serial.println(data_length);
#endif

  int actual_read_length = 0;
  do
  {
    actual_read_length += Serial.readBytes(&arr[actual_read_length], data_length - actual_read_length);

    if (actual_read_length != data_length)
    {
      Serial.print("[ERROR] Actual_read_length = ");
      Serial.println(actual_read_length);

      Serial.print("[ERROR] true_data_length = ");
      Serial.println(data_length);
    }

  } while (actual_read_length < data_length);

#if DEBUG
  Serial.print("data_arr:");
  for (int i = 0; i < actual_read_length; i++)
  {
    Serial.print(arr[i]);
  }
  Serial.println();
#endif

  broadcast(arr, data_length);
  // broadcast(arr, actual_read_length);
  memset(arr, 0, data_length + 2);
}