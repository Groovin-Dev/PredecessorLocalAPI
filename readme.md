# PredecessorLocalAPI

A **DLL injection** and **local API** project for experimenting with game data in Predecessor.  
This setup uses three main components:

1. **PredCommon** – Shared constants, named pipe utilities, and other helpers.  
2. **PredLoader** – A small executable that injects the payload into Predecessor, then connects to the payload’s named pipe to send/receive commands.  
3. **PredPayload** – A DLL injected into the game, sets up hooks for certain game functions (via **KPE** + Dumper-7–generated SDK), and hosts a named pipe server for debugging/logging or command handling.


## Building

1. Clone this repo locally:
   ```bash
   git clone https://github.com/Groovin-Dev/PredecessorLocalAPI
   git submodule init
   git submodule update
   ```
2. Clone Dumper-7 and generate the SDK (See [Dumper-7](https://github.com/Encryqed/Dumper-7) on how to do this)
    a. Once generated, copy the files from `CppSDK` to `PredPayload/SDK`
3. Open the solution (`.sln`) in Visual Studio.  
4. Build each project in `Release` or `Debug` mode:
   - **PredCommon** → produces a static library or DLL with shared code.  
   - **PredLoader** → produces an executable that handles injection.  
   - **PredPayload** → produces the DLL to be injected into the game.

## Usage

1. **Start Predecessor** (the game).  
2. **Run `PredLoader`** (e.g., `PredLoader.exe`). It will:  
   - Find the Predecessor process by name.  
   - Inject `PredPayload.dll`.  
   - Attempt to connect to the Payload’s named pipe.  
3. Once connected, the **Payload** does the following:
   - Allocates a console (if `DEBUG_CONSOLE` is enabled).  
   - Prints engine addresses (e.g., `UWorld`, `UGameEngine`).  
   - Installs hooks to track purchase attempts, sending log messages to the named pipe and console.  

## Project Structure

- **PredCommon**  
  - `CommonConstants.h`, `NamedPipeServer.h/cpp`, etc.  
- **PredLoader**  
  - `main.cpp`, `Injector.cpp/h`, `Loader.cpp/h`.  
  - Manages injection, connects as named pipe client.  
- **PredPayload**  
  - `dllmain.cpp`, `Hooks.cpp/h`, `Logging.cpp/h`, etc.  
  - Named pipe **server**, hooking logic, logging.

## Notes & Warnings

- **Anti-Cheat**: Any injection or hooking can be flagged by anti-cheat systems. **Use at your own risk**.  
- **Legal**: Make sure you are not violating any Terms of Service. This is purely for *educational or personal debugging* purposes.  
- **Stability**: Hooking or reading game memory can crash or destabilize the application.
