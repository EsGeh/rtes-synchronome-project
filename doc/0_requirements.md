# Synchronome Project - Requirements ^[This is a summary of the documents in <./doc/resources> given as requirements and grading criteria of the course.]

## Minimal Objectives

Input: Images acquired from stationary digital camera facing an analog clock with hour, minute and second hands (or a stop watch with 1/10th hand \*)

Output: selection of camera frames for which the following conditions hold:

- Frames encode a time-lapse of the clock at 1Hz (/10Hz \*)
- Bijection (1:1 relation) between distinguishable clock hand positions and selected frames
    - Every frame shows a unique state of hands on the analog clock
    - There is a frame for every distinguishable stationary position of clock hands
- Skipped, blurred or repeated frames are considered an error
- All frames not-blurry (all hands stationary)
- 1800+1 frames (frame 0...1800) that means at 1Hz runtime = 1800s = 30min (at 10Hz: runtime 180s = 3min \*)
- Resolution >= 320x240

- Frames stored to disk, where
    - Every frame stored as separate file
    - Every frame stored as [Portable Anymap](https://de.wikipedia.org/wiki/Portable_Anymap) (.ppm P3/P6 or .pgm P2/P5)
    - Every frame contains a timestamp in the file header or in the image
    - Every frame contains system info from `uname -a` in the file header or in the image

Additional objective: compute and plot average jitter and drift

## Target Objectives

**One or more** of the following features:

- Program saves frames at 10Hz and runs stable (glitches are allowed)
- Image processing or frame compression
- Continuous streaming or automatic periodic download of frames, so that the service never runs out of flash memory
- another image processing feature that can be toggled during runtime

## Stretch Goals (Full Credit)

- Program acquires frames at 20Hz
- Program outputs frames at 1Hz or 10Hz without glitches and with continuous download 
- Program runs glitch-free with an an external digital clock that updates 1/10th of a second
- Any other frame-by-frame image processing at 1Hz and 10Hz
- Switching single or mutliple features on/off during runtime does not create stability issues (jitter, drift, monotonicity)
