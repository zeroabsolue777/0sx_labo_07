#include "Alarm.h"
#include <Arduino.h>

Alarm::Alarm(int rPin, int gPin, int bPin, int buzzerPin, float& distanceRef)
    : _rPin(rPin), _gPin(gPin), _bPin(bPin), _buzzerPin(buzzerPin), _distance(&distanceRef),
      _state(OFF), _lastUpdate(0), _lastDetectedTime(0), _testStartTime(0),
      _currentColor(false), _distanceTrigger(15.0), _timeoutDelay(3000), _variationRate(200)
{
    pinMode(_rPin, OUTPUT);
    pinMode(_gPin, OUTPUT);
    pinMode(_bPin, OUTPUT);
    pinMode(_buzzerPin, OUTPUT);

    // Default gyrophare colors: red and blue
    _colA[0] = 255; _colA[1] = 0; _colA[2] = 0;
    _colB[0] = 0;   _colB[1] = 0; _colB[2] = 255;
}

void Alarm::setDistance(float dist) {
    _distanceTrigger = dist;
}

AlarmState Alarm::getState() const {
    return _state;
}

void Alarm::update() {
    _currentTime = millis();

    switch (_state) {
        case OFF:
            _offState();
            break;
        case WATCHING:
            _watchState();
            break;
        case ON:
            _onState();
            break;
        case TESTING:
            _testingState();
            break;
    }
}

void Alarm::_offState() {
    _setRGB(0, 0, 0);
    noTone(_buzzerPin);
    if (*_distance <= _distanceTrigger) {
        _state = ON;
        _lastDetectedTime = _currentTime;
    }
}

void Alarm::_watchState() {
    if (*_distance <= _distanceTrigger) {
        _state = ON;
        _lastDetectedTime = _currentTime;
    } else {
        _setRGB(0, 0, 0);
        noTone(_buzzerPin);
    }
}

void Alarm::_onState() {
    if (_currentTime - _lastDetectedTime > _timeoutDelay) {
        _state = WATCHING;
        _setRGB(0, 0, 0);
        noTone(_buzzerPin);
        return;
    }

    if (_currentTime - _lastUpdate >= _variationRate) {
        _lastUpdate = _currentTime;
        _currentColor = !_currentColor;
        _setRGB(
            _currentColor ? _colA[0] : _colB[0],
            _currentColor ? _colA[1] : _colB[1],
            _currentColor ? _colA[2] : _colB[2]
        );
        tone(_buzzerPin, 880);  // 1 kHz buzzer tone
    }
}

void Alarm::_testingState() {
    if (_currentTime - _testStartTime > 3000) {
        _state = OFF;
        _setRGB(0, 0, 0);
        noTone(_buzzerPin);
    } else {
        _setRGB(255, 255, 255);  // White light
        tone(_buzzerPin, 2000);  // 2 kHz buzzer
    }
}

void Alarm::turnOn() {
    _state = ON;
    _lastDetectedTime = millis();
}

void Alarm::turnOff() {
    _state = OFF;
    _setRGB(0, 0, 0);
    noTone(_buzzerPin);
}

void Alarm::test() {
    _testStartTime = millis();
    _state = TESTING;
}

void Alarm::_setRGB(int r, int g, int b) {
    analogWrite(_rPin, r);
    analogWrite(_gPin, g);
    analogWrite(_bPin, b);
}

