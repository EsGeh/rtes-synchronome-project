# Synchronome Project

*REMARK*:
This project is under rapid development.
Some functionality might still be missing or incomplete.

![system block diagram](./doc/imgs/diagrams/1_block-diagram.svg)

*Short Description*:
This project is an exercise in realtime embedded engineering.
An embedded computer connected to a camera periodically takes pictures of a ticking clock and saves them to persistent memory (Flash).
The program outputs one image each second, each showing a still image of the seconds hand in one stationary position.
(or one image each 1/10th of a second if the clock displays 1/10ths of a second).
In order to achieve that, the process of taking/picking pictures needs to be carefully synchronized with the external clock.

![frame acquisition](./doc/imgs/diagrams/2_frame-acquisition.svg)

This project is the [fourth part](https://www.coursera.org/learn/real-time-project-embedded-systems) of the [Realtime Embedded Systems Specialization](https://www.coursera.org/specializations/real-time-embedded-systems) by the *University of Colorado Boulder* via Coursera.
According to the [course description](https://www.coursera.org/learn/real-time-project-embedded-systems) successful completion of this project proofs advanced level applied knowledge about realtime embedded engineering.

For details about grading and requirements for passing this course, see `./doc/requirements/`.

## Build

    $ ./scripts/build.fish

## Run

    $ ./scripts/run_capture.fish

## Run Tests

*Prerequisit*: install the [check](https://libcheck.github.io/check/) test framework

    $ ./scripts/test.fish

## Developer Notes

Auto run tests before every commit:

    $ git config core.hooksPath hooks
