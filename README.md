# Godot Audio Profiler
A Godot extension that adds a new "Audio" tab to the debugger.

![Screenshot 2025-05-12 205949](https://github.com/user-attachments/assets/27336d8d-5943-4721-a4c6-afd1f5a197a7)

## How to install

1. Download the latest [release](https://github.com/pirey0/godot-audio-profiler/releases)
2. Unzip the content into your project's "addons" folder.

## What is it for?
When implementing sounds and music in Godot it is often very hard and/or painful to debug and solve issues.
This debugger keeps track of all playing AudioStreamPlayers and displays useful information about them to improve productivity when integrating or debugging sounds.

Here is a set of questions, that are normally quite bothersome, that you can answer quickly by using this tool:

### 1. Why can't I hear anything?
Consider the following scenario: 

You've setup a new SFX, hooked it up to code correctly (allegedly), and started testing, but you end up hearing nothing.

There are many potential causes for this, and often, you end up having to check all of them.
- The player doesn't exist or was not instantiated correctly.
- The player exists, but isn't actually playing.
- The player is playing, but it's just too quiet to be heard.
- The player is playing, but it's being channeled to the wrong bus.
- The player is playing, but its spatial attenuation is too strong/restrictive.
- The player is technically playing, but it keeps getting restarted every frame.
- The player is playing at the right volume; you just forgot to connect your headset.

#### Heres how you can use the Audio profiler to clear up situations like this quickly:
Trigger the interaction, and use the search box to find the releveant players. 
![Screenshot 2025-05-12 213411](https://github.com/user-attachments/assets/40c6e4ab-d6ee-4749-8c31-68725c8409f8)
1. The Player exists, because it's listed ✔️
2. THe Player is playing correctly, because the "On" column displays an *O* after sitting.  ✔️
3. The player is not getting restarting every frame, because the Playback Time progresses correctly. ✔️
4. The Bus is correct. ✔️
5. The mixed volume is a bit too low. ❌

In this case, the mixed volume (which is the combination of volume_db, bus attenuation and spatial attenuation) is simply too quiet. Either the spatial attenuation is off, the volume_db property should be turned up higher, or the Stream itself was not Normalized/Balanced correctly.


### 2. What is that damn sound?!
Consider the following scenario:

You are testing your game when suddenly you hear a sudden *weird sound*, completely of place. You have no clue where it comes from, it could be anything, really.

By switching on "Loudest Only" mode, only the top 10 loudest sounds are shown.
This allows you to quickly identify where the sounds you are hearing are actually coming from.
![Screenshot 2025-05-12 214553](https://github.com/user-attachments/assets/f099b86c-f904-429d-ad22-234b4f23e9e4)


### 3. Why is the sound system eating up so much performance?!
It's really easy in Godot to forget to turn off audio players.
Sometimes we write code that changes the volume dynamically and end up with mute-players at -80db in perpetuity.
Sometimes the spatial attenuation is just setup incorrectly and we forget the sound even existed.

Every playing AudioStreamPlayer (especially 2D and 3D) comes with some overhead every frame.
You can use the Audio profiler to find these lingering mute players.
![Screenshot 2025-05-12 214915](https://github.com/user-attachments/assets/767ca19d-2b3a-437e-a3f5-16aee45b8cda)


