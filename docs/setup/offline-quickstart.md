# Offline Quickstart

Use this path when testing with no internet connection.

1. Confirm Unreal Engine is installed locally.
2. Confirm `config/unreal-engine.json` points to your local engine root.
3. Build the plugin:

   ```powershell
   .\scripts\build-plugin\Build-HostProject.ps1
   ```

4. Open the Unreal project and choose `Tools > Revolt Bridge`.
5. Click `Start Bridge`.
6. Launch the desktop runtime:

   ```powershell
   cd desktop
   npm run dev
   ```

7. Use `Manual Command Mode` if no local model is installed.
8. Optional: start a local model server and create a local model profile.

Default behavior stays local: Unreal bridge uses localhost, desktop data is SQLite, and cloud providers are disabled.
