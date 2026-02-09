# Controller Configuration Guide

## Overview

This enhanced version of `drkMapleDevices.cpp` adds support for external controller configuration via a `controls.cfg` file, allowing you to customize button mappings and controller behavior without recompiling the code.

## Features

### ✨ New Features

1. **External Configuration File** - `controls.cfg` for easy customization
2. **Customizable Button Mappings** - Map any Wii Remote or GameCube button to any Dreamcast button
3. **Adjustable Dead Zones** - Fine-tune analog stick sensitivity
4. **Toggle Y-Axis Inversion** - Enable/disable Y-axis inversion
5. **Configurable Exit Combo** - Enable or disable the exit button combination
6. **Analog Stick as D-Pad** - Toggle whether analog stick movements trigger D-pad inputs

## Installation

1. Replace your existing `drkMapleDevices.cpp` with the new version
2. Place `controls.cfg` in the same directory as `nullDC.cfg`
3. Recompile your emulator

## File Location

The code automatically looks for `controls.cfg` in the same directory as your emulator:
```
/path/to/emulator/nullDC.cfg
/path/to/emulator/controls.cfg  ← Place your config here
```

## Configuration Options

### Analog Stick Settings

```ini
# Dead zone threshold (0-127)
analog_deadzone=20

# Invert Y-axis for GameCube controllers
invert_y_axis=true

# Enable analog stick to also trigger D-pad
analog_as_dpad=true
```

### Trigger Settings

```ini
# Threshold for analog triggers (0-255)
trigger_threshold=20
```

### System Settings

```ini
# Enable MINUS+PLUS or R+L+Z exit combination
enable_exit_combo=true
```

### Button Mappings

Format: `map=WII_BUTTON,GC_BUTTON,DC_BUTTON`

#### Available Buttons

**Wii Remote:**
- `WPAD_BUTTON_A`, `WPAD_BUTTON_B`
- `WPAD_BUTTON_1`, `WPAD_BUTTON_2`
- `WPAD_BUTTON_MINUS`, `WPAD_BUTTON_PLUS`
- `WPAD_BUTTON_HOME`
- `WPAD_BUTTON_UP`, `WPAD_BUTTON_DOWN`, `WPAD_BUTTON_LEFT`, `WPAD_BUTTON_RIGHT`

**GameCube Controller:**
- `PAD_BUTTON_A`, `PAD_BUTTON_B`, `PAD_BUTTON_X`, `PAD_BUTTON_Y`
- `PAD_BUTTON_START`
- `PAD_TRIGGER_L`, `PAD_TRIGGER_R`, `PAD_TRIGGER_Z`
- `PAD_BUTTON_UP`, `PAD_BUTTON_DOWN`, `PAD_BUTTON_LEFT`, `PAD_BUTTON_RIGHT`

**Dreamcast Controller:**
- `DC_A`, `DC_B`, `DC_X`, `DC_Y`
- `DC_START`
- `DC_C`, `DC_D`, `DC_Z`
- `DC_DPAD_UP`, `DC_DPAD_DOWN`, `DC_DPAD_LEFT`, `DC_DPAD_RIGHT`

## Example Configurations

### Default Configuration
Standard layout matching most Dreamcast games:
```ini
map=WPAD_BUTTON_A,PAD_BUTTON_A,DC_A
map=WPAD_BUTTON_B,PAD_BUTTON_B,DC_B
map=WPAD_BUTTON_1,PAD_BUTTON_Y,DC_Y
map=WPAD_BUTTON_2,PAD_BUTTON_X,DC_X
```

### Fighting Game Layout
Optimized for fighting games:
```ini
# Face buttons for punches
map=WPAD_BUTTON_A,PAD_BUTTON_X,DC_X
map=WPAD_BUTTON_B,PAD_BUTTON_Y,DC_Y
map=WPAD_BUTTON_1,PAD_BUTTON_A,DC_A

# Shoulder buttons for kicks
map=WPAD_BUTTON_MINUS,PAD_TRIGGER_L,DC_B
map=WPAD_BUTTON_PLUS,PAD_TRIGGER_R,DC_C
```

### Platformer Layout
Optimized for platformer games:
```ini
# Jump on A
map=WPAD_BUTTON_A,PAD_BUTTON_A,DC_A

# Run on B
map=WPAD_BUTTON_B,PAD_BUTTON_B,DC_B

# Action on shoulder
map=WPAD_BUTTON_PLUS,PAD_TRIGGER_R,DC_X
```

### Racing Game Layout
```ini
# Accelerate on A
map=WPAD_BUTTON_A,PAD_BUTTON_A,DC_A

# Brake on B  
map=WPAD_BUTTON_B,PAD_BUTTON_B,DC_B

# Handbrake on shoulder
map=WPAD_BUTTON_PLUS,PAD_TRIGGER_R,DC_Y

# Higher sensitivity for racing
analog_deadzone=10
```

## Troubleshooting

### Config Not Loading

**Problem:** Changes to `controls.cfg` aren't being applied

**Solutions:**
1. Ensure `controls.cfg` is in the same directory as `nullDC.cfg`
2. Check file permissions (must be readable)
3. Look for console output: "Controls configuration loaded successfully"
4. Verify file path by checking console output

### Buttons Not Working

**Problem:** Button presses aren't registering

**Solutions:**
1. Check button names match exactly (case-sensitive)
2. Verify mapping format: `map=WII,GC,DC` with no spaces
3. Use `0` for unmapped buttons, not blank
4. Maximum 16 button mappings supported

### Analog Stick Issues

**Problem:** Analog stick too sensitive or not responsive

**Solutions:**
1. Adjust `analog_deadzone` (try values 10-30)
2. Toggle `invert_y_axis` if controls feel backwards
3. Disable `analog_as_dpad` if D-pad interferes

### Exit Combo Not Working

**Problem:** Can't exit emulator with button combo

**Solutions:**
1. Verify `enable_exit_combo=true`
2. Try MINUS+PLUS on Wiimote
3. Try R+L+Z on GameCube controller

## Technical Details

### Configuration Loading

The configuration is loaded once during `InitControllers()`:

1. Attempts to read `controls.cfg` from application directory
2. Falls back to default configuration if file not found
3. Parses each line for key=value pairs
4. Ignores comments (lines starting with #)
5. Applies configuration immediately

### Default Behavior

If `controls.cfg` is missing or cannot be read:
- Uses built-in default mappings
- Analog deadzone: 20
- Trigger threshold: 20
- Y-axis inversion: enabled
- Analog as D-pad: enabled
- Exit combo: enabled

### Performance Impact

Minimal performance impact:
- Configuration loaded once at startup
- Mapping lookup uses simple array iteration
- No file I/O during gameplay

## Code Changes Summary

### New Functions
- `LoadControlsConfig()` - Loads and parses controls.cfg
- `LoadDefaultConfig()` - Sets default values
- `StringToWiiButton()` - Converts string to Wii button constant
- `StringToGCButton()` - Converts string to GC button constant  
- `StringToDCButton()` - Converts string to DC button constant
- `TrimWhitespace()` - Helper for parsing
- `ParseBool()` - Helper for parsing

### Modified Functions
- `MapButtons()` - Now uses configuration mappings
- `MapAnalogStick()` - Now uses configurable deadzone and Y-axis inversion
- `CheckExitCombination()` - Now respects enable_exit_combo setting
- `InitControllers()` - Now calls LoadControlsConfig()

### New Data Structures
- `ButtonMapping` - Stores a single button mapping
- `ControlsConfig` - Stores all configuration options

## Future Enhancements

Potential additions for future versions:
- Per-port button mappings (different configs for each controller)
- Analog trigger support (not just digital on/off)
- Analog stick sensitivity scaling
- Multiple profile support (load different configs)
- Rumble settings
- Hot-reload configuration without restart

## Support

If you encounter issues:
1. Check console output for error messages
2. Verify your `controls.cfg` syntax
3. Try the default configuration first
4. Ensure you're using compatible controller types

## License

Same license as the original drkMapleDevices.cpp file.
