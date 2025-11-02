import 'package:flutter/material.dart';
import 'alert_item.dart';
import 'settings.dart';

class AppState {
  List<AlertItem> logs = [];
  List<AlertSetting> settings = [];
  String pairedDeviceName = 'Not paired';
  int deviceBattery = 100;
  bool bleConnected = false;

  AppState();

  // Sample data for testing
  factory AppState.sample() {
    final s = AppState();
    s.settings = [
      AlertSetting(type: 'Fire Alarm', color: Colors.red, vibrationPattern: 'long'),
      AlertSetting(type: 'Doorbell', color: Colors.blue, vibrationPattern: 'short'),
      AlertSetting(type: 'Siren', color: Colors.orange, vibrationPattern: 'repeat'),
    ];
    s.logs = [
      AlertItem(type: 'Doorbell', time: DateTime.now().subtract(Duration(hours: 2))),
      AlertItem(type: 'Fire Alarm', time: DateTime.now().subtract(Duration(days: 1, hours: 1))),
      AlertItem(type: 'Siren', time: DateTime.now().subtract(Duration(days: 1, hours: 12))),
    ];
    s.pairedDeviceName = 'ESP32_Wrist_01';
    s.deviceBattery = 82;
    s.bleConnected = true;
    return s;
  }
}
