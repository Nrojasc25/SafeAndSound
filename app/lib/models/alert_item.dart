class AlertItem {
  final String type; // e.g. "Fire Alarm"
  final DateTime time;
  final String note;
  final String color;

  AlertItem({required this.type, required this.time, this.note = '', this.color = 'ffffff'});
}
