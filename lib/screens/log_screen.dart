import 'package:flutter/material.dart';
import '../models/app_state.dart';
import '../models/alert_item.dart';

class LogScreen extends StatefulWidget {
  final AppState appState;
  LogScreen({required this.appState});

  @override
  State<LogScreen> createState() => _LogScreenState();
}

class _LogScreenState extends State<LogScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Alert History')),
      body: Column(
        children: [
          Expanded(
            child: ListView.builder(
              itemCount: widget.appState.logs.length,
              itemBuilder: (ctx, i) {
                final AlertItem it = widget.appState.logs[i];
                return ListTile(
                  leading: Icon(Icons.event),
                  title: Text(it.type),
                  subtitle: Text('${it.time}'),
                );
              },
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(12),
            child: ElevatedButton(
              onPressed: () {
                setState(() => widget.appState.logs.clear());
              },
              child: const Text('Clear Log'),
            ),
          ),
        ],
      ),
    );
  }
}
