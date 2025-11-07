import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';

class AlertSetting {
  String type;
  Color color;
  String vibrationPattern; // e.g., "short", "long", "repeat"

  AlertSetting({
    required this.type,
    required this.color,
    required this.vibrationPattern,
  });

  // Convert list of settings to BLE-friendly string
  static String convertToBLE(List<AlertSetting> settings) {
    return settings
        .map((s) =>
            '${s.type},${s.color.value.toRadixString(16).padLeft(8, '0')},${s.vibrationPattern}')
        .join(';');
  }

  // Fetch settings from Firebase
  static Future<List<AlertSetting>> fetchFromFirebase(String uid) async {
    final ref = FirebaseDatabase.instance.ref('users/$uid/settings');
    final snapshot = await ref.get();
    final List<AlertSetting> settings = [];

    if (snapshot.exists) {
      final data = snapshot.value as Map<dynamic, dynamic>;
      data.forEach((key, value) {
        settings.add(AlertSetting(
          type: value['type'] ?? 'Unknown',
          color: Color(int.parse(value['color'], radix: 16)),
          vibrationPattern: value['vibrationPattern'] ?? 's',
        ));
      });
    }

    return settings;
  }
}
