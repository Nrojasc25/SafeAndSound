import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import '../models/app_state.dart';
import '../models/settings.dart';

class CustomizeScreen extends StatefulWidget {
  final AppState appState;
  const CustomizeScreen({Key? key, required this.appState}) : super(key: key);

  @override
  State<CustomizeScreen> createState() => _CustomizeScreenState();
}

class _CustomizeScreenState extends State<CustomizeScreen> {
  bool isSaving = false;
  String? editingSound;
  final DatabaseReference ref = FirebaseDatabase.instance.ref();

  final List<String> defaultSounds = ['Fire Alarm', 'Siren', 'Timer'];

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final user = FirebaseAuth.instance.currentUser;
    if (user == null) return;

    try {
      final snapshot = await ref.child('users/${user.uid}/settings').get();
      if (snapshot.exists) {
        final data = List<dynamic>.from(snapshot.value as List);
        widget.appState.loadSettingsFromList(data);
      }
    } catch (e) {
      debugPrint('Error loading settings: $e');
    }

    widget.appState.ensureDefaultSounds();
    setState(() {});
  }

  Future<void> _saveSettings() async {
    final user = FirebaseAuth.instance.currentUser;
    if (user == null) return;

    setState(() => isSaving = true);
    try {
      await ref.child('users/${user.uid}/settings').set(widget.appState.settingsToList());
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Settings saved successfully!')),
      );
    } catch (e) {
      debugPrint('Error saving settings: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Failed to save settings.')),
      );
    } finally {
      setState(() => isSaving = false);
    }
  }

  void _addSound() {
    String newName = '';
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Add New Sound'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              decoration: const InputDecoration(hintText: 'Sound Name'),
              onChanged: (v) => newName = v,
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () {
                // Placeholder for upload functionality
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('Upload functionality not yet implemented')),
                );
              },
              icon: const Icon(Icons.upload_file),
              label: const Text('Upload Sound'),
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              if (newName.trim().isEmpty) return;
              setState(() {
                widget.appState.settings.add(AlertSetting(
                  type: newName.trim(),
                  color: Colors.blue,
                  vibrationPattern: 'short',
                ));
              });
              Navigator.pop(ctx);
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }

  void _deleteSound(AlertSetting s) {
    if (defaultSounds.contains(s.type)) return;

    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Delete Sound'),
        content: Text('Are you sure you want to delete "${s.type}"?'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              setState(() {
                widget.appState.settings.remove(s);
                if (editingSound == s.type) editingSound = null;
              });
              Navigator.pop(ctx);
            },
            child: const Text('Delete'),
          ),
        ],
      ),
    );
  }

  Widget _vibrationSelector(AlertSetting s) {
    final options = ['short', 'long', 'repeat'];
    return Wrap(
      spacing: 8,
      children: options.map((option) {
        final isSelected = s.vibrationPattern == option;
        return ChoiceChip(
          label: Text(option),
          selected: isSelected,
          onSelected: (_) {
            setState(() {
              s.vibrationPattern = option; // Live update locally
            });
          },
        );
      }).toList(),
    );
  }

  Widget _colorSelector(AlertSetting s) {
    final colors = [Colors.blue, Colors.green, Colors.red, Colors.purple, Colors.orange, Colors.teal];
    return Wrap(
      spacing: 8,
      children: colors.map((color) {
        final isSelected = color.value == s.color.value;
        return GestureDetector(
          onTap: () {
            setState(() {
              s.color = color; // Live update locally
            });
          },
          child: Stack(
            alignment: Alignment.center,
            children: [
              CircleAvatar(backgroundColor: color, radius: 20),
              if (isSelected) const Icon(Icons.check, color: Colors.white, size: 22),
            ],
          ),
        );
      }).toList(),
    );
  }

  Widget _soundCard(AlertSetting s) {
    final isEditing = editingSound == s.type;

    return Card(
      key: ValueKey(s.type),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(s.type, style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                Row(
                  children: [
                    IconButton(
                      icon: Icon(isEditing ? Icons.check : Icons.edit),
                      onPressed: () {
                        setState(() {
                          editingSound = isEditing ? null : s.type;
                        });
                      },
                    ),
                    if (!defaultSounds.contains(s.type))
                      IconButton(
                        icon: const Icon(Icons.delete, color: Colors.red),
                        onPressed: () => _deleteSound(s),
                      ),
                  ],
                ),
              ],
            ),
            const SizedBox(height: 8),
            if (isEditing) ...[
              const Text('Vibration Pattern'),
              _vibrationSelector(s),
              const SizedBox(height: 8),
              const Text('Notification Color'),
              _colorSelector(s),
            ] else ...[
              Text('Vibration: ${s.vibrationPattern}'),
              const SizedBox(height: 8),
              Row(children: [
                const Text('Color: '),
                CircleAvatar(backgroundColor: s.color, radius: 12),
              ]),
            ],
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Customize Notifications')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            Expanded(
              child: ReorderableListView(
                onReorder: (oldIndex, newIndex) {
                  setState(() {
                    if (newIndex > oldIndex) newIndex -= 1;
                    final item = widget.appState.settings.removeAt(oldIndex);
                    widget.appState.settings.insert(newIndex, item);
                  });
                },
                children: [
                  for (var s in widget.appState.settings) _soundCard(s),
                ],
              ),
            ),
            ElevatedButton.icon(
              onPressed: _addSound,
              icon: const Icon(Icons.add),
              label: const Text('Add Sound'),
            ),
            const SizedBox(height: 12),
            ElevatedButton(
              onPressed: isSaving ? null : _saveSettings,
              child: isSaving
                  ? const SizedBox(
                      width: 18,
                      height: 18,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Text('Save Changes'),
            ),
          ],
        ),
      ),
    );
  }
}
