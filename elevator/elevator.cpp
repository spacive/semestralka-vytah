#include "elevator.h"
#include "proxyswitch.h"
#include "math.h"

void Elevator::checkButtons(packet_t * packet) {
  if (packet->dataLength < 1) return;
  if (packet->data[0] != D_BTN_PRESS) return;
  const uint8_t addr = packet->peerAddr;
  
  // panel A
  if (addr == (BTN_PANEL_A | BTN_P)) {
    ledPanelA->setLed(LED_P);
    floorQueue.push(FLOOR_P);
  }
  if (addr == (BTN_PANEL_A | BTN_1)) {
    ledPanelA->setLed(LED_1);
    floorQueue.push(FLOOR_1);
  }
  if (addr == (BTN_PANEL_A | BTN_2)) {
    ledPanelA->setLed(LED_2);
    floorQueue.push(FLOOR_2);
  }
  if (addr == (BTN_PANEL_A | BTN_3)) {
    ledPanelA->setLed(LED_3);
    floorQueue.push(FLOOR_3);
  }
  if (addr == (BTN_PANEL_A | BTN_4)) {
    ledPanelA->setLed(LED_4);
    floorQueue.push(FLOOR_4);
  }

  // panel B
  if (addr == (BTN_PANEL_B | BTN_P)) {
    ledPanelB->setLed(LED_P);
    floorQueue.push(FLOOR_P);
  }
  if (addr == (BTN_PANEL_B | BTN_1)) {
    ledPanelB->setLed(LED_1);
    floorQueue.push(FLOOR_1);
  }
  if (addr == (BTN_PANEL_B | BTN_2)) {
    ledPanelB->setLed(LED_2);
    floorQueue.push(FLOOR_2);
  }
  if (addr == (BTN_PANEL_B | BTN_3)) {
    ledPanelB->setLed(LED_3);
    floorQueue.push(FLOOR_3);
  }
  if (addr == (BTN_PANEL_B | BTN_4)) {
    ledPanelB->setLed(LED_4);
    floorQueue.push(FLOOR_4);
  }
}

void Elevator::checkProximity(packet_t *packet) {
  if (packet->dataLength < 1) return;
  if (packet->peerAddr == PROXY_P) {
    proxy.addr = PROXY_P;
    proxy.proxy = packet->data[0];
  }
  if (packet->peerAddr == PROXY_1) {
    proxy.addr = PROXY_1;
    proxy.proxy = packet->data[0];
  }
  if (packet->peerAddr == PROXY_2) {
    proxy.addr = PROXY_2;
    proxy.proxy = packet->data[0];
  }
  if (packet->peerAddr == PROXY_3) {
    proxy.addr = PROXY_3;
    proxy.proxy = packet->data[0];
  }
  if (packet->peerAddr == PROXY_4) {
    proxy.addr = PROXY_4;
    proxy.proxy = packet->data[0];
  }
}

void Elevator::execute() {
  switch (state) {
    case STATE_IDLE: idle(); break;
    case STATE_START: start(); break;
    case STATE_MOVE: move(); break;
    case STATE_BREAK1: break1(); break;
    case STATE_BREAK2: break2(); break;
    case STATE_STOP: stop(); break;
    case STATE_BOARD: board(); break;
  };
}

void Elevator::idle() {
  int floor = floorQueue.pop();
  // transit to moving state if floor request available
  if (floor != -1) {
    requestedFloor = floor;
    engine->requestEncoderCount();
    state = STATE_START;
    help.printf("idle->start\r\n");
  }
}

void Elevator::start() {
  // current position from encoder
  if (!engine->isEncoderCountValid()) {
    engine->requestEncoderCount();
    return;
  }
  double actual = engine->getLastEncoderCount();
  double desired = (4 - requestedFloor) * -250.0f;

  // check if cabin is in proximty of some floor
  if (proxy.proxy == PROXY_NARROW || proxy.proxy == PROXY_WIDE) {
    // check distance from desired floor
    int distance = desiredFloor - (proxy.addr & 0x0F);
    if (abs(distance) > 1) {
      state = STATE_MOVING;
    } else if (abs(distance) == 1) {
      state = STATE_BREAK1;
    } else {
      // current floor
      if (proxy.proxy == PROXY_NARROW) {
        state = STATE_BOARD;
      } else {
        state = STATE_BREAK2;
      }
    }
  } else {

  }
}

void Elevator::move() {
  switch (movingState) {
    case MOVING_STATE_START: {
      cabin->lock();
      help.printf("requested floor:%d\r\n", requestedFloor);
      // get current position

      // start up/down according to the current position
      if (!engine->isEncoderCountValid()) {
        engine->requestEncoderCount();
        return;
      }

      double actual = engine->getLastEncoderCount();
      // normalise requested position to range 0 -> -1000
      double desired = (4 - requestedFloor) * -250.0f;
      help.printf("actual:%f\t desired:%f\r\n", actual, desired);
      if (desired < actual) {
        engine->move(-100);
      } else {
        engine->move(100);   
      }

      // switch state to 
      movingState = MOVING_STATE_LOOP;
    } break;

    case MOVING_STATE_LOOP: {
      if (((proxy.addr & 0x0F) == (requestedFloor & 0x0F)) && (proxy.proxy == PROXY_WIDE)) {
        movingState = MOVING_STATE_STOP;
        help.printf("moving->stopping\r\n");
      }
    } break;

    case MOVING_STATE_STOP: {
      engine->stop();
      state = STATE_BOARD;
      boardingDelay = 0;
    } break;
  }
}

void Elevator::boarding() {
  if (boardingDelay == 0) {
    cabin->unlock();
  }
  if (boardingDelay++ > 200) {
    state = STATE_IDLE;
  }
}

void start();
void move();
void break1();
void break2();
void stop();
void board();