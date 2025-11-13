class AlertItem {
  final String type; // e.g. "Fire Alarm"
  final DateTime time;
  final String note;

  AlertItem({required this.type, required this.time, this.note = ''});
}
