#include <Arduino.h>
#include <esp_now.h>
#include <Wifi.h>
#include <esp_wifi.h>
#include <freeRTOS/FreeRTOS.h>
#include <freeRTOS/task.h>

void Command_Task(void *pvParams);
void Telemetry_Task(void *pvParams);
void Motor_Task(void *pvParams);
void Command_Received_Handler(const uint8_t *mac_addr, const uint8_t *data, int len);

TaskHandle_t Command_Task_ID;
TaskHandle_t Telemetry_Task_ID;
TaskHandle_t Motor_Task_ID;

struct packet{
  uint8_t packet_type = 0;
  uint8_t command_type = 0;
};

enum Command{
  START_COMMAND = 's',
  HALT_COMMAND = 'h'
};

enum Packet_Type{
  COMMAND,
  TELEMETRY
};

packet telemetry_structure;
packet telemetry_sent;
packet command_received;

uint8_t BASE_STATION_MAC_ADDR[6] = {0x30,0x76,0xF5,0xB9,0xE2,0x94};


void setup() {

Serial.begin(115200);

WiFi.mode(WIFI_STA);
esp_now_init();

esp_now_register_recv_cb(Command_Received_Handler);

esp_now_peer_info_t peerInfo = {};
memcpy(peerInfo.peer_addr, BASE_STATION_MAC_ADDR, 6);
peerInfo.encrypt = false;
peerInfo.channel = 0;
peerInfo.ifidx = WIFI_IF_STA;


esp_now_add_peer(&peerInfo);


Serial.println("=================");
Serial.println("UGV MAC Address: ");
Serial.println(WiFi.macAddress());
Serial.println("================");

constexpr configSTACK_DEPTH_TYPE STACK_SIZE_BYTES = 4096;

constexpr UBaseType_t COMMAND_TASK_PRIORITY = 5; 
constexpr UBaseType_t TELEMETRY_TASK_PRIORITY = 5;
constexpr UBaseType_t MOTOR_TASK_PRIORITY = 8;
  
xTaskCreate(
  Command_Task,
  "Command Task",
  STACK_SIZE_BYTES,
  nullptr,
  COMMAND_TASK_PRIORITY,
  &Command_Task_ID
  );

xTaskCreate(
  Telemetry_Task,
  "Telemetry Task",
  STACK_SIZE_BYTES,
  nullptr,
  TELEMETRY_TASK_PRIORITY,
  &Telemetry_Task_ID
);

xTaskCreate(
  Motor_Task,
  "Motor Task",
  STACK_SIZE_BYTES,
  nullptr,
  MOTOR_TASK_PRIORITY,
  &Motor_Task_ID
);

}



void loop() {
 vTaskDelay(pdMS_TO_TICKS(1000));
}


void Command_Task(void *pvParams){
    for(;;){
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

void Telemetry_Task(void *pvParams){
    for(;;){
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Motor_Task(void *pvParams){
    for(;;){
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

void Command_Received_Handler(const uint8_t *mac_addr, const uint8_t* data, int len){

if (len!=sizeof(command_received)){
  return;
}

memcpy(&command_received, data, len);

  Serial.println(command_received.packet_type);
  Serial.println(command_received.command_type);

}