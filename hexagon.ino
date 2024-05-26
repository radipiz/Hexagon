#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*commandHandler)(const uint8_t *buffer);

struct command_execution {
  commandHandler handler;
  uint8_t *paramBuffer;
};

struct serial_command {
  const uint8_t command;
  commandHandler handler;
};

const int COMMAND_BUFFER_SIZE = 66;
const int PARAM_BUFFER_SIZE = COMMAND_BUFFER_SIZE - 1;
const int PIN_LOUIE_SPEED = 3;  // D3 PWM

const uint8_t SPEED_MAX = 200;
const uint8_t SPEED_MIN = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);
  pinMode(PIN_LOUIE_SPEED, OUTPUT);
  analogWrite(PIN_LOUIE_SPEED, 80);
}

void loop() {
  uint8_t commandBuffer[COMMAND_BUFFER_SIZE];
  struct command_execution parsedCommand;

  while (Serial.available()) {
    memset(&commandBuffer, 0x0, sizeof(commandBuffer));
    Serial.readBytes(commandBuffer, sizeof(commandBuffer));
    //memset(&parsedCommand, 0x0, sizeof(struct command_execution));
    int parseResult = parseCommand(commandBuffer, &parsedCommand);
    if (parseResult == 0) {
      parsedCommand.handler(parsedCommand.paramBuffer);
    } else {
      respond_err("Unknown command");
    }
  }
}

void respond_err(const char *msg) {
  Serial.write("[err] ");
  Serial.write(msg);
  Serial.write('\n');
}

void respond_ok(const char *msg) {
  Serial.write("[ok]");
  if (strlen(msg) > 0) {
    Serial.write(" ");
    Serial.write(msg);
    Serial.write('\n');
  }
}

void nop_handler(const uint8_t *_) {}

void echo_handler(const uint8_t *buf) {
  Serial.write((char *)buf);
}

void set_speed_handler(const uint8_t *buf) {
  uint8_t wantedSpeed;
  if (buf[1] == '\0') {
    wantedSpeed = buf[0];
  } else if (buf[4] != '\0') {
    respond_err("Number too big I guarantee!");
    return;
  } else {
    int wantedSpeedFromStr = atoi((char *)buf);
    if (wantedSpeedFromStr > 255) {
      respond_err("Number is too big");
      return;
    }
    wantedSpeed = wantedSpeedFromStr & 0xff;
  }
  char response[50];
  sprintf(response, "Setting speed to %u", wantedSpeed);
  respond_ok(response);
  analogWrite(PIN_LOUIE_SPEED, wantedSpeed);
}

static const struct serial_command commands[] = {
  { .command = 0x00, .handler = nop_handler },
  { .command = 0x01, .handler = echo_handler },
  { .command = 0x10, .handler = set_speed_handler }
};

int parseCommand(uint8_t *commandBuffer, struct command_execution *result) {
  const int commandLen = sizeof(commands) / sizeof(commands[0]);
  int commandIndex = 0;
  uint8_t command = commandBuffer[0];
  bool found = false;
  for (; commandIndex < commandLen; commandIndex++) {
    if (commands[commandIndex].command == command) {
      found = true;
      break;
    }
  }
  if (found) {
    result->handler = commands[commandIndex].handler;
    result->paramBuffer = commandBuffer + sizeof(uint8_t);
    return 0;
  }
  memset(result, 0x0, sizeof(struct command_execution));
  return 1;
}