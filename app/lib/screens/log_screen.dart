import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:intl/intl.dart';
import '../models/app_state.dart';
import '../models/alert_item.dart';

class LogScreen extends StatefulWidget {
  final AppState appState;
  const LogScreen({super.key, required this.appState});

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
          final alertTime = DateTime.now(); // fallback time
          final color = value['color'] ?? 'ffffff';

          widget.appState.logs.add(AlertItem(
            type: value['sound'] ?? 'Unknown',
            time: alertTime,
            color: color,
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
                      return ListTile(
                        leading: Container(
                          width: 24,
                          height: 24,
                          decoration: BoxDecoration(
                            shape: BoxShape.circle,
                            color: Color(int.parse('0xff${log.color}')),
                            border: Border.all(color: Colors.black26),
                          ),
                        ),
                        title: Text(log.type),
                        subtitle: Text(
                          DateFormat('yyyy-MM-dd HH:mm:ss').format(log.time),
                        ),
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
