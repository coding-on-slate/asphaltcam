# AsphaltCam

**AsphaltCam** is a privacy-first, looping dash cam application built specifically for [Sailfish OS](https://sailfishos.org/). It records a rolling video loop while you drive, automatically saving and locking incident clips when a crash is detected or when triggered manually.

## 🚀 Features

* **Rolling Video Loop:** Continuously records in the background without filling up your storage. The active loop is kept in the app cache and hidden from your Gallery.
* **Incident Locking:** Saves a permanent clip on a manual trigger or automatically when the accelerometer detects a sudden knock or impact.
* **Flexible Recording:** Works with both the front and rear cameras. Supports custom look-back (pre-incident) and post-incident clip lengths.
* **Subtitle Overlay:** Registers GPS, speed, and timestamps into a sidecar `.srt` file, allowing you to view telemetry data during in-app playback or on desktop players.
* **Background Operation:** Keep the screen on while driving or let it blank — the recording loop keeps running either way.
* **Device Protection:** Built-in safeguards monitor heat, storage limits, and battery levels to safely stop recording before your device runs into trouble.
* **Storage:** Saved incidents are stored in individual folders under `Videos/AsphaltCam`. 

## 🔒 Permissions

To function correctly, AsphaltCam requires the following permissions:
* **Camera:** To capture video from the front or rear lenses.
* **Microphone:** For optional audio recording (disabled by default).
* **Location:** To log GPS coordinates and speed data for the telemetry overlay.
* **Videos:** To save, organize, and manage locked incident clips.
* **Audio:** For video playback and optional microphone recording.

## ⚖️ Privacy & Legal Disclaimer

* **100% Offline:** There are no user accounts and no data is ever uploaded. Everything stays securely on your phone.
* **Legal Compliance:** On the first launch, you must accept a notice acknowledging that you are solely responsible for complying with local recording and surveillance laws. 
* **Personal Use Only:** This app is intended for personal use and convenience; clips are not certified to automatically constitute legal evidence.

## License

MIT. See [LICENSE](LICENSE).
