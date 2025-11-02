# Safe&Sound 

**Safe&Sound** is a cross-platform Flutter application designed to enhance safety and awareness for individuals with hearing impairments. It connects to the **Safe&Sound bracelet**, a wearable device that detects environmental sounds and alerts the user through gentle vibrations and visual cues. The app allows users to customize alert preferences, monitor device status, view logs, and manage multiple devices, providing a seamless and empowering experience for the hearing-impaired community.

## 🎯 Project Overview 
**Purpose:** Safe&Sound provides an intuitive interface for users of the Safe&Sound bracelet. The bracelet detects important environmental sounds—such as alarms, doorbells, or other notifications—and the app ensures that these alerts are communicated effectively through vibrations and visual signals. 
#### **Key Features:** 
- **User Authentication:** Secure sign-up and login with email verification. 
- **Device Integration:** Connect and monitor the Safe&Sound bracelet. 
- **Custom Alerts:** Configure which sounds trigger notifications. 
- **Logs & History:** Track past alerts and device activity. 
- **Accessibility-Focused Design:** Clear, user-friendly interface optimized for users with hearing impairments. 
- **Technologies:** - Flutter (Android, iOS, Windows, Web) - Firebase (Authentication, Firestore, Cloud Storage) - Dart 

## 🚀 Getting Started 
### Prerequisites 
- [Flutter SDK](https://flutter.dev/docs/get-started/install) 
- Git
- VS Code or Android Studio

### [1] Clone the Repository
```bash
git clone https://github.com/your-username/SafeAndSound.git
cd SafeAndSound\app
```

### [2] Install Dependencies
```
flutter pub get
```

### [3] Run the App
Web: 
```
flutter run -d chrome
```

Windows: 
```
flutter run -d windows
```

Andorid/iOS:
```
flutter run
```
### [4] Authentication Notes
* Sign up requires email verification
* Login will prompt users if their email is not verified
* Users with existing accounts can sign in directly.
* Password visibility toggle is available on both login and sing-up screens. 
