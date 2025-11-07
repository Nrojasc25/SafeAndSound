import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:intl/intl.dart';
import '../models/app_state.dart';
import '../models/alert_item.dart';

class LogScreen extends StatefulWidget {
  final AppState appState;
  const LogScreen({Key? key, required this.appState}) : super(key: key);

  @override
  State<LogScreen> createState() => _LogScreenState();
}

class _LogScreenState extends State<LogScreen> {
  DatabaseReference? alertsRef;
  Stream<DatabaseEvent>? alertsStream;

  @override
  void initState() {
    super.initState();
    _initFirebaseListener();
  }

  void _initFirebaseListener() {
    final user = FirebaseAuth.instance.currentUser;
    if (user == null) return;

    alertsRef = FirebaseDatabase.instance.ref('users/${user.uid}/alerts');
    alertsStream = alertsRef!.onValue;

    alertsStream!.listen((DatabaseEvent event) {
      final data = event.snapshot.value;
      widget.appState.logs.clear();

      if (data != null && data is Map) {
        data.forEach((key, value) {
          widget.appState.logs.add(AlertItem(
            type: value['sound'] ?? 'Unknown',
            time: value['timestamp'] != null
                ? DateTime.fromMillisecondsSinceEpoch(value['timestamp'], isUtc: true).toLocal()
                : value['time'] != null
                    ? DateTime.fromMillisecondsSinceEpoch(value['time'], isUtc: true).toLocal()
                    : DateTime.now(),
          ));
        });
        widget.appState.logs.sort((a, b) => b.time.compareTo(a.time));
      }

      setState(() {});
    });
  }

  void _clearLogs() async {
    final user = FirebaseAuth.instance.currentUser;
    if (user == null) return;

    await FirebaseDatabase.instance.ref('users/${user.uid}/alerts').remove();
    setState(() => widget.appState.logs.clear());

    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('All alerts cleared from Firebase')),
    );
  }

  IconData _iconForType(String type) {
    switch (type.toLowerCase()) {
      case 'fire alarm':
        return Icons.local_fire_department;
      case 'doorbell':
        return Icons.notifications;
      case 'siren':
        return Icons.sos;
      default:
        return Icons.volume_up;
    }
  }

  @override
  Widget build(BuildContext context) {
    final logs = widget.appState.logs;

    return Scaffold(
      appBar: AppBar(title: const Text('Full Alert Log')),
      body: Column(
        children: [
          Expanded(
            child: logs.isEmpty
                ? const Center(
                    child: Text(
                      'No detected alerts yet',
                      style: TextStyle(fontSize: 16),
                    ),
                  )
                : ListView.builder(
                    itemCount: logs.length,
                    itemBuilder: (ctx, i) {
                      final log = logs[i];
                      final formattedTime =
                          DateFormat('yyyy-MM-dd HH:mm:ss').format(log.time);
                      return ListTile(
                        leading: Icon(_iconForType(log.type)),
                        title: Text(log.type),
                        subtitle: Text(formattedTime),
                      );
                    },
                  ),
          ),
          Padding(
            padding: const EdgeInsets.all(12),
            child: ElevatedButton.icon(
              icon: const Icon(Icons.delete),
              label: const Text('Clear Logs'),
              onPressed: _clearLogs,
            ),
          ),
        ],
      ),
    );
  }
}
