import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:intl/intl.dart'; 
import '../models/app_state.dart';
import '../models/alert_item.dart';

class LogScreen extends StatefulWidget {
  final AppState appState;
  LogScreen({required this.appState});

  @override
  State<LogScreen> createState() => _LogScreenState();
}

class _LogScreenState extends State<LogScreen> {
  DatabaseReference? logsRef;
  Stream<DatabaseEvent>? logsStream;

  @override
  void initState() {
    super.initState();
    _initFirebaseListener();
  }

  void _initFirebaseListener() {
    final user = FirebaseAuth.instance.currentUser;
    if (user == null) return;

    logsRef = FirebaseDatabase.instance.ref('users/${user.uid}/logs');
    logsStream = logsRef!.onValue;

    // Listen for live changes
    logsStream!.listen((DatabaseEvent event) {
      final data = event.snapshot.value;
      if (data != null && data is Map) {
        widget.appState.logs.clear();
        data.forEach((key, value) {
          widget.appState.logs.add(AlertItem(
            type: value['type'] ?? 'Unknown',
            time: DateTime.fromMillisecondsSinceEpoch(value['time']),
          ));
        });
        setState(() {});
      }
    });
  }

  Future<void> _addLog(AlertItem log) async {
    final user = FirebaseAuth.instance.currentUser;
    if (user == null) return;

    final ref = FirebaseDatabase.instance.ref('users/${user.uid}/logs').push();
    await ref.set({
      'type': log.type,
      'time': log.time.millisecondsSinceEpoch,
    });
  }

  void _clearLogs() async {
    final user = FirebaseAuth.instance.currentUser;
    if (user == null) return;

    await FirebaseDatabase.instance.ref('users/${user.uid}/logs').remove();
    setState(() => widget.appState.logs.clear());

    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('All logs cleared from Firebase')),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Alert History')),
      body: Column(
        children: [
          Expanded(
            child: widget.appState.logs.isEmpty
                ? const Center(child: Text('No alerts recorded'))
                : ListView.builder(
                    itemCount: widget.appState.logs.length,
                    itemBuilder: (ctx, i) {
                      final AlertItem log = widget.appState.logs[i];
                      final formattedTime =
                          DateFormat('yyyy-MM-dd HH:mm:ss').format(log.time);
                      return ListTile(
                        leading: const Icon(Icons.notifications),
                        title: Text(log.type),
                        subtitle: Text(formattedTime),
                      );
                    },
                  ),
          ),
          Padding(
            padding: const EdgeInsets.all(12),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                ElevatedButton.icon(
                  icon: const Icon(Icons.add),
                  label: const Text('Simulate Alert'),
                  onPressed: () {
                    final newLog = AlertItem(
                      type: 'Motion Detected',
                      time: DateTime.now(),
                    );
                    setState(() => widget.appState.logs.add(newLog));
                    _addLog(newLog);
                  },
                ),
                ElevatedButton.icon(
                  icon: const Icon(Icons.delete),
                  label: const Text('Clear Logs'),
                  onPressed: _clearLogs,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
