#include <Arduino.h>
#include <M5Unified.h>
#include <driver/rmt_encoder.h>
#include <driver/rmt_tx.h>
#include <esp_err.h>

#include "remote_logic.h"

namespace {
constexpr gpio_num_t kTxPin = GPIO_NUM_46;
constexpr uint32_t kResolutionHz = 1000000;
constexpr unsigned kFrameCount = 3;
constexpr uint32_t kFramePeriodUs = 45000;

rmt_channel_handle_t txChannel = nullptr;
rmt_encoder_handle_t encoder = nullptr;
bool hardwareReady = false;
wonder::GestureRecognizer buttonA;
wonder::GestureRecognizer buttonB;
const char *lastSent = "Ready";

bool check(esp_err_t result, const char *operation) {
  if (result == ESP_OK) return true;
  Serial.printf("[ERROR] %s: %s (0x%x)\n", operation,
                esp_err_to_name(result), result);
  return false;
}

void render() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(4, 5);
  M5.Display.println("Wonder Remote");
  M5.Display.drawFastHLine(3, 25, 233, TFT_DARKGREY);
  M5.Display.setCursor(4, 32);
  M5.Display.println("A: Video2 / TV / Power");
  M5.Display.println("   single / double / hold");
  M5.Display.println("B: Vol Up / Vol Down");
  M5.Display.println("   single / double");
  M5.Display.println();
  M5.Display.printf("Last: %s\n", lastSent);
  M5.Display.println("Sony SIRC15 40 kHz");
  M5.Display.println("Single waits 350 ms");
}

bool initIr() {
  if (hardwareReady) return true;
  M5.Speaker.end();
  M5.Power.setExtOutput(true, m5::ext_none);
  delay(100);
  if (!M5.Power.getExtOutput()) {
    Serial.println("[ERROR] StickS3 EXT_5V did not turn on; IR is unpowered");
    return false;
  }

  rmt_tx_channel_config_t txConfig = {};
  txConfig.gpio_num = kTxPin;
  txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
  txConfig.resolution_hz = kResolutionHz;
  txConfig.mem_block_symbols = 64;
  txConfig.trans_queue_depth = 1;
  if (!check(rmt_new_tx_channel(&txConfig, &txChannel),
             "rmt_new_tx_channel")) return false;

  rmt_carrier_config_t carrier = {};
  carrier.frequency_hz = wonder::kCarrierHz;
  carrier.duty_cycle = 0.33f;
  if (!check(rmt_apply_carrier(txChannel, &carrier), "rmt_apply_carrier"))
    return false;

  rmt_copy_encoder_config_t encoderConfig = {};
  if (!check(rmt_new_copy_encoder(&encoderConfig, &encoder),
             "rmt_new_copy_encoder")) return false;
  if (!check(rmt_enable(txChannel), "rmt_enable TX")) return false;

  hardwareReady = true;
  Serial.printf("[IR] Ready: TX GPIO46, Sony SIRC15, address=%u, carrier=%lu Hz\n",
                wonder::kSonyAddress,
                static_cast<unsigned long>(wonder::kCarrierHz));
  return true;
}

bool sendSony(uint8_t command) {
  if (!hardwareReady && !initIr()) return false;

  rmt_symbol_word_t symbols[16] = {};
  symbols[0].level0 = 1;
  symbols[0].duration0 = 2400;
  symbols[0].level1 = 0;
  symbols[0].duration1 = 600;

  const uint16_t payload = wonder::sircPayload(command);
  for (size_t bit = 0; bit < 15; ++bit) {
    symbols[bit + 1].level0 = 1;
    symbols[bit + 1].duration0 =
        (payload & (uint16_t(1) << bit)) ? 1200 : 600;
    symbols[bit + 1].level1 = 0;
    symbols[bit + 1].duration1 = 600;
  }

  rmt_transmit_config_t config = {};
  config.loop_count = 0;
  config.flags.eot_level = 0;
  for (unsigned frame = 0; frame < kFrameCount; ++frame) {
    const uint32_t frameStartUs = micros();
    if (!check(rmt_transmit(txChannel, encoder, symbols, sizeof(symbols),
                            &config), "rmt_transmit Sony")) return false;
    if (!check(rmt_tx_wait_all_done(txChannel, 1000),
               "rmt_tx_wait_all_done Sony")) return false;
    if (frame + 1 < kFrameCount) {
      const uint32_t elapsedUs = micros() - frameStartUs;
      if (elapsedUs < kFramePeriodUs)
        delayMicroseconds(kFramePeriodUs - elapsedUs);
    }
  }
  return true;
}

void dispatch(wonder::Button button, wonder::Gesture gesture) {
  const wonder::Command command = wonder::commandFor(button, gesture);
  if (!command.name) return;

  Serial.printf("[TX] %s command=%u address=%u\n", command.name,
                command.code, wonder::kSonyAddress);
  if (sendSony(command.code)) {
    lastSent = command.name;
  } else {
    lastSent = "IR transmit error";
  }
  render();
}

void serviceButton(wonder::Button button, wonder::GestureRecognizer &recognizer,
                   m5::Button_Class &physicalButton, bool longPressEnabled) {
  const wonder::EventBatch events = recognizer.update(
      millis(), physicalButton.isPressed(), physicalButton.wasPressed(),
      physicalButton.wasReleased(), longPressEnabled);
  for (uint8_t i = 0; i < events.count; ++i)
    dispatch(button, events.events[i]);
}
}  // namespace

void setup() {
  auto config = M5.config();
  config.output_power = true;
  config.internal_spk = false;
  M5.begin(config);
  M5.Speaker.end();
  M5.Display.setRotation(3);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.BtnA.setHoldThresh(wonder::kLongPressMs);
  M5.BtnB.setHoldThresh(wonder::kLongPressMs);

  Serial.begin(115200);
  delay(250);
  Serial.println("[BOOT] Wonder Remote for M5StickS3");
  render();
}

void loop() {
  M5.update();
  serviceButton(wonder::Button::A, buttonA, M5.BtnA, true);
  serviceButton(wonder::Button::B, buttonB, M5.BtnB, false);
}
