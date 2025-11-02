import 'package:flutter/material.dart';

class AlertSetting {
  String type;
  Color color;
  String vibrationPattern; // e.g., "short", "long", "repeat"

  AlertSetting({
    required this.type,
    required this.color,
    required this.vibrationPattern,
  });
}
