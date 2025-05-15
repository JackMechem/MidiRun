# :musical_keyboard: MidiRun

###### A small, simple, and minimal program for using any midi controller as a macro pad on Linux.

<details>

<summary>Table of Contents</summary>

#### Table of Contents

- [Support](#support)
  - [Linux](#support-linux)
- [Dependencies](#dependencies)
- [Compiling and Installing](#comp-and-installing)
  - [Install Dependencies](#cai-1)
    - [Arch Linux](#cai-1-1)
    - [Fedora Linux](#cai-1-2)
    - [MacOS (brew)](#cai-1-3)
  - [Clone and Compile](#cai-2)
    - [Clone](#cai-2-1)
    - [Build Directory](#cai-2-2)
    - [Build](#cai-2-3)
    - [Install](#cai-2-4)
- [Usage](#usage)
  - [Help Page](#help-page)
  - [Listing and Listening](#listing-io)
    - [Listing all IO](#listing-io-1)
    - [Listening to a Port](#listing-io-2)
  - [Running](#running)
    - [Default Location](#running-default)
    - [Custom Location](#running-custom)
- [Configuration](#configuration)
  - [Default Location](#configuration-default-path)
  - [Config Section](#configuration-config)
  - [Mapping Section](#configuration-mapping)
    - [Overall Syntax](#configuration-mapping-syntax)
    - [Fields](#configuration-mapping-fields)
    - [Key Codes](#configuration-mapping-key-codes)
  - [Example Configuration File](#configuration-example)

</details>

---

## Support <a name="support"></a>

##### :penguin: Linux <a name="support-linux"></a>

- Full support for Wayland and X11
  - Key Codes can be found in the [**key codes**](#configuration-mapping-key-codes) section.

## Dependencies <a name="dependencies"></a>

- [**rtmidi**](https://archlinux.org/packages/extra/x86_64/rtmidi/)
- [**jack (not for macOS)**](https://jackaudio.org/downloads/)
- [**toml++ (tomlplusplus)**](https://archlinux.org/packages/extra/x86_64/tomlplusplus/)
- [**cmake**](https://archlinux.org/packages/extra/x86_64/cmake/)
- [**gcc**](https://archlinux.org/packages/core/x86_64/gcc/)

## Compiling and Installing <a name="comp-and-installing"></a>

### 1. Install Dependencies <a name="cai-1"></a>

##### 1.1 Arch Linux <a name="cai-1-1"></a>

```
sudo pacman -S rtmidi tomlplusplus gcc cmake
```

##### 1.2 Fedora Linux <a name="cai-1-2"></a>

```
sudo dnf install rtmidi rtmidi-devel
```

##### 1.3 MacOS (brew) <a name="cai-1-3"></a>

```
brew install rtmidi tomlplusplus gcc cmake
```

### 2. Clone the Project and Compile <a name="cai-2"></a>

##### 2.1 Clone the Project <a name="cai-2-1"></a>

```
git clone https://github.com/JackMechem/MidiRun.git

cd MidiRun
```

##### 2.2 CD Or Make The Build Directory If It Doesn't Exist <a name="cai-2-2"></a>

```
mkdir build
cd build
```

##### 2.3 Build The Project <a name="cai-2-3"></a>

###### :warning: Must be in `MidiRun/build/` directory!

```
cmake ..        # Must be in the midirun/build directory!
cmake --build .
```

##### 2.4 Install The Project <a name="cai-2-4"></a>

###### :warning: Must be in `MidiRun/build/` directory!

```
sudo cmake --install .
```

##### See the [usage](#usage) and [configuration](#configuration) sections to configure.

## Usage <a name="usage"></a>

###### :warning: If there are any errors when trying to simulate keystrokes, try running with sudo. For a user to run MidiRun without sudo, they will need read and write permissions to `/dev/uinput`. [Possible Solution](https://stackoverflow.com/questions/11939255/writing-to-dev-uinput-on-ubuntu-12-04)

### Help Page <a name="help-page"></a>

##### To show this help page run `midirun -h`

```
Usage:
  Help and List:
    midirun {--help|-h} | Shows Help Page
    midirun [{--verbose|-v}] {--list-io|-lio} | Lists midi inputs and outputs
    midirun [{--verbose|-v}] {--listen|-ln} <Port-Number> | Listens to specified input port and displays midi note registered


  Running The Program:
    midirun [{--verbose|-v}] [{--config|-c} </path/to/config>] | Note: Default config is $HOME/.config/midirun/config.toml
```

## Listing Midi IO and Listen On Port <a name="listing-io"></a>

### Listing all inputs and outputs <a name="listing-io-1"></a>

###### :warning: Keep note of the port of the input device you want to use as this is used in the next step and the configuration file!

```
midirun --list-io
# OR
midirun -lio
```

##### Example output:

```
There are 2 MIDI input sources available.
  Input Port #1: Midi Through:Midi Through Port-0 14:0
  Input Port #2: Arduino Micro:Arduino Micro MIDI 1 16:0

There are 3 MIDI output ports available.
  Output Port #1: Midi Through:Midi Through Port-0 14:0
  Output Port #2: Arduino Micro:Arduino Micro MIDI 1 16:0
```

### Listening to a midi port <a name="listing-io-2"></a>

The command below listens to any inputs coming from the port given (in this example, port #2). <br />
Keep note of Byte 0 and Byte 1 for all of the buttons you want to map! <br /> Byte 0 is used to tell if a button is pressed down or released and Byte 1 translates to the midi note being played. <br />

###### :warning: This will be used in the configuration file later!

```
midirun -ln 2             # Replace the 2 with what ever port you are using
OR
midirun --listen 2        # Replace the 2 with what ever port you are using
```

## Running the Program <a name="running"></a>

###### :warning: If there are any errors when trying to simulate keystrokes, try running with sudo. For a user to run MidiRun without sudo, they will need read and write permissions to `/dev/uinput`. [Possible Solution](https://stackoverflow.com/questions/11939255/writing-to-dev-uinput-on-ubuntu-12-04)

### Running With The Default Configuration Path <a name="running-default"></a>

**The default configuration path is listed below. See the [configuration](#configuration) section to create a configuration file.**

###### :warning: The configuration file is NOT created automatically!!<br />

```
~/.config/midirun/config.toml
```

##### To run MidiRun with the default config path, simply run the command without arguments:

```
midirun
```

### Running With a Custom Configuration Path <a name="running-custom"></a>

##### To choose a different config path, use the `-c` or `--config` argument followed by the directly

###### :star: You can run more than one instance with different config files to use more than one device)

```
midirun -c /path/to/config.toml
# OR
midirun --config /path/to/config.toml
```

## Configuration <a name="configuration"></a>

###### :star: This project uses [toml](https://toml.io/) for its configuration file.

#### Default Configuration File Path <a name="configuration-default-path"></a>

```
~/.config/midirun/config.toml
```

### Config Section <a name="configuration-config"></a>

The config section only contains the input port.<br />
To find the input port, see the [listing io](#listing-io-1) section.

```
[config]
inputPort = 2 # The port of your input device
```

### Mapping Section <a name="configuration-mapping"></a>

The mapping section is composed of an array of tables named mapping. <br />

#### Overall Syntax <a name="configuration-mapping-syntax"></a>

```
[[mapping]]            # Every mapping entry is written exactly like this
name = "Bottom Left"   # Name of the mapping that shows up in console
byte0 = 144            # Byte 0 (button position) of the button or event
byte1 = 37             # Byte 1 (note) of the button or event
type = "command"       # Type of event either "command" or "key"
command = "kitty"      # command executed when the event is called

[[mapping]]
name = "Bottom Middle"
byte0 = 144
byte1 = 38
type = "key"
key = [2, 3, 4]        # Key codes that will be simulated when the event is called
```

#### Key Codes <a name="configuration-mapping-key-codes"></a>

##### Linux (linux input event codes)

```
sudo showkey --keycodes
```

Or [see this article (the section on identifying keycodes in console).](https://wiki.archlinux.org/title/Keyboard_input#Identifying_keycodes_in_console) <br/>

Or all key codes can also be found in [this header file](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h) located in the linux kernel source.

##### MacOS

###### :warning: Not full implemented!

Because of the way virtual keyboard inputs work on MacOS, every key code represents the physical key being pressed. Currently, it is not possible to consistently simulate capital letters or other letters or symbols that require shift or caps lock to be pressed (coming soon!).

A good guide for finding the key codes can be found [**here**](https://eastmanreference.com/complete-list-of-applescript-key-codes).

### Example Configuration <a name="configuration-example"></a>

#### Linux

```
[config]
inputPort = 2          # Port of the midi input device

[[mapping]]            # Every mapping entry is written exactly like this
name = "Bottom Left"   # Name of the mapping that shows up in console
byte0 = 144            # Byte 0 (button position) of the button or event
byte1 = 37             # Byte 1 (note) of the button or event
type = "command"       # Type of event either "command" or "key"
command = "kitty"      # command executed when the event is called

[[mapping]]
name = "Bottom Middle"
byte0 = 144
byte1 = 38
type = "key"
key = [2, 3, 4]        # Key codes that will be simulated when the event is called

[[mapping]]
name = "Bottom Right"
byte0 = 144
byte1 = 39
type = "command"
command = "thunar"
```

#### MacOS

```
[config]
inputPort = 3          # Port of the midi input device

[[mapping]]            # Every mapping entry is written exactly like this
name = "Bottom Left"   # Name of the mapping that shows up in console
byte0 = 144            # Byte 0 (button position) of the button or event
byte1 = 37             # Byte 1 (note) of the button or event
type = "command"       # Type of event either "command" or "key"
command = "open -a kitty"      # command executed when the event is called

[[mapping]]
name = "Bottom Middle"
byte0 = 144
byte1 = 38
type = "key"
key = [4, 14, 37, 37, 31] # Key codes that will be simulated when the event is called (translates to hello)

[[mapping]]
name = "Bottom Right"
byte0 = 144
byte1 = 39
type = "command"
command = "open -a firefox" # On MacOS, you have to use the open -a command to open applications.
```
