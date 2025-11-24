// import 'dart:convert';
// import 'package:flutter/material.dart';
// import 'package:flutter_blue/flutter_blue.dart';
// import '../models/app_state.dart';
// import '../models/settings.dart';

// class DeviceScreen extends StatefulWidget {
//   final AppState appState;
//   const DeviceScreen({super.key, required this.appState});

//   @override
//   State<DeviceScreen> createState() => _DeviceScreenState();
// }

// class _DeviceScreenState extends State<DeviceScreen> {
//   FlutterBlue flutterBlue = FlutterBlue.instance;
//   BluetoothDevice? connectedDevice;
//   BluetoothCharacteristic? settingsChar;
//   bool isConnecting = false;

//   List<BluetoothDevice> devicesList = [];

//   @override
//   void initState() {
//     super.initState();
//     startScan();
//   }

//   // Start BLE scan
//   void startScan() async {
//     setState(() {
//       devicesList.clear();
//     });

//     flutterBlue.startScan(timeout: const Duration(seconds: 5));

//     flutterBlue.scanResults.listen((results) {
//       for (ScanResult r in results) {
//         if (!devicesList.contains(r.device)) {
//           // Filter for ESP32 detector
//           if (r.device.name.contains("ESP32_Detector")) {
//             setState(() {
//               devicesList.add(r.device);
//             });
//           }
//         }
//       }
//     });
//   }

//   // Connect to selected device
//   void connectToDevice(BluetoothDevice device) async {
//     setState(() => isConnecting = true);
//     await device.connect(autoConnect: false).catchError((e) {});
//     setState(() {
//       connectedDevice = device;
//       widget.appState.pairedDeviceName = device.name;
//       widget.appState.bleConnected = true;
//     });

//     // Discover services
//     List<BluetoothService> services = await device.discoverServices();
//     for (var service in services) {
//       for (var char in service.characteristics) {
//         if (char.properties.write) {
//           settingsChar = char;
//           sendSettings();
//           break;
//         }
//       }
//     }

//     setState(() => isConnecting = false);
//   }

//   // Disconnect from device
//   void disconnectDevice() {
//     if (connectedDevice != null) {
//       connectedDevice!.disconnect();
//       setState(() {
//         connectedDevice = null;
//         settingsChar = null;
//         widget.appState.pairedDeviceName = 'Not paired';
//         widget.appState.bleConnected = false;
//       });
//     }
//   }

//   // Send settings from AppState to ESP32
//   void sendSettings() async {
//     if (settingsChar == null || widget.appState.settings.isEmpty) return;

//     // Convert settings to JSON string
//     List<Map<String, String>> jsonList = widget.appState.settings
//         .map((s) => {
//               'type': s.type,
//               'color': s.color.value.toRadixString(16).padLeft(8, '0'),
//               'vibrationPattern': s.vibrationPattern,
//             })
//         .toList();

//     String jsonStr = jsonEncode(jsonList);

//     // Split into BLE-safe chunks (~20 bytes)
//     final chunkSize = 20;
//     for (var i = 0; i < jsonStr.length; i += chunkSize) {
//       var end = (i + chunkSize > jsonStr.length) ? jsonStr.length : i + chunkSize;
//       List<int> bytes = utf8.encode(jsonStr.substring(i, end));
//       await settingsChar!.write(bytes, withoutResponse: true);
//       await Future.delayed(const Duration(milliseconds: 50));
//     }

//     ScaffoldMessenger.of(context).showSnackBar(
//       const SnackBar(content: Text('Settings sent to ESP32!')),
//     );
//   }

//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       appBar: AppBar(title: const Text('Device Settings')),
//       body: Padding(
//         padding: const EdgeInsets.all(16),
//         child: Column(
//           children: [
//             // Show paired device info dynamically
//             ListTile(
//               title: const Text('Paired Device'),
//               subtitle: Text(widget.appState.pairedDeviceName),
//             ),
//             ListTile(
//               title: const Text('Battery'),
//               subtitle: Text('${widget.appState.deviceBattery}%'),
//             ),
//             ListTile(
//               title: const Text('BLE Status'),
//               subtitle: Text(widget.appState.bleConnected ? 'Connected' : 'Disconnected'),
//             ),
//             const SizedBox(height: 20),

//             // Scan button
//             ElevatedButton(
//               onPressed: startScan,
//               child: const Text('Scan for Devices'),
//             ),
//             const SizedBox(height: 10),

//             // Show found devices
//             Expanded(
//               child: ListView.builder(
//                 itemCount: devicesList.length,
//                 itemBuilder: (ctx, i) {
//                   final device = devicesList[i];
//                   return ListTile(
//                     title: Text(device.name),
//                     subtitle: Text(device.id.id),
//                     trailing: ElevatedButton(
//                       onPressed: () => connectToDevice(device),
//                       child: const Text('Connect'),
//                     ),
//                   );
//                 },
//               ),
//             ),

//             // Disconnect button
//             if (connectedDevice != null)
//               ElevatedButton(
//                 onPressed: disconnectDevice,
//                 child: const Text('Disconnect'),
//               ),

//             if (isConnecting) const Padding(
//               padding: EdgeInsets.all(8),
//               child: CircularProgressIndicator(),
//             ),
//           ],
//         ),
//       ),
//     );
//   }
// }
