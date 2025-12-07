# 🚀 Building and Running

## Prerequisites
- **GCC** (native x86_64 or cross-compiler)
- **GNU Make**
- **GRUB tools** (grub-mkrescue, xorriso)
- **QEMU** (qemu-system-x86_64)

## Build Instructions (Linux / WSL)

Run these commands in your terminal (or PowerShell if using WSL):

```bash
make clean         # Clean build artifacts
make               # Build kernel only
make iso           # Create bootable ISO (build/microkernel.iso)
make run           # Run in QEMU (opens window + serial output)
make run-headless  # Run in QEMU (terminal only, no window)
make gdb           # Debug with GDB
```

> **WSL Users:** Prepend `wsl` to the commands if running from PowerShell.
> Example: `wsl make iso`, `wsl make run-headless`

## 🖥️ How to Exit QEMU

When running in **headless mode** (`make run-headless`), QEMU takes over your terminal.
To exit:

1.  Press **`Ctrl` + `a`** (release keys)
2.  Press **`x`**

*(If you are stuck, open a new terminal and run `wsl killall qemu-system-x86_64`)*
## Output Files

- `build/kernel.elf` - Kernel binary
- `build/microkernel.iso` - Bootable ISO image
- `serial.log` - Serial output capture (when running with -serial file:serial.log)

## WSL Build (Windows)

If you are using Windows Subsystem for Linux (WSL), the easiest way is to use `wsl` directly:

```powershell
# 1. Build the ISO
wsl make iso

# 2. Run in Terminal (Headless)
wsl make run-headless

# 3. Run with GUI (Requires X Server like VcXsrv installed on Windows)
wsl make run
```

### Automated Testing (Capture Output)
To run the kernel for a fixed time and save the output to a file:
```powershell
wsl bash -c "cd '/mnt/c/path/to/project' && rm -f serial.log && timeout 15 qemu-system-x86_64 -cdrom build/microkernel.iso -serial file:serial.log -display none; cat serial.log"
```
*Note: The "terminating on signal 15" message is normal.*

**Note:** Replace `/mnt/c/path/to/project` with your actual project path (e.g., `/mnt/c/Users/Name/Desktop/Microkernel OS`).

## Understanding the Output

### "terminating on signal 15" - This is NORMAL!
When you see:
```
qemu-system-x86_64: terminating on signal 15 from pid 18 (timeout)
```
This is **NOT an error**. It means:
- ✅ The `timeout 15` command successfully ran QEMU for 15 seconds
- ✅ QEMU was cleanly terminated (signal 15 = SIGTERM)
- ✅ The kernel output was captured to `serial.log`
- ✅ The output is then displayed by `cat serial.log`

## Troubleshooting

**Problem**: `make: command not found`  
**Solution**: Ensure you are running inside WSL or have Make installed.

**Problem**: QEMU not found  
**Solution**: Install in WSL: `sudo apt-get install qemu-system-x86`

**Problem**: "GTK cannot open display"  
**Solution**: You are trying to run graphical QEMU from a headless WSL instance. Use the headless command (serial output) or install a generic X server for Windows (like VcXsrv).

## Usage 📝

### Interactive Shell

The shell starts automatically and supports these commands:

```
shell> help              # Show available commands
shell> server            # Launch IPC server
shell> client            # Launch IPC client
shell> keyboard_test     # Test keyboard input
shell> exit              # Exit shell
```

### IPC Demo

The kernel automatically launches the IPC server and client at boot. You'll see:

```
[client] start
[server] got ping 1
[client] got pong 1
[server] got ping 2
[client] got pong 2
...
[server] got ping 5
[client] got pong 5
```

### Serial Output

Connect to serial output for debugging:
```bash
qemu-system-x86_64 -cdrom build/microkernel.iso -serial stdio
```