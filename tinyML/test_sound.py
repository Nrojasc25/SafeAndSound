"""
Minimal TinyML Alarm Sound Classifier (updated)
- Automatically splits long WAVs into 1-second clips
- Adds simple background noise augmentation
- Train tiny CNN, convert to TFLite, and test all files
"""

import os
import numpy as np
import librosa
import tensorflow as tf
from sklearn.utils import shuffle

# ------------------------------
# Config
# ------------------------------
DATA_DIR = "sounds"
SAMPLE_RATE = 16000
DURATION = 1.0        # seconds per clip
N_MFCC = 13
HOP_LENGTH = 512
EPOCHS = 10
TRAIN_SPLIT = 0.8
AUGMENT_NOISE = True   # Add small background noise

# ------------------------------
# Helper: load WAV and extract MFCCs
# ------------------------------
def extract_mfcc(y, sr=SAMPLE_RATE):
    mfcc = librosa.feature.mfcc(y=y, sr=sr, n_mfcc=N_MFCC, hop_length=HOP_LENGTH)
    return mfcc.T  # (time_steps, n_mfcc)

def split_audio(y, sr=SAMPLE_RATE, duration=DURATION):
    clip_len = int(sr * duration)
    clips = []
    for start in range(0, len(y), clip_len):
        clip = y[start:start+clip_len]
        if len(clip) < clip_len:
            clip = np.pad(clip, (0, clip_len - len(clip)), mode='constant')
        clips.append(clip)
    return clips

def augment_noise(y, noise_factor=0.005):
    noise = np.random.randn(len(y))
    return y + noise_factor * noise

# ------------------------------
# Load dataset
# ------------------------------
X, y_labels = [], []
label_map = {}

for i, class_name in enumerate(sorted(os.listdir(DATA_DIR))):
    class_path = os.path.join(DATA_DIR, class_name)
    if os.path.isdir(class_path):
        label_map[i] = class_name
        for file in os.listdir(class_path):
            if file.endswith(".wav"):
                file_path = os.path.join(class_path, file)
                y_audio, sr = librosa.load(file_path, sr=SAMPLE_RATE)
                
                # Optional noise augmentation
                clips = split_audio(y_audio, sr)
                for clip in clips:
                    if AUGMENT_NOISE:
                        clip = augment_noise(clip)
                    mfcc = extract_mfcc(clip)
                    X.append(mfcc)
                    y_labels.append(i)

# Pad sequences to same length
max_len = max([x.shape[0] for x in X])
X_padded = []
for x in X:
    if x.shape[0] < max_len:
        pad_width = max_len - x.shape[0]
        x = np.pad(x, ((0,pad_width),(0,0)), mode='constant')
    X_padded.append(x)
X = np.array(X_padded)
X = np.expand_dims(X, -1)  # add channel dimension
y_labels = np.array(y_labels)

# Normalize MFCCs
X = (X - np.mean(X)) / np.std(X)

# Shuffle dataset
X, y_labels = shuffle(X, y_labels, random_state=42)

# Train/test split
split_idx = int(TRAIN_SPLIT * len(X))
X_train, X_test = X[:split_idx], X[split_idx:]
y_train, y_test = y_labels[:split_idx], y_labels[split_idx:]

print("X shape:", X.shape, "y shape:", y_labels.shape)
print(f"Training samples: {len(X_train)}, Testing samples: {len(X_test)}")

# ------------------------------
# Build tiny CNN
# ------------------------------
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=X.shape[1:]),
    tf.keras.layers.Conv2D(8, (3,3), activation='relu'),
    tf.keras.layers.Flatten(),
    tf.keras.layers.Dense(len(label_map), activation='softmax')
])

model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])

# ------------------------------
# Train
# ------------------------------
model.fit(X_train, y_train, epochs=EPOCHS, verbose=2, validation_data=(X_test, y_test))

# ------------------------------
# Convert to TFLite
# ------------------------------
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()
with open("sound_classifier.tflite", "wb") as f:
    f.write(tflite_model)
print("Saved TinyML model as sound_classifier.tflite")

# ------------------------------
# Test all WAV files (simplified)
# ------------------------------
interpreter = tf.lite.Interpreter(model_path="sound_classifier.tflite")
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("\n=== TESTING ALL WAV FILES ===\n")
total_files = 0
correct_preds = 0

for class_name in sorted(os.listdir(DATA_DIR)):
    class_path = os.path.join(DATA_DIR, class_name)
    if os.path.isdir(class_path):
        for file in sorted(os.listdir(class_path)):
            if file.endswith(".wav"):
                file_path = os.path.join(class_path, file)
                y_audio, sr = librosa.load(file_path, sr=SAMPLE_RATE)
                clips = split_audio(y_audio, sr)
                for clip in clips:
                    total_files += 1
                    mfcc = extract_mfcc(clip)
                    if mfcc.shape[0] < max_len:
                        mfcc = np.pad(mfcc, ((0, max_len - mfcc.shape[0]), (0,0)), mode='constant')
                    mfcc = np.expand_dims(mfcc, axis=(0,-1))

                    interpreter.set_tensor(input_details[0]['index'], mfcc.astype(np.float32))
                    interpreter.invoke()
                    output = interpreter.get_tensor(output_details[0]['index'])[0]

                    pred_class = label_map[np.argmax(output)]
                    is_correct = "✓" if pred_class == class_name else "✗"
                    if is_correct == "✓":
                        correct_preds += 1

                    print(f"{file}: Predicted -> {pred_class} {is_correct}")

accuracy = correct_preds / total_files * 100 if total_files > 0 else 0
print(f"\n=== SUMMARY ===")
print(f"Total clips tested: {total_files}")
print(f"Correct predictions: {correct_preds}")
print(f"Accuracy: {accuracy:.2f}%")
