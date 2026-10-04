# Lambda1VR on the Valve Steam Frame

This is a build of Lambda1VR for Valve's Steam Frame. The Frame runs Android
apps in a container called Lepton (Android 11, arm64, GLES 3.2) on top of
SteamVR's OpenXR runtime. So it's the same Android app, with a few changes to
get it going there.

I haven't tried this in the headset yet. It builds, the APK looks right, and
that's all I can say so far. See the end for what's checked and what isn't.

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

The Frame controllers are a bit like a split gamepad. Left has the D-pad,
View, bumper, trigger, grip and stick. Right has A B X Y, Menu, bumper,
trigger, grip and stick. I kept the Quest layout wherever the Frame has the
button.

| Frame | Does |
| --- | --- |
| Right trigger, grip, stick | Same as Quest: fire, reload and secondary fire, turn and weapon select |
| Left stick, trigger, grip | Move, run, two-handed hold |
| Stick clicks | Use (right), laser sight and scope (left) |
| A, B | Crouch, jump |
| Left bumper or right X | Torch |
| View (tap) or right Y | Screen view, scoreboard in multiplayer |
| Right Menu | Pause and menu back |
| View (hold 1 s) | Recentre, with a buzz |
| View (hold 3 s) | Recentre and use the head height as standing height |

The Quest has X and Y on the left controller, which is why the torch and screen
view moved. Left-handed schemes use the same physical buttons.

If SteamVR hands the app Index or Touch controllers instead, they bind like they
do on Quest (Index has no Menu for apps, so pause is the left trackpad pressed
hard). The Frame layout only kicks in when the runtime says the Frame profile is
active.

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
- `TBXR_Common.c/.h`, `OpenXrInput.c`, `L1VR_SurfaceView.c`, `common/port.h`: the
  Frame code, all behind `L1VR_STEAM_FRAME`
- `#include <string.h>` in `TBXR_Common.c` and `OpenXrInput.c`, which `-Wall` asked for

## The Quest build

I built `assembleRelease` before and after these changes. The repo doesn't ship
the Meta loader, so both builds used the Khronos one as a stand-in to link.

- Everything except `libxash.so` is byte for byte the same.
- The `string.h` includes change how a few functions in `libxash.so` compile
  (strcpy becomes the checked version, memset gets inlined differently).
  Same behaviour.
- With those two includes taken out of the Quest build too, the code,
  read-only data, data, GOT, dynamic symbols and relocations of `libxash.so` are identical
  to the old build. Only the debug info and the build id differ, because line
  numbers moved.
- The Quest APK gets a `BuildConfig` class in the dex, since Java checks it now.

## Checked and not checked

Checked on the Mac:

- `assembleFrame` builds
- the APK has the pinned Khronos loader and the game libraries, arm64 only
- `aapt2 dump badging` says minSdk 30, no headtracking feature, no Quest or
  Pico metadata, package `com.drbeef.lambda1vr.frame`
- the Quest release build is as described above

Not checked yet:

- anything on the headset: install, start, the loader finding SteamVR,
  session start, what the eyes see, controllers, performance, standby, quitting
  from the SteamVR menu
- that the Frame controller paths are accepted by the runtime (the log lines say)
- the recentre buttons
- Pico: nothing there changed as far as I can tell, but I can't build or run it
