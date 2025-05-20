# Synchronome Project - Timing Analyis

*Definitions: (same as in previous chapters)*

- Tc := "tick interval of external clock" ( = 1s or 0.1s for target/stretch goal)
- Ta := "acquisition interval" should be an integer multiple >3 of the external clock interval. Let `s_res` be the factor between Ta and Tc:

    s_res * Ta ~ Tc, s_res >= 3, s_res Integer

The system consists of the following services:

- S1: Frame Acquisition
- S2: Tick Recognition
- S3: Image Conversion
- S4: Files to Storage
- S5, ...: Other Image Processing Tasks

## Real-Time Requirements

For the following discussion, we assume that for each service the deadline is equal to its period: D = T.
S1, S2 and S3 are considered critical Services. The deadline is determined by the functional requirements:
In order to guarantee at least 1 frames between ticks and to fulfill the additional target requirement to measure and control drift, Ta should be less than half of a clock tick, preferrably Ta <= 1/3 * Tc.
S4 and others are not critical for synchronization issues and may therefore run as best effort services. In order to prevent data overflow, the period is required to be less than Tc *on average*.

In summary, we get:

| Deadline/Period | S1 | S2 | S3 | S4 |
|-----------------|----|----|----|----|
| T               | Ta | Ta | Tc | Tc |

## Real-Time Analyis and Design

In order to more specifically analyse the timing requirements and give concrete constraints, we need to specify a mapping of services to cpu cores.
The following table shows the chosen mapping in combination with the requirements.

| Deadline/Period | S1 | S2 | S3 | S4 |
|-----------------|----|----|----|----|
| T               | Ta | Ta | Tc | Tc |
| CPU             | 2  | 3  | 3  | 4  |

### Feasibility

#### S1 and S4

S1 and S4 occupy one core on their own. Therefore they must fulfill the trivial constraint U < 1.

#### S2 and S3

S2 and S3 share core 3.
According to the [Liu & Layland paper [1]](#references), feasibility is guaranteed for rate-monotonic (shortest-deadline-first) policy, as long as the total utilization below the least upper bound. For 2 processes, this criteria evaluates to:

U_{total} < LUB = 2*(2^(1/2) - 1) = 0.83 = 83%

## Concrete Measurements

The updated timing analysis is based on the program tracing runtime information such as start/end/runtime information via syslog.
The following diagrams show the data collected from logfiles for the case Tc = (1/10)s & Ta = (1/30)s. If feasibility can be shown, the requirements for the less challenging case of Tc = 1s, Ta = (1/3)s shall be trivially fulfilled.

![frame_acq runtime](./imgs/diagrams/statistics/frame_acq_runtime.svg)

![select runtime](./imgs/diagrams/statistics/select_runtime.svg)

![convert runtime](./imgs/diagrams/statistics/convert_runtime.svg)

![write_to_storage runtime](./imgs/diagrams/statistics/write_to_storage_runtime.svg)

Diskussion

- frame_acq: The execution time of frame acquisition solely depends on the minimum supported acquisition interval of the camera (the contribution to the runtime of additional code is minimal and should be neglegible). The minimum supported sample interval listed by the V4L2 driver is 1/30. The driver accepts values given as fraction of integers. As can be seen in the diagram, the driver chooses a slightly unequal distribution. This is no problem for this application, as long as the average interval is as requested and the variance is sufficiently slow.

- select, convert: From the diagrams, we can assume that the WCET for both services is < 0.01. Under this assumption, following the discussion above, this is always feasable for Tc = 1s and Tc = 0.1s:

    - Case Ta = 1/3:
        - U2 = 0.01s / (1/3)s = 3%
        - U3 = 0.01s / 1s = 1%
        - Utilization U = U2 + U3 = 4%
    - Case Ta = 1/30:
        - U2 = 0.01s / (1/30)s = 30%
        - U3 = 0.01s / (1/10)s = 10%
        - Utilization U = U2 + U3 = 40%

    Timing diagrams for these cases (units are 1s/300):

    ![scheduling diagram Ta=1/3, time in 1s/300](./imgs/diagrams/3_scheduling_core3.png)

    ![scheduling diagram Ta=1/30, time in 1s/300](./imgs/diagrams/4_scheduling_core3_extended.png)

- write_to_storage: This service is run as best effort task. The services are decoupled via queues, such that their runtime does not depend on each other. The average runtime must still be be below Tc to prevent the queues from overflowing. The occasional spikes are a result of buffered file I/O. Linux buffers write operations in RAM and occasionaly synchronizes the collected data to flash, which is remarkably slow. This demands the queue lenghts limit to be sufficiently high as well as other tasks involving file system access such as logging to be decoupled as well in order to prevent unintended race conditions for the file system.

## Enforcing Deadlines / Hard Real-Time

In order to proof hard real-time capabilities, a missed deadline in S1, S2 and S3 is considered fatal and the program will fail with a non-zero exit code.

## Drift

The following diagrams visualize drift by plotting the fractional part of start times for each service.
The diagram for the frame selection service shows, that the system indeed successfully detects external ticks and counteracts them by adjusting the frame numbering.

![frame_acq start time mod 1s](./imgs/diagrams/statistics/frame_acq_start.svg)

![select start time mod 1s](./imgs/diagrams/statistics/select_start.svg)

![convert start time mod 1s](./imgs/diagrams/statistics/convert_start.svg)

![write_to_storage start time mod 1s](./imgs/diagrams/statistics/write_to_storage_start.svg)

# Preliminary Design & Analysis (for Reference)

Running the `./scripts/run_statistics` on the destination platform prints information about the platform and tests for the worst-case and average runtime for relevant cpu-intensive parts of the services described.
These can be used to give a first estimation of WCET without doing an actual test run of synchronome program.
Before the program was in a mature state, services including runtime tracing via syslog implemented, the preliminary system design and analysis was based on these estimations.
These measurements are given here for completeness:

| service: task            | 320x240                       | 640x480                       | 1280x960                      |
|--------------------------|-------------------------------|-------------------------------|-------------------------------|
| S1: camera_get_frame     | max: 0.068028s avg: 0.066803s | max: 0.067995s avg: 0.066800s | max: 0.143959s avg: 0.141613s |
| S2: image_diff           | max: 0.001505s avg: 0.001478s | max: 0.006652s avg: 0.006592s | max: 0.041315s avg: 0.041092s |
| S3: image_convert_to_rgb | max: 0.001124s avg: 0.001094s | max: 0.015134s avg: 0.005367s | max: 0.031482s avg: 0.018423s |
| S4: image_save_ppm       | max: 0.017130s avg: 0.010344s | max: 0.117209s avg: 0.045070s | max: 0.239186s avg: 0.207855s |

For the minimal objectives Tc = 1s and taking into account the measured WECT for 320x240, we get:

| Parameters | S1    | S2    | S3    | S4    |
|------------|-------|-------|-------|-------|
| T in s     | 1/3   | 1/3   | 1     | 1     |
| C in s     | 0.068 | 0.002 | 0.001 | 0.017 |
| U in %     | 20%   | 0.45% | 0.11% | 1.71% |

For the target/stretch goals Tc = 0.1s, we get:

| Parameters | S1      | S2    | S3    | S4     |
|------------|---------|-------|-------|--------|
| T in s     | 1/30    | 1/30  | 0.1   | 0.1    |
| C in s     | 0.068   | 0.002 | 0.001 | 0.017  |
| U in %     | 204.08% | 4.52% | 1.12% | 17.13% |

The obvious issue of S1 exceeding the maximum period of 1/3 seems to be rooted is rooted in a flawed runtime measuring strategy and doesn't appear in practice (as has been demonstrated above).

## References

- [1] Liu, C. L.; Layland, J. (1973), "Scheduling algorithms for multiprogramming in a hard real-time environment", Journal of the ACM, 20 (1): 46–61, CiteSeerX 10.1.1.36.8216, doi:10.1145/321738.321743 
