#pragma once
extern int fakeServoUs; extern int fakeServoAttached; extern int fakeServoFirstUs; extern bool fakeServoActive;
class Servo {
 public:
  void writeMicroseconds(int us) { fakeServoUs = us; }
  void attach(int) { fakeServoAttached++; fakeServoFirstUs = fakeServoUs; fakeServoActive = true; }
  void detach() { fakeServoActive = false; }
};
