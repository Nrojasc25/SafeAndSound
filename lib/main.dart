import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'firebase_options.dart';

import 'screens/login_screen.dart';
import 'screens/home_screen.dart';
import 'screens/customize_screen.dart';
import 'screens/log_screen.dart';
import 'screens/device_screen.dart';
import 'models/app_state.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Initialize Firebase
  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );

  runApp(SafeAndSoundApp());
}

class SafeAndSoundApp extends StatefulWidget {
  @override
  _SafeAndSoundAppState createState() => _SafeAndSoundAppState();
}

class _SafeAndSoundAppState extends State<SafeAndSoundApp> {
  // Simple in-memory global app state
  final AppState appState = AppState.sample();

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Safe&Sound',
      theme: ThemeData(primarySwatch: Colors.teal),
      initialRoute: '/',
      routes: {
        '/': (ctx) => LoginScreen(
            onLogin: () => Navigator.pushReplacementNamed(ctx, '/home')),
        '/home': (ctx) => HomeScreen(appState: appState),
        '/customize': (ctx) => CustomizeScreen(appState: appState),
        '/logs': (ctx) => LogScreen(appState: appState),
        '/device': (ctx) => DeviceScreen(appState: appState),
      },
    );
  }
}
