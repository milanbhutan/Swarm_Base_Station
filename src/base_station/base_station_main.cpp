#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//Base Station MAC address: 30:76:F5:B9:E2:94
//UGV_1 MAC address (ESP32-S3): A4:CB:8F:D9:2D:D8


struct packet{
uint8_t packet_type = 0;
uint8_t command_type = 0;
};


void Command_Task(void *pCom_Params);
void Telemetry_Task(void *pTelem_Params);
void Mode_Check_Task(void *pMode_Check_Params);

TaskHandle_t Command_Task_ID = nullptr;
TaskHandle_t Telemetry_Task_ID = nullptr;
TaskHandle_t Mode_Check_ID = nullptr;

enum Mode{
COMMAND_MODE = 'c',
TELEMETRY_MODE = 't',
EXIT = 'e'
};

enum Command{
START_COMMAND = 's',
HALT_COMMAND = 'h'
};

enum Packet_Type{
COMMAND,
TELEMETRY
};

packet start_command;
packet halt_command;
packet command_sent;


uint8_t UGV_MAC_ADDR[6] = {0xA4,0xCB,0x8F,0xD9,0x2D,0xD8};

QueueHandle_t Packet_Queue_ID = xQueueCreate(10 , sizeof(packet));

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, UGV_MAC_ADDR, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;


  esp_now_add_peer(&peerInfo);
  

  Serial.println("=====================");
  Serial.println("Base Station MAC Address"); 
  Serial.println(WiFi.macAddress());
  Serial.println("=====================");
  
  
  

  constexpr configSTACK_DEPTH_TYPE STACK_SIZE_BYTES = 4096;
  constexpr UBaseType_t COMMAND_TASK_PRIORITY = 5;
  constexpr UBaseType_t TELEMETRY_TASK_PRIORITY = 5;
  constexpr UBaseType_t MODE_CHECK_TASK_PRIORITY = 3;

  start_command.packet_type = COMMAND;
  start_command.command_type = START_COMMAND;

  halt_command.packet_type = COMMAND;
  halt_command.command_type = HALT_COMMAND;
  
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
    Mode_Check_Task,
    "Mode Check Task",
    STACK_SIZE_BYTES,
    nullptr,
    MODE_CHECK_TASK_PRIORITY,
    &Mode_Check_ID
  );

}


void loop(){
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void Command_Task(void *pCom_Params){
  for(;;){
    xQueueReceive(Packet_Queue_ID, &command_sent, portMAX_DELAY);
    Serial.println(command_sent.packet_type);
    Serial.println(command_sent.command_type);
    esp_now_send(UGV_MAC_ADDR, (uint8_t *)&command_sent, sizeof(command_sent));
    vTaskDelay(pdMS_TO_TICKS(10));
  }

}

void Telemetry_Task(void *pTelem_Params){
  for(;;){
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void Mode_Check_Task(void *pMode_Check_Params){
 
  
  for(;;){
    if(Serial.available()>0){
      switch(Serial.read()){
        case COMMAND_MODE:{
          Serial.println("Command Mode");
          int exit_status=false;
          while(!exit_status){
            if(Serial.available()>0){  
              switch(Serial.read()){
                case START_COMMAND:{
                  Serial.println("Sending Start Command");
                  xQueueSend(Packet_Queue_ID, &start_command, portMAX_DELAY);
                  break;
                }
                case HALT_COMMAND:{
                  Serial.println("Sending Halt Command");
                  xQueueSend(Packet_Queue_ID, &halt_command, portMAX_DELAY);
                  break;
                }
                case EXIT:{
                  Serial.println("Exiting Command Mode");
                  exit_status=true;
                  break;
                }
                default:{
                  Serial.println("Invalid Command");
                }
              }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
          }
          break;
        }
        case TELEMETRY_MODE:{
          Serial.println("Telemetry Mode");
          break;
        }
        default:{
          Serial.println("Invalid Mode");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

}

