# 🌊 GrainEye Ver: 1.03
*A Sand Grain Analyzer & Beach Sand Grain Mapping Tool*

---

## 📖 Overview
  **GrainEye** is a sand grain analyzer and beach mapping tool designed for deployment on standalone computing devices like **Raspberry Pi**.  
  The app enables users to capture and upload sand sample images, send them to a cloud-hosted deep learning model for analysis, and visualize results with geolocation tagging and CSV export.  

This project is being developed as part of the **Smart India Hackathon (SIH) 2025** submission.  

---

## ✨ Features
- 📸 **Image Upload & Analysis**  
  - Capture/upload sand sample images from the Raspberry Pi camera.  
  - Send images to cloud-hosted ML model for sand grain analysis.  
  - Receive structured results back in the app.
    
- 🏝️ **Sand Grain Analysis Output** (Example)  
        SAND TYPE ANALYSIS COMPLETE:
        
        • Beach Zone: Intertidal Zone (Foreshore / Swash Zone)
        • Location: Area between high tide and low tide
        • Sand Size: Medium Sand (0.25–0.5 mm)
        • Median (d50): 0.43 mm
        • Mean Grain Size: 0.43 mm
        • Range (d10–d90): 0.26 – 0.70 mm
        • Beach Type: Typical sandy beach, dissipative
        
        • Category: Medium Sand → Intertidal
        • GPS: 21.63°N, 87.55°E
        • Time: 2025-09-25 1:48pm
        • Image: c:/users/.../20191010_130927_1c.jpg
  
- 📍 **Geolocation Tagging**  
  - Fetch real-time GPS coordinates using **EC2000U-CN GNSS module**.  
  - Tag analysis data on a custom-built mapping API.  
  
  - 💾 **Data Export**  
  - Save full results to a `.csv` file.  
  - Start over with a new sample using the **Restart** button.  

---

## ⚙️ System Architecture
1. **Client-Side (Win32 App on Raspberry Pi)**  
 - Built in **C++ Win32 API**.  
 - Provides UI: Upload, Analyze, Location, Tag, Save, Restart.  
 - Handles communication with cloud + hardware modules.  

2. **Cloud-Side:**  
 - Hosts Deep Learning model for sand grain classification & beach type mapping.  
 - Returns structured results to client app.  

3. **Hardware Components:**  
   - **Raspberry Pi**: primary device.  
   - **Raspberry Pi Camera**: image capture.  
   - **EC2000U-CN Module**: GNSS-based location tracking and internet connection.  
 
---

## 🖥️ Tech Stack:
  - **Language**: C++ (Win32 API), Currently porting to Qt for native Linux support  
  - **Platform**: Raspberry Pi OS / Windows cross-compile support, Win10 IOT Core  
  - **Cloud**: Deep Learning model endpoint (under development)  
  - **Hardware**: Raspberry Pi Camera, EC2000U-CN GNSS module, Raspberry Pi Zero 2W  

---

## 🚀 Getting Started:
  1. **Clone this repository**
     ``bash
               git clone https://github.com/your-username/GrainEye.git
  2. Open in Visual Studio (or Raspberry Pi toolchain).
  3. Build the project.
  4. Run the app on Raspberry Pi device with connected camera + GNSS module.

📌 Current Status:

  - ✅ Frontend Win32 App ready.
  - 🚧 Cloud connectivity & deep learning analysis pipeline under development.
  - 🚧 Map API integration in progress.

🔮 Future Scope: 

  - Advanced deep learning models for finer grain classification.  
  - Real-time mapping dashboard with multiple beach survey datasets.  
  - Offline fallback mode with lightweight local analysis.

⚠️ Disclaimer:

   This project is currently under development and submitted as part of SIH 2025.
   The code is intended for demonstration purposes only.
   Redistribution, modification, or commercial use is not permitted at this stage.
   A proper open-source license will be added after final release.

👨‍💻 Contributors:

   Team GrainEYE  
   Subhajit Halder– C++ Development, System Architecture  
   Aniket Mandal- Deep Learning Model, under development
