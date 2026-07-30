#pragma once
#include "OpenKNX.h"

#ifdef WMBUS_SPI

#include <WMBus.h>

// Zustandstabelle der Status-LED (Wiki "Status-LED"), Kommentare je Zustand:
// Farb-LED / einfarbige LED. Gilt identisch für Gesamtstatus und Zählerstatus X.
enum class MBusLedState : int8_t
{
    Unset = -1,   // Startwert, erzwingt das erste Anwenden
    Unconfigured, // Aus          / Aus
    NoRadio,      // Rot          / Aus
    NoData,       // Gelb         / Aus
    Alarm,        // Rot blinkend / Blinkend
    Ok,           // Grün         / An
};

class MBusChannel : public OpenKNX::Channel
{
  protected:
    uint32_t _meterId = 0;
    uint8_t _aesKey[16] = {};
    bool _hasAesKey = false;
    uint8_t _meterType = 0;

    uint32_t _lastSendValue1Time = 0;
    uint32_t _lastSendValue2Time = 0;
    uint32_t _lastSendTemp1Time  = 0;
    uint32_t _lastSendTemp2Time  = 0;  // Rücklauf oder Diff — niemals gleichzeitig
    uint32_t _lastSendPowerTime  = 0;

    bool     _statusSent      = false;
    bool     _statusActive    = false;
    uint32_t _lastValidFrameMs  = 0;
    uint32_t _watchdogStartMs   = 0;

    OpenKNX::Led::FunctionGroup *_led = nullptr;
    MBusLedState _lastLedState = MBusLedState::Unset;

    bool intervalAllows(uint32_t &lastTime);
    void setStatus(bool active);
    void loopStatus(bool configured);
    void sendValue1(float value);
    void sendValue2(float value);
    void sendTemp1(float value);
    void sendTemp2(float value);
    void sendPower(float value);
    void sendTempDiff(float value);

  public:
    MBusChannel(uint8_t index);

    void setup(bool configured) override;
    void loop(bool configured) override;
    void processInputKo(GroupObject &ko) override;
    const std::string name() override;

    uint32_t meterId() const;
    bool hasAesKey() const;
    const uint8_t *aesKey() const;
    uint8_t meterType() const;

    bool isActive();
    // gültiges Telegramm innerhalb des Watchdog-Zeitraums empfangen
    bool isValid() const;
    // noch nie Telegramm oder Watchdog-Alarm gehabt, Wartezeit also noch nicht abgelaufen
    bool isPending() const;
    void processFrame(const WMBus::Frame &frame, const WMBus::DataRecord *records, uint8_t count);

  private:
    static bool parseHexKey(const char *hex, uint8_t *out);
};

#endif // WMBUS_SPI
