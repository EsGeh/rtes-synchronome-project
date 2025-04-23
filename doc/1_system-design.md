# Synchronome Project - Preliminary System Design

## Platform Specification

Platform:

- OS: linux
- Board: Raspberry Pi Model 4B
- Camera: C270 HD WEBCAM

## Implementation Details

- Implementation in C
- Frame acquisition from the camera uses the Video for Linux ver.2 API (V4L2) and uses the streaming I/O method aka *memory mapping*
- Frames should *never* be copied but shared/accessed via pointers for performance reasons

## System Design

*Definitions:*

- Tc := "tick of external clock" ( = 1s or 0.1s for target/stretch goal)
- Ta := "acquisition rate" ( < 1/2 * Tc, preferrably < 1/3 * Tc )

**REMARK:** This is a preliminary design and might change.

![System Components Overview](./imgs/diagrams/5_block-diagram-detail.svg)

### Resources

*Remarks:*
Buffers are used to decouple read and write access. The practical implementation (Queue, Ringbuffer,...) is yet to be decided upon. The behaviour of the chosen mechanism, (ie. read blocking if empty, write blocks if full) will impose dependencies between services and might impact timing constraints.

- Camera: repeatedly sampled to acquire frames

- (Internal) Clock: Abstraction around the system clock as available through posix OS api. The abstraction will have to provide functions to get current time and to sleep for a short time frame (like posix `clock_gettime` and `nanosleep`). Additionaly, small adjustments to the clock must be supported in order to counteract phase drift against the external clock. This functionality might somehow be emulated by a wrapper API around posix time functions without actually changing the system clock.
- Qa: buffer of frames acquired from the camera
- Qs: buffer of selected frames
- Qrgb: buffer of selected frames in rgb format

### Services

**Remark:** Pseudocode for list manipulation follows haskell-like syntax.

#### S1: Frame Acquisition

**Idea:** aquire frames from camera and attach time stamps

```plantuml
start
repeat
:**time** = Clock.get_time();
:**frame** = Camera.get_frame();
note right
blocks until frame has been captured ...
end note
:**frame.time** = **time**;
:push Qa = (frame:Qa);
:Clock.sleep_until(**time**+Ta);
repeat while(true)
```

#### S2: Tick Recognition 

**Idea:**

- Don't yet specify implementation details
- For each cycle, the service can in principal inspect multiple captured frames from the past
- The frame selection has a lot of information (frames have timestamps attached) to make decisions

```plantuml
start
:**n** = 0
**last_ticks** = [];
repeat
:dump frames older **frame[n].time - Ts** from Qa;
:look at latest frames (...:frame[n]:frame[n-1]:...) = Qa;
if (a new tick has been found at **tick_time**) is (true) then
:select **snapshot_frame** between latest 2 ticks and add it to Qs;
:**last_ticks** = (**tick_time**:**last_ticks**);
endif
if (**tick_time** deviates from prediction based on **last_ticks**) is (true) then
:adjust Clock for small amount;
endif
:**n** ++;
repeat while(true)
```

#### S3: Image Conversion

**Idea:**

- Keep it simple

```plantuml
start
repeat
:get next (**frame**:**rest**) = Qs;
:calculate **frame_rgb** from **frame**;
:Qrgb = (**frame_rgb**:Qrgb);
:delete Qs = **rest**;
repeat while(true)
```

#### S4: File To Storage & S5,...: Other Image Processing

**Idea:**

- Keep it simple

```plantuml
start
repeat
:get next **frame_rgb** from Qrgb;
:save **frame_rgb** to filesystem;
:vote for deletion of **frame_rgb** from Qrgb;
repeat while(true)
```

Other image processing tasks S5,... have a similar control flow with a different action instead.

**Notes:**

- deletion of frames from Qrgb is postponed until all Services (S4,S5,...) have agreed.
