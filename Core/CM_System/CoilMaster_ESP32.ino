#include "Core/CM_System/CM_System.h"

CM_System System;

void setup() {
    System.begin();
}

void loop() {
    System.update();
}