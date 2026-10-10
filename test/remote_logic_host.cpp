#include <cassert>
#include <cstdint>

#include "../src/remote_logic.h"

int main() {
  using wonder::Button;
  using wonder::Gesture;

  assert(wonder::commandFor(Button::A, Gesture::Single).code == 30);
  assert(wonder::commandFor(Button::A, Gesture::Double).code == 106);
  assert(wonder::commandFor(Button::A, Gesture::Long).code == 21);
  assert(wonder::commandFor(Button::B, Gesture::Single).code == 18);
  assert(wonder::commandFor(Button::B, Gesture::Double).code == 19);
  assert(wonder::commandFor(Button::B, Gesture::Long).name == nullptr);
  assert(wonder::sircPayload(30) == (48u << 7 | 30u));

  wonder::GestureRecognizer recognizer;
  auto events = recognizer.update(100, true, true, false, true);
  assert(events.count == 0);
  events = recognizer.update(160, false, false, true, true);
  assert(events.count == 0);
  events = recognizer.update(300, true, true, false, true);
  assert(events.count == 0);
  events = recognizer.update(350, false, false, true, true);
  assert(events.count == 1 && events.events[0] == Gesture::Double);
  events = recognizer.update(700, false, false, false, true);
  assert(events.count == 0);

  events = recognizer.update(1000, true, true, false, true);
  assert(events.count == 0);
  events = recognizer.update(1800, true, false, false, true);
  assert(events.count == 1 && events.events[0] == Gesture::Long);
  events = recognizer.update(1810, false, false, true, true);
  assert(events.count == 0);

  events = recognizer.update(2000, true, true, false, false);
  assert(events.count == 0);
  events = recognizer.update(2800, true, false, false, false);
  assert(events.count == 0);
  events = recognizer.update(2810, false, false, true, false);
  assert(events.count == 0);

  events = recognizer.update(3000, true, true, false, true);
  assert(events.count == 0);
  events = recognizer.update(3010, false, false, true, true);
  assert(events.count == 0);
  events = recognizer.update(3360, false, false, false, true);
  assert(events.count == 1 && events.events[0] == Gesture::Single);
}
