# Embedded Responsive Visualizer

An interactive embedded visualization application developed in C for the STM32 Nucleo-L152RE microcontroller. This project features a custom-built 2D graphics library, a responsive touch-based user interface, and real-time 3D rendering capabilities controlled by physical hardware inputs.

## Key Features

* **Custom 2D Graphics Engine:** Built from scratch to interface with an ILI9341 TFT display over SPI.
* **DMA Acceleration:** Utilizes Direct Memory Access (DMA) for bulk pixel transfers, enabling rapid screen clearing and efficient shape rendering without stalling the CPU.
* **Responsive Touch Control:** Integrates a TSC2007 resistive touch controller via I2C. Uses EXTI hardware interrupts combined with software-based double-sampling to filter noise and register accurate touch coordinates.
* **Multi-Screen UI:** A navigable, state-machine-driven interface allowing users to switch between Home, Data, and Visualization menus seamlessly using touch inputs.
* **Real-Time 3D Rendering:** Features a dynamic 3D wireframe cube animation that rotates in real-time.
* **Hardware Integration:** The 3D animation's rotation is physically controlled by a rotary encoder, leveraging a hardware timer (TIM2) for precise input tracking.

## Hardware Requirements

* **Microcontroller:** STM32 Nucleo-L152RE
* **Display:** ILI9341 TFT LCD Display (SPI)
* **Touch Controller:** TSC2007 Resistive Touch Controller (I2C)
* **Input:** Standard Rotary Encoder

## System Architecture

* **SPI & DMA:** Used for high-speed communication with the display. DMA handles the heavy lifting of pixel drawing, freeing up the CPU for UI logic and matrix math.
* **I2C & EXTI:** Used for touch detection. An interrupt is triggered on touch, and a double-sampling routine ensures that the ADC noise inherent to resistive touchscreens is filtered out.
* **Timers:** TIM2 is configured in encoder mode to track the rotary encoder's position, directly mapping physical rotation to the mathematical rotation of the 3D rendered cube.

## Getting Started

1. Clone this repository to your local machine.
2. Open the project in your preferred STM32 development environment (e.g., STM32CubeIDE).
3. Connect your STM32 Nucleo-L152RE board.
4. Ensure the ILI9341, TSC2007, and rotary encoder are wired to the correct SPI, I2C, and Timer GPIO pins as defined in the configuration files.
5. Build the project and flash the firmware to the microcontroller.

## Demo

https://github.com/user-attachments/assets/f3926307-3fa5-4e62-a14b-6ba41dd3203b
