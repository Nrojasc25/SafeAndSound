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

  // Fetch settings from Firebase for a given UID
  static Future<List<AlertSetting>> fetchFromFirebase(String uid) async {
    final ref = FirebaseDatabase.instance.ref("users/$uid/settings");
    final snapshot = await ref.get();

    if (!snapshot.exists) return [];

    final List<AlertSetting> settings = [];
    final data = snapshot.value as Map<dynamic, dynamic>;

    data.forEach((key, value) {
      if (value != null) {
        settings.add(AlertSetting(
          type: value['type'] ?? 'Unknown',
          color: Color(int.parse(value['color'] ?? '0xFFFFFFFF')),
          vibrationPattern: value['vibrationPattern'] ?? 'short',
        ));
      }
    });

    return settings;
  }
}
