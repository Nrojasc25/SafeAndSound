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
      AlertSetting(type: 'Siren', color: Colors.orange, vibrationPattern: 'repeat'),
      AlertSetting(type: 'Timer', color: Colors.blue, vibrationPattern: 'short'),
    ];
    s.logs = [
      AlertItem(type: 'Fire Alarm', time: DateTime.now().subtract(Duration(hours: 2))),
      AlertItem(type: 'Siren', time: DateTime.now().subtract(Duration(days: 1, hours: 1))),
      AlertItem(type: 'Timer', time: DateTime.now().subtract(Duration(days: 1, hours: 12))),
    ];
    s.pairedDeviceName = 'ESP32_Wrist_01';
    s.deviceBattery = 82;
    s.bleConnected = true;
    return s;
  }

  // Convert settings to a List for Firebase to preserve order
  List<Map<String, dynamic>> settingsToList() {
    return settings.map((s) => {
      'type': s.type,
      'color': s.color.value.toRadixString(16).padLeft(8, '0'),
      'vibrationPattern': s.vibrationPattern,
    }).toList();
  }

  // Load settings from List to preserve order
  void loadSettingsFromList(List<dynamic> data) {
    settings.clear();
    for (var item in data) {
      settings.add(AlertSetting(
        type: item['type'],
        color: Color(int.parse(item['color'], radix: 16)),
        vibrationPattern: item['vibrationPattern'],
      ));
    }
    ensureDefaultSounds();
  }

  // Convert logs to a Map for Firebase
  Map<String, dynamic> logsToMap() {
    final Map<String, dynamic> map = {};
    for (int i = 0; i < logs.length; i++) {
      map['log_$i'] = {
        'type': logs[i].type,
        'time': logs[i].time.millisecondsSinceEpoch,
      };
    }
    return map;
  }

  // Load logs from Firebase map
  void loadLogsFromMap(Map<dynamic, dynamic> data) {
    logs.clear();
    data.forEach((key, value) {
      logs.add(AlertItem(
        type: value['type'],
        time: DateTime.fromMillisecondsSinceEpoch(value['time']),
      ));
    });
  }

  // Ensure default sounds exist
  void ensureDefaultSounds() {
    final defaults = ['Fire Alarm', 'Siren', 'Timer'];
    for (var type in defaults) {
      if (!settings.any((s) => s.type == type)) {
        settings.add(AlertSetting(
          type: type,
          color: Colors.red,
          vibrationPattern: 'long',
        ));
      }
    }
  }
}
