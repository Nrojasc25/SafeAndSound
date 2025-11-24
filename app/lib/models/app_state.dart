import 'package:flutter/material.dart';
import 'alert_item.dart';
import 'settings.dart';

class AppState {
  String? uid; // Store Firebase UID
  List<AlertItem> logs = [];
  List<AlertSetting> settings = [];
  String pairedDeviceName = 'Not paired';
  int deviceBattery = 100;
  bool bleConnected = false;

  AppState();

  factory AppState.sample() {
    final s = AppState();
    s.settings = [
      AlertSetting(type: 'Car_horn', color: Colors.red, vibrationPattern: 'long'),
      AlertSetting(type: 'Siren', color: Colors.orange, vibrationPattern: 'repeat'),
      AlertSetting(type: 'Door_knock', color: Colors.blue, vibrationPattern: 'short'),
      AlertSetting(type: 'Alarm_clock', color: Colors.green, vibrationPattern: 'long'),
    ];
    s.logs = [
      AlertItem(type: 'Car_horn', time: DateTime.now().subtract(Duration(hours: 2))),
      AlertItem(type: 'Siren', time: DateTime.now().subtract(Duration(days: 1, hours: 1))),
      AlertItem(type: 'Door_knock', time: DateTime.now().subtract(Duration(days: 1, hours: 12))),
      AlertItem(type: 'Alarm_clock', time: DateTime.now().subtract(Duration(days: 1, hours: 12))),
    ];
    s.pairedDeviceName = 'ESP32_Wrist_01';
    s.deviceBattery = 82;
    s.bleConnected = true;
    return s;
  }

  List<Map<String, dynamic>> settingsToList() {
    return settings.map((s) => {
      'type': s.type,
      'color': s.color.value.toRadixString(16).padLeft(8, '0'),
      'vibrationPattern': s.vibrationPattern,
    }).toList();
  }

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

  void loadLogsFromMap(Map<dynamic, dynamic> data) {
    logs.clear();
    data.forEach((key, value) {
      logs.add(AlertItem(
        type: value['type'],
        time: DateTime.fromMillisecondsSinceEpoch(value['time']),
      ));
    });
  }

  void ensureDefaultSounds() {
    final defaults = ['Car_horn', 'Siren', 'Door_knock', 'Alarm_clock'];
    final defaultColors = [Colors.red, Colors.orange, Colors.blue, Colors.green];
    final defaultPatterns = ['long', 'repeat', 'short', 'long'];
    
    for (int i = 0; i < defaults.length; i++) {
      if (!settings.any((s) => s.type == defaults[i])) {
        settings.add(AlertSetting(
          type: defaults[i],
          color: defaultColors[i],
          vibrationPattern: defaultPatterns[i],
        ));
      }
    }
  }
}