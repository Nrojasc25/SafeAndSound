import 'package:flutter/material.dart';
import '../models/app_state.dart';

class DeviceScreen extends StatelessWidget {
  final AppState appState;
  DeviceScreen({required this.appState});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Device Settings')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            ListTile(
              title: Text('Paired Device'),
              subtitle: Text(appState.pairedDeviceName),
            ),
            ListTile(
              title: Text('Battery'),
              subtitle: Text('${appState.deviceBattery}%'),
            ),
            ListTile(
              title: Text('BLE Status'),
              subtitle: Text(appState.bleConnected ? 'Connected' : 'Disconnected'),
            ),
            const SizedBox(height: 20),
            ElevatedButton(onPressed: () {
              // Simulate scan
              ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Scanning... (not implemented)')));
            }, child: Text('Scan for Devices')),
            ElevatedButton(onPressed: () {
              // Simulate disconnect
              ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('Disconnecting (simulated)')));
            }, child: Text('Disconnect')),
          ],
        ),
      ),
    );
  }
}
