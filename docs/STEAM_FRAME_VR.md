# Lambda1VR on the Valve Steam Frame

This is a build of Lambda1VR for Valve's Steam Frame. The Frame runs Android
apps in a container called Lepton (Android 11, arm64, GLES 3.2) on top of
SteamVR's OpenXR runtime. So it's the same Android app, with a few changes to
get it going there.

I haven't had it on my head yet. It builds, it runs on the Frame (unattended, on a desk) and it
draws the first level in both eyes. See the end for what's checked and what isn't.

## Build

You need JDK 17 and NDK 25.1.8937393, same as the Quest build.

```
cd Projects/Android
sh ./gradlew assembleFrame
```

The APK ends up in `Projects/Android/build/outputs/apk/frame/lambda1vr-frame.apk`.
It's signed with the debug key.

The first build downloads the Khronos OpenXR loader (1.1.63) from Maven
Central, checks its SHA-256 and unpacks it into `Projects/Android/build/`. Nothing
gets committed. The Quest builds don't touch it.

## Install and run

Start "Lepton Development" on the Frame and use adb over the network:

```
adb connect <frame-ip>:5555
adb install -r lambda1vr-frame.apk
adb shell am start -n com.drbeef.lambda1vr.frame/com.drbeef.lambda1vr.GLES3JNIActivity
```

Lepton Development deletes the apps you install when it exits. For something that
sticks around you can add the APK as a devkit title in Steam on the Frame. The
game data lives outside the container either way (next section).

Launched from adb the app doesn't get controllers or focus from SteamVR, so use
that for logs. Start it from the Steam library to actually play.

## Game data

Copy the `valve` folder from your Steam copy of Half-Life to
`~/Documents/Lambda1VR/valve` on the headset. Inside Lepton that's
`/sdcard/Documents/Lambda1VR/valve`. Documents is the headset's own folder, so
it's still there after a Lepton reset. On the Quest it's `/sdcard/xash`, that
hasn't changed.

The app makes the folder on first start and puts its own files in it (config,
the weapon models and so on), like it does on the Quest.

## Controls

The Frame's controllers are a split gamepad. Left has the stick, trigger, grip, bumper, D-pad and
View. Right has the stick, trigger, grip, bumper, A B X Y and Menu.

Half-Life's 25th anniversary update ships its own Steam Input layout for a gamepad
(`controller_configs/xbox_controller_config_standard.vdf` in the game's `valve` folder). I used that
one, so it works like the gamepad version people already know, and added the VR parts: your head looks,
and the grips are your hands.

| Frame | Playing |
| --- | --- |
| Left stick | Move |
| Left stick click | Crouch (toggle). Physically ducking still works |
| Right stick left / right | Turn (Lambda1VR's snap or smooth turn) |
| Right stick click | Flashlight |
| Right stick up | Flat screen view (the scoreboard in multiplayer) |
| Right stick down | Laser sight, and steadies a scope |
| Weapon hand trigger | Fire |
| Other hand trigger | Alt fire |
| A | Jump |
| B | Crouch (while held) |
| X | Use |
| Y | Reload |
| Left / right bumper | Previous / next weapon (fire to pick it, like in Half-Life) |
| D-pad up / down | Previous / next weapon, same as the bumpers |
| D-pad right | Last weapon |
| D-pad left | Crowbar |
| Menu | Pause menu. Held for half a second: quick save |
| View | Flashlight (the scoreboard in multiplayer) |
| View, held 1 s | Recentre, with a buzz |
| View, held 3 s | Recentre and use the head height as standing height |
| Weapon hand grip | Use from that hand: grab, push or pull things, press buttons. Behind your head it pulls the crowbar from the backpack |
| Other hand grip | Near the weapon: the two-handed hold (steadier aim, a scope). Well away from it: use from that hand |

The weapon hand is the right one, or the left one with `vr_control_scheme 10`. Left-handed swaps the
roles that belong to a hand (the triggers, the grips, the laser, where the gun is) and leaves the buttons
where they are.

The grips use what Lambda1VR already had: the game looks for things you can use near the hand and in
front of it, from where the controller is, not from your head. The other grip decides what it is when you
press it, by how far the hands are from each other (under 35 cm it's the gun hold, over 55 cm it's use, in
between it stays what it was last time), and keeps that until you let go. The log says which it picked.
`vr_gesture_triggered_use` has to stay on for this, it's what gives the short reach and use from the other
hand. Waving a hand about doesn't use things on the Frame, double clicking jump doesn't crouch.

Kept as it was: aiming from the controller, swinging the crowbar, the flashlight beam from your off hand
(`vr_headtorch` and `vr_reversetorch` still work), the two-handed scope, reaching behind your head for the
crowbar, ducking by ducking, walking around the room, haptics, the highlight on things you can use. With
your other hand behind your head, the flashlight and View buttons are quick save, and the right stick up
is quick load, as on the Quest. Walk (the slow key) is gone, the stick is analog.

| Frame | In a menu |
| --- | --- |
| Weapon hand laser and trigger | Point and click |
| A | Confirm |
| B, Menu, View | Back |
| D-pad, left stick | Move |
| Left / right bumper | Previous / next tab or page |
| Right stick up / down | Scroll |

The map is one table, `VrFrameMap.c`, and `tools/steam_frame/tests` in the dev branch checks it for an
input that does two things in one situation and for actions nothing can reach.

If SteamVR hands the app Index or Touch controllers instead, they bind like they do on Quest (Index has no
Menu for apps, so pause is the left trackpad pressed hard). The Frame layout only kicks in when the runtime
says the Frame profile is active.

## What's different from the Quest build, and why

- There's a `frame` build type, not a flavour. A flavour would make `assembleDebug`
  and `assembleRelease` build it as well, and the Quest builds should stay as
  they are. minSdk is 30 there because Lepton is Android 11 (the Quest build is 32).
- The manifest overlay (`Projects/Android/src/frame`) drops the headtracking
  requirement, since the container doesn't have that feature and the install
  would fail, and the Quest and Pico metadata.
- Lepton has no OpenXR loader broker, so the APK carries the Khronos loader.
  Java loads it as `openxr_loader`.
- Only OpenGL ES is required from the runtime. Everything else is enabled if
  the runtime lists it. There's no Pico or Meta code on that build.
- It renders at the runtime's recommended eye size. The Quest build uses 1.3x of
  it, which is a lot on the Frame.
- When the headset is in standby (`shouldRender` is false) the frame goes in with
  no layers and the game doesn't simulate. Without that the server kept running.
- If SteamVR quits the app the game shuts down the normal way.
- The eye framebuffers use plain GLES 3 on the Frame, so the runtime's GL doesn't
  need `EXT_multisampled_render_to_texture`. It's one swapchain per eye, there's
  no multiview.
- The HUD, game text and titles (the tram credits) stay level with the world when you tilt your head. They
  were drawn into the eye image, so they rolled with your head. The HUD pass is turned the other way by the
  roll the world is drawn with, about the middle of each eye's view, so it follows your head's turn and
  nod but not its roll. The menus (a panel fixed in the world) and a scope are as they were.
- Each eye is drawn from the eye pose the runtime gives (position, and rotation if the eye
  has one, in head space), and each projection view goes in with that eye's own pose. The
  Quest build moves each eye by a fixed 65 mm along the side and submits both views with
  the head's pose. On my Frame the runtime says the eyes are at +-34.5 mm (68.9 mm apart,
  that's the headset's IPD setting), with no rotation, so for the world the difference is small. The
  fixed pose in the layer was the wrong thing to tell the compositor though.
- The HUD sits `vr_hud_distance` metres in front of you (archived cvar, default 1.0). The
  client DLL (a submodule I can't touch) shifts every HUD item by width / 36 per eye, which
  came to about 0.4 m on the Frame and was hard to look at. While the HUD draws, the DLL is told
  the eyes are the mono ones (it adds nothing then), and the HUD is shifted instead by the
  pixels that put it at that distance, from that eye's real position and focal length. A
  scope and the flat menu screen are left as they were.
- The eye framebuffers are made through gl4es but their attachments are set with the
  driver's own GL (`eglGetProcAddress`). The plain `gl*` names in `libxash.so` are gl4es's,
  and it swaps a texture it doesn't know for an empty one of its own.
- The engine's messages go to logcat under the tag `Xash`. They always did, but the developer
  level defaulted to 0, so nothing came out. It's 3 on the Frame build; `-dev` in
  `commandline.txt` still wins.
- `vr_refresh` is taken from the frame period, not from the refresh rates the runtime lists.
  SteamVR sets the rate per app, and it offered 90 Hz while the display ran at 72.
- The log lines that start with `[openxr]` say what was picked: runtime, enabled
  extensions, swapchain format and size, session state, which profiles got
  bindings and what the runtime answered.
- The all-files permission screen doesn't exist on the Frame. There the app
  checks that it can write to the data folder and only asks if it can't.

## Files I changed that the Quest build uses too

- `Projects/Android/build.gradle`: the frame build type, the loader download,
  `buildConfig` on (for `BuildConfig.STEAM_FRAME`)
- `Projects/Android/gradlew` and `gradlew.bat`: the wrapper jar and properties
  were there already, the scripts weren't
- `Projects/AndroidPrebuilt/jni/Android.mk`: the Frame build says which loader
  to link
- `Projects/Android/jni/src/Xash3D/xash3d/engine/Android.mk`: `L1VR_STEAM_FRAME`,
  and `-Wall` for the VR sources
- `java/com/drbeef/lambda1vr/GLES3JNIActivity.java`: loader, data folder,
  permission check
- `TBXR_Common.c/.h`, `OpenXrInput.c`, `L1VR_SurfaceView.c`, `VrEyeMath.c/.h` (new),
  `VrCvars.h`, `common/port.h`, and in the engine `gl_rmain.c`, `gl_draw.c`, `cl_game.c`,
  `cl_scrn.c`: the Frame code, all behind `L1VR_STEAM_FRAME`
- `#include <string.h>` in `TBXR_Common.c` and `OpenXrInput.c`, which `-Wall` asked for

## The Quest build

I built `assembleRelease` before and after these changes. The repo doesn't ship
the Meta loader, so both builds used the Khronos one as a stand-in to link.

This was done before the shutdown fix below, and the comparison wasn't redone after it.

- Everything except `libxash.so` is byte for byte the same.
- The `string.h` includes change how a few functions in `libxash.so` compile
  (strcpy becomes the checked version, memset gets inlined differently).
  Same behaviour.
- With those two includes taken out of the Quest build too, the code,
  read-only data, data, GOT, dynamic symbols and relocations of `libxash.so` are identical
  to the old build. Only the debug info and the build id differ, because line
  numbers moved.
- The Quest APK gets a `BuildConfig` class in the dex, since Java checks it now.
- One real change for Quest too: `jVM` in `L1VR_SurfaceView.c` was never set, so
  `jni_shutdown` dereferenced NULL. The compiler drops everything after a call like
  that, which made the quit path in `AppThreadFunction` run off the end of the function
  (an abort on the Frame). `JNI_OnLoad` sets it now. I haven't built or run it on a Quest.

## Checked and not checked

Checked on the Frame, unattended (headset on a desk, nobody wearing it, started over adb):

- the Khronos loader finds SteamVR 2.17.10, 1728x1728 swapchains, the Frame, Index and Touch
  binding suggestions all come back `XR_SUCCESS`
- the session goes through READY and SYNCHRONIZED, the frame loop holds 72 fps
- the first level (`c1a0`) loads from the Steam Half-Life files and both eyes show it, with
  a sensible difference between the eyes, turned 90 degrees too
- the app quits cleanly when told to
- the stereo, measured on the eye images with rays traced into the map: surfaces at 2.4 to
  10 m land within 0.6 px of the interpupillary distance times the focal length over
  the depth, a wall 58 m away shows 0.1 px where 0.7 is right, and the HUD at
  `vr_hud_distance` 1 and 2 measured 41.0 and 21.0 px of disparity (41.4 and 20.7 expected)

Checked on the Mac:

- `assembleFrame` builds
- the APK has the pinned Khronos loader and the game libraries, arm64 only
- `aapt2 dump badging` says minSdk 30, no headtracking feature, no Quest or
  Pico metadata, package `com.drbeef.lambda1vr.frame`
- the Quest release build is as described above

Not checked yet:

- anything with a person in the headset: how it looks and feels, scale, the HUD,
  controllers, performance, quitting from the SteamVR menu
- starting it from the Steam library (so far only over adb)
- the replacement weapon and hand models: the repo's `assets` don't have the `.mdl`
  files (they're git-ignored), so the engine logs that it can't load `v_hand.mdl` and
  friends
- that the Frame controller paths are accepted by the runtime (the log lines say)
- the recentre buttons
- Pico: nothing there changed as far as I can tell, but I can't build or run it
