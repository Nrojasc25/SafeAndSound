import 'package:flutter/material.dart';
import '../models/app_state.dart';
import '../models/settings.dart';

class CustomizeScreen extends StatefulWidget {
  final AppState appState;
  CustomizeScreen({required this.appState});

  @override
  State<CustomizeScreen> createState() => _CustomizeScreenState();
}

class _CustomizeScreenState extends State<CustomizeScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Customize Alerts')),
      body: ListView(
        padding: const EdgeInsets.all(12),
        children: [
          ...widget.appState.settings.map((s) => Card(
                child: ListTile(
                  leading: CircleAvatar(backgroundColor: s.color),
                  title: Text(s.type),
                  subtitle: Text('Vibration: ${s.vibrationPattern}'),
                  trailing: IconButton(
                    icon: Icon(Icons.edit),
                    onPressed: () => _editSetting(context, s),
                  ),
                ),
              )),
          const SizedBox(height: 16),
          ElevatedButton(onPressed: _save, child: const Text('Save Changes (local)')),
        ],
      ),
    );
  }

  void _editSetting(BuildContext ctx, AlertSetting s) {
    final vibrationCtrl = TextEditingController(text: s.vibrationPattern);
    Color selected = s.color;
    showDialog(
      context: ctx,
      builder: (dCtx) => AlertDialog(
        title: Text('Edit ${s.type}'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Wrap(
              spacing: 8,
              children: [
                _colorChoice(Colors.red, () => setState(() => selected = Colors.red)),
                _colorChoice(Colors.blue, () => setState(() => selected = Colors.blue)),
                _colorChoice(Colors.orange, () => setState(() => selected = Colors.orange)),
                _colorChoice(Colors.green, () => setState(() => selected = Colors.green)),
              ],
            ),
            const SizedBox(height: 12),
            TextField(controller: vibrationCtrl, decoration: InputDecoration(labelText: 'Vibration pattern')),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(dCtx), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              setState(() {
                s.color = selected;
                s.vibrationPattern = vibrationCtrl.text;
              });
              Navigator.pop(dCtx);
            },
            child: const Text('OK'),
          ),
        ],
      ),
    );
  }

  Widget _colorChoice(Color c, VoidCallback onTap) {
    return GestureDetector(
      onTap: onTap,
      child: CircleAvatar(backgroundColor: c, radius: 16),
    );
  }

  void _save() {
    // For now save is local. We will wire Firebase later.
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Settings saved locally')));
  }
}
