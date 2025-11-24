import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import '../models/app_state.dart';
import '../models/settings.dart';
import 'signup_screen.dart';

class LoginScreen extends StatefulWidget {
  final VoidCallback onLogin;
  final AppState appState; // AppState instance
  const LoginScreen({super.key, required this.onLogin, required this.appState});

  @override
  State<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  final _emailCtrl = TextEditingController();
  final _passCtrl = TextEditingController();
  bool _isLoading = false;
  String? _error;
  bool _showPassword = false;

  Future<void> _handleLogin() async {
    setState(() {
      _isLoading = true;
      _error = null;
    });

    try {
      final credential = await FirebaseAuth.instance.signInWithEmailAndPassword(
        email: _emailCtrl.text.trim(),
        password: _passCtrl.text.trim(),
      );

      final user = credential.user!;
      if (!user.emailVerified) {
        // Email not verified
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: const Text('Email not verified. Please check your inbox.'),
            action: SnackBarAction(
              label: 'Resend',
              onPressed: () async {
                try {
                  await user.sendEmailVerification();
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('Verification email resent!')),
                  );
                } catch (err) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    SnackBar(content: Text('Failed to resend: $err')),
                  );
                }
              },
            ),
          ),
        );
      } else {
        // ✅ Store UID in AppState
        widget.appState.uid = user.uid;

        // ✅ Fetch user settings from Firebase
        try {
          final settings = await AlertSetting.fetchFromFirebase(user.uid);
          setState(() {
            widget.appState.settings = settings;
          });
          print("Loaded ${settings.length} settings for UID ${user.uid}");
        } catch (e) {
          print("Error fetching settings: $e");
        }

        // ✅ Navigate to home
        widget.onLogin();
      }
    } on FirebaseAuthException catch (e) {
      if (e.code == 'user-not-found') {
        setState(() => _error = 'Account does not exist. Please sign up.');
      } else if (e.code == 'wrong-password') {
        setState(() => _error = 'Incorrect password.');
      } else {
        setState(() => _error = e.message);
      }
    } finally {
      setState(() => _isLoading = false);
    }
  }

  void _navigateToSignUp() {
    _emailCtrl.clear();
    _passCtrl.clear();
    Navigator.push(
      context,
      MaterialPageRoute(builder: (context) => const SignUpScreen()),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Safe&Sound Login')),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            const SizedBox(height: 40),
            TextField(
              controller: _emailCtrl,
              decoration: const InputDecoration(labelText: 'Email'),
              keyboardType: TextInputType.emailAddress,
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _passCtrl,
              obscureText: !_showPassword,
              decoration: InputDecoration(
                labelText: 'Password',
                suffixIcon: IconButton(
                  icon: Icon(
                    _showPassword ? Icons.visibility : Icons.visibility_off,
                  ),
                  onPressed: () => setState(() => _showPassword = !_showPassword),
                ),
              ),
            ),
            const SizedBox(height: 20),
            if (_error != null)
              Text(_error!, style: const TextStyle(color: Colors.red)),
            const SizedBox(height: 10),
            ElevatedButton(
              onPressed: _isLoading ? null : _handleLogin,
              child: _isLoading
                  ? const CircularProgressIndicator()
                  : const Text('Login'),
            ),
            TextButton(
              onPressed: _navigateToSignUp,
              child: const Text('Don\'t have an account? Sign Up'),
            ),
          ],
        ),
      ),
    );
  }
}
