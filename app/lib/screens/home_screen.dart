import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import '../models/app_state.dart';
import '../models/alert_item.dart';
import 'package:intl/intl.dart';

class HomeScreen extends StatefulWidget {
  final AppState appState;
  const HomeScreen({Key? key, required this.appState}) : super(key: key);

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
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
                ? DateTime.fromMillisecondsSinceEpoch(
                    value['timestamp'], isUtc: true)
                    .toLocal()
                : DateTime.now(),
          ));
        });
        widget.appState.logs.sort((a, b) => b.time.compareTo(a.time));
      }

      setState(() {}); // rebuild HomeScreen when logs update
    });
  }

  @override
  Widget build(BuildContext context) {
    final recentAlerts = widget.appState.logs.take(5).toList();

    return Scaffold(
      appBar: AppBar(
        title: const Text('Safe&Sound'),
        actions: [
          // Removed Bluetooth IconButton
          IconButton(
              onPressed: () => Navigator.pushNamed(context, '/customize'),
              icon: const Icon(Icons.settings)),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Center(
              child: const Text(
                'Hello — quick recent alerts',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
              ),
            ),
            const SizedBox(height: 12),
            Expanded(
              child: recentAlerts.isEmpty
                  ? const Center(
                      child: Text(
                        'No detected alerts yet',
                        style: TextStyle(fontSize: 16),
                      ),
                    )
                  : ListView.builder(
                      itemCount: recentAlerts.length,
                      itemBuilder: (ctx, i) {
                        final AlertItem item = recentAlerts[i];
                        return ListTile(
                          leading: Icon(_iconForType(item.type)),
                          title: Text(item.type),
                          subtitle: Text(
                            DateFormat('yyyy-MM-dd HH:mm:ss')
                                .format(item.time),
                          ),
                        );
                      },
                    ),
            ),
            Center(
              child: ElevatedButton(
                onPressed: () => Navigator.pushNamed(context, '/logs'),
                child: const Text('View Full Log'),
              ),
            ),
          ],
        ),
      ),
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
}
