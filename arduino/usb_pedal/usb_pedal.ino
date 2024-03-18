/**
 * Determine whether the pedal is pressed or not by reading the different
 * photoresistors. Use the Mouse library to send a mouse click when the pedal is
 * pressed. Filter the readings using a moving average to avoid false positives.
 */

#include <Mouse.h>

constexpr int kBufferSize{5};
/// The interval to read the sensor. Affects the responsiveness of the pedal.
/// Lower values will make the pedal more responsive but will also increase the
/// probability of ghost presses (false positives).
constexpr unsigned long kInterval{5UL}; // In milliseconds

class LdrSensor {
public:
  /**
   * @brief Construct a new Ldr Sensor
   *
   * @param pin The analog pin where the sensor is connected
   * @param dark The threshold value under which (inclusive) the sensor is
   * considered pressed
   */
  LdrSensor(uint8_t pin, int dark) : mPin{pin}, mDark{dark} {
    for (auto &value : mBuffer) {
      value = 1023; // Initialize the buffer with the maximum value
    }
  }

  void read() {
    mBuffer[mBufferIndex] = analogRead(mPin);
    mBufferIndex = (mBufferIndex + 1) % kBufferSize;
  }

  bool wasJustPressed() {
    const auto pressed = isPressed();
    const auto wasAlreadyPressed = mPreviouslyPressed;
    mPreviouslyPressed = pressed;
    return pressed && !wasAlreadyPressed;
  }

  /// For debugging purposes
  int getLastReading() const {
    const auto previousIndex = (mBufferIndex - 1 + kBufferSize) % kBufferSize;
    return mBuffer[previousIndex];
  }

private:
  uint8_t mPin{};
  int mDark{};
  int mBuffer[kBufferSize]{0};
  int mBufferIndex{0};
  bool mPreviouslyPressed{false};

  int getAverage() const {
    int sum{0};
    for (auto value : mBuffer) {
      sum += value;
    }
    const float average =
        static_cast<float>(sum) / static_cast<float>(kBufferSize);
    // Round up to the nearest integer to ensure quick reaction
    // to "button up" events
    return static_cast<int>(average + 0.5F);
  }

  bool isPressed() const {
    const auto average = getAverage();
    return average <= mDark;
  }
};

void doLeftSensorAction() {
  Serial.println("Left sensor pressed");
  Mouse.move(0, 0, 1);
}

void doMiddleSensorAction() {
  Serial.println("Middle sensor pressed");
  Mouse.move(0, 0, 5);
}

void doRightSensorAction() {
  Serial.println("Right sensor pressed");
  Mouse.move(0, 0, -1);
}

void setup() {
  Serial.begin(9600);
  Mouse.begin();
}

void loop() {
  static unsigned long lastProcessing{millis()};
  const auto currentTime = millis();

  if (currentTime - lastProcessing < kInterval) {
    return;
  }
  lastProcessing = currentTime;

  // Using a 4-7 kOhm LDR and a 220 Ohm pull-down
  static LdrSensor leftSensor{A1, 1};
  static LdrSensor middleSensor{A0, 1};
  static LdrSensor rightSensor{A2, 1};
  static int measurementsToFillBuffer{0};
  static bool bufferHasBeenFilledOnce{false};

  leftSensor.read();
  middleSensor.read();
  rightSensor.read();

  // Make sure the buffer is filled up once before processing
  if (!bufferHasBeenFilledOnce) {
    if (measurementsToFillBuffer < kBufferSize) {
      ++measurementsToFillBuffer;
      return;
    }
  }
  bufferHasBeenFilledOnce = true;

  if (leftSensor.wasJustPressed()) {
    doLeftSensorAction();
  }
  if (middleSensor.wasJustPressed()) {
    doMiddleSensorAction();
  }
  if (rightSensor.wasJustPressed()) {
    doRightSensorAction();
  }
}
