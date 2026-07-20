# UGV Swarm Base Station

## Overview
An ESP32 base station will be used to send commands to and receive telemetry from three ESP32-based unmanned ground vehicles (UGVs). A laptop terminal will be used to interface with an ESP32 that will send/receive using the ESP-NOW wireless communication protocol. The hardware layout is shown below

<img width="552" height="559" alt="image" src="https://github.com/user-attachments/assets/ab21580c-0d36-4367-b1ee-e8dce1c7639c" />

## Software Layout
FreeRTOS is used to create additional tasks for the ESP32 base station that manage message queues and allow the user to switch between command mode and telemetry mode. A software layout is shown below.

<img width="884" height="442" alt="image" src="https://github.com/user-attachments/assets/70463a5d-c353-4157-b972-ba04d9f2c98e" />

## FreeRTOS Usage

FreeRTOS is a real-time operating system (RTOS) for microcontrollers that manages the concurrent execution of tasks and is primarily used to help tasks meet their deadlines. Tasks are a unit of execution implemented as a function that can be given runtime based on their priority level. Most microcontrollers, like the STM32, have single-core CPUs that allow for only one task to be executed at a time. There are different ways of determining which tasks should run first, one of which is through cooperative scheduling, as shown below

```cpp
while(1){
  task1();
  task2();
  task3();
  task4();
  task5();
}
```

With this type of scheduling, a task is only given runtime once the previous tasks have completed their work. Even if this task has high priority, it will still have to wait for previous tasks to complete their work, which is especially dangerous in medical or avionics systems. A better way to ensure that high-priority tasks are given runtime is to use preemptive scheduling. This type of scheduling allows for high-priority tasks to preempt or interrupt lower-priority tasks when needed. FreeRTOS is a preemptive scheduler that already runs on ESP32’s by default, and its setup() and loop() functions are called within a task. In the Arduino IDE you will see the following starter code

```cpp
void setup(){
}

void loop(){
}
```

These functions will then be called within a loop task

```cpp
void loop_task(){
  setup();
  while(1){
    loop();
   }
}
```

For the base station, I chose to avoid the loop task and instead used my own tasks to improve code readability. These tasks are responsible for managing message queues, enabling/disabling motors, and receiving WiFi data. 


### Tasks

- Mode Check Task (Base Station): In command mode, the user can send start or halt commands to enable or disable motors. These commands are first placed on a queue, and a command task will periodically trigger to pop the first command off the queue and broadcast it to all UGVs using an ESP-NOW message. In telemetry mode, the user will be able to monitor telemetry such as orientation, accelerometer values, vehicle separation, and battery life from all UGVs. A UGV will place a telemetry packet on a queue, and a telemetry task will periodically trigger to pop off the first packet and send it to the base station via ESP-NOW.


- Command Task (UGV and Base Station): This task will periodically trigger to execute a command from the command queue (UGV) or send a command from the command queue (Base Station)

- Telemetry Task (UGV and Base Station): This task will periodically trigger to send a telemetry message from the telemetry queue (UGV) or read a telemetry message from the telemetry queue (Base Station)

- Motor Control Task (UGV): This is a task that periodically checks a global status flag to enable or disable motors.

- WiFi Task (Built-In Task)(UGV and Base Station): Upon arrival of newly received data, the WiFi task will execute the callback function and read this data. Since ESP-NOW documentation states that the WiFi task runs at a high priority, the amount of work performed in this task should be minimized to give other tasks runtime [2]. That means the Wi-Fi task should only be reading messages and storing them, rather than reading messages and executing heavy logic thereafter. This is being done in our base station implementation by using queues to store messages and separate tasks to execute commands or display telemetry from these queues.

## Queue vs. Stack
For this design, queues are used as buffers for storing messages because they have a first-in, first-out (FIFO) structure, which preserves the arrival order of messages. If more command types were to be added to this design, then preserving the arrival order becomes even more important since older commands, which may be halt commands, will eventually be serviced once the command task has popped this command off the queue. This is shown below

<img width="869" height="562" alt="image" src="https://github.com/user-attachments/assets/dfe5d1ac-32e8-4f55-b9ef-92b66e5c8592" />

As illustrated above, the halt command is the first to arrive in the queue so it is the first command to be serviced by the command task. A last-in, first-out (LIFO) structure like a stack present an issue for servicing this halt command, as shown below

<img width="1014" height="356" alt="image" src="https://github.com/user-attachments/assets/d810808e-2d43-4f89-8a08-e9e157118f74" />

As illustrated above, the commands that arrive most recently are being serviced first. The halt command will not be serviced until all other commands above it have been removed from the stack, which will add delay to servicing this command. This delay may cause damage to our UGVs if they are moving into rough terrain, unintended areas, or are otherwise at risk of damaging themselves from continuous movement.

Queues serve as the more appropriate data structure for storing ESP-NOW messages, given the importance of preserving arrival order for faster servicing of high-priority commands like the halt command.

