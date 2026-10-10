#pragma once

#include <stddef.h>
#include <stdint.h>

namespace wonder {

constexpr uint32_t kDoubleClickMs = 350;
constexpr uint32_t kLongPressMs = 800;
constexpr uint8_t kSonyAddress = 48;
constexpr uint32_t kCarrierHz = 40000;

enum class Button : uint8_t { A, B };
enum class Gesture : uint8_t { None, Single, Double, Long };

struct Command {
  const char *name;
  uint8_t code;
};

constexpr Command commandFor(Button button, Gesture gesture) {
  if (button == Button::A) {
    switch (gesture) {
      case Gesture::Single: return {"Video 2", 30};
      case Gesture::Double: return {"TV", 106};
      case Gesture::Long: return {"Power", 21};
      case Gesture::None: break;
    }
  } else {
    switch (gesture) {
      case Gesture::Single: return {"Volume Up", 18};
      case Gesture::Double: return {"Volume Down", 19};
      case Gesture::None:
      case Gesture::Long: break;
    }
  }
  return {nullptr, 0};
}

constexpr uint16_t sircPayload(uint8_t command, uint8_t address = kSonyAddress) {
  return static_cast<uint16_t>(command) |
         (static_cast<uint16_t>(address) << 7);
}

struct EventBatch {
  Gesture events[2] = {Gesture::None, Gesture::None};
  uint8_t count = 0;

  void add(Gesture event) {
    if (event != Gesture::None && count < 2) events[count++] = event;
  }
};

class GestureRecognizer {
 public:
  EventBatch update(uint32_t now, bool isPressed, bool wasPressed,
                    bool wasReleased, bool longPressEnabled) {
    EventBatch result;

    if (clickPending_ && now - clickAt_ >= kDoubleClickMs) {
      result.add(Gesture::Single);
      clickPending_ = false;
    }

    if (wasPressed) {
      pressed_ = true;
      longSent_ = false;
      pressAt_ = now;
    }

    if (pressed_ && isPressed && now - pressAt_ >= kLongPressMs &&
        !longSent_) {
      longSent_ = true;
      if (longPressEnabled) result.add(Gesture::Long);
    }

    if (wasReleased && pressed_) {
      pressed_ = false;
      const uint32_t heldFor = now - pressAt_;
      if (heldFor >= kLongPressMs) {
        if (!longSent_ && longPressEnabled) result.add(Gesture::Long);
        longSent_ = true;
      } else if (!longSent_) {
        if (clickPending_ && now - clickAt_ < kDoubleClickMs) {
          clickPending_ = false;
          result.add(Gesture::Double);
        } else {
          clickPending_ = true;
          clickAt_ = now;
        }
      }
    }

    return result;
  }

 private:
  bool pressed_ = false;
  bool longSent_ = false;
  bool clickPending_ = false;
  uint32_t pressAt_ = 0;
  uint32_t clickAt_ = 0;
};

}  // namespace wonder
