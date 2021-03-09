# Shakra
Lightweight MIDI interface for Windows (and for macOS and Linux soon™).

## What is Shakra's purpose?
Lots and lots of synthesizers aren't available as MIDI drivers yet, and rather than deal with those messy and slow virtual MIDI cables (loopMIDI, MIDIYoke etc...), I decided to create a fast and low latency interface that allows applications to poke into the MIDI drivers' events buffer without having to deal with interfacing with WinMM (on Windows) and other interfaces.

## How can I create/host a synth that interfaces with Shakra?
There's no documentation yet, but you can take a look at [ShakraHost's code](https://github.com/KaleidonKep99/Shakra/tree/daddy/ShakraHost), which is a basic example of how you're able to interface with the driver and parse both short and long (SysEx) events.

## Will you publish pre-compiled binaries?
I will publish both pre-compiled binaries of ShakraDrv and a basic host synth, which should allow people to take a look at how Shakra works, and also allow some people to move away from [OmniMIDI](https://github.com/KeppySoftware/OmniMIDI).

# License
See [License](LICENSE.MD) for more info.
The driver itself (ShakraDrv) is GPL, but the audio host (ShakraHost) isn't, since it uses non-GPL material (e.g. BASS and BASSMIDI) to work.
