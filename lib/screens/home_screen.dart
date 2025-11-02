import 'package:flutter/material.dart';
import '../models/app_state.dart';
import '../models/alert_item.dart';

class HomeScreen extends StatelessWidget {
  final AppState appState;
  HomeScreen({required this.appState});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Safe&Sound'),
        actions: [
          IconButton(onPressed: () => Navigator.pushNamed(context, '/device'), icon: Icon(Icons.bluetooth)),
          IconButton(onPressed: () => Navigator.pushNamed(context, '/customize'), icon: Icon(Icons.settings)),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Text('Hello — quick recent alerts', style: TextStyle(fontSize: 18)),
            const SizedBox(height: 12),
            Expanded(
              child: ListView.builder(
                itemCount: appState.logs.length,
                itemBuilder: (ctx, i) {
                  final AlertItem item = appState.logs[i];
                  return ListTile(
                    leading: Icon(_iconForType(item.type)),
                    title: Text(item.type),
                    subtitle: Text(_formatTime(item.time)),
                    onTap: () {},
                  );
                },
              ),
            ),
            ElevatedButton(
              onPressed: () => Navigator.pushNamed(context, '/logs'),
              child: const Text('View Full Log'),
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

  String _formatTime(DateTime t) {
    final now = DateTime.now();
    final diff = now.difference(t);
    if (diff.inDays >= 1) return '${diff.inDays}d ${t.hour}:${t.minute.toString().padLeft(2,'0')}';
    return '${t.hour}:${t.minute.toString().padLeft(2,'0')}';
  }
}
