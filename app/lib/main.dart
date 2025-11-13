import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'firebase_options.dart';

import 'screens/login_screen.dart';
import 'screens/home_screen.dart';
import 'screens/customize_screen.dart';
import 'screens/log_screen.dart';
import 'models/app_state.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Initialize Firebase
  final firebaseApp = await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );

  // Explicitly initialize Realtime Database
  FirebaseDatabase.instanceFor(
    app: firebaseApp,
    databaseURL: "https://safeandsound-ea1e1-default-rtdb.firebaseio.com",
  );

  runApp(const SafeAndSoundApp());
}

class SafeAndSoundApp extends StatefulWidget {
  const SafeAndSoundApp({Key? key}) : super(key: key);

  @override
  State<SafeAndSoundApp> createState() => _SafeAndSoundAppState();
}

class _SafeAndSoundAppState extends State<SafeAndSoundApp> {
  final AppState appState = AppState(); // Single shared AppState

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Safe&Sound',
      theme: ThemeData(primarySwatch: Colors.teal),
      initialRoute: '/',
      routes: {
        '/': (ctx) => LoginScreen(
              appState: appState,  // Pass AppState
              onLogin: () => Navigator.pushReplacementNamed(ctx, '/home'),
            ),
        '/home': (ctx) => HomeScreen(appState: appState),
        '/customize': (ctx) => CustomizeScreen(appState: appState),
        '/logs': (ctx) => LogScreen(appState: appState),
        // Removed '/device' route
      },
    );
  }
}
