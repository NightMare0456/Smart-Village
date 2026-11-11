# Smart Village: Day and Night Scenario

**Computer Graphics Project \| C++ \| OpenGL 3.3 Core \| GLFW \| GLAD**

## 1. Project Overview

Smart Village is a 2D computer graphics project developed using C++ and
OpenGL. It represents a village environment containing houses, a school, a
shop, a super shop, a playground, roads with footpaths and a zebra
crossing, a working traffic signal, walking people, trees, a pond,
mountains, streetlights, and moving vehicles.

The project demonstrates basic computer graphics concepts such as
geometric shapes, object positioning, layering, transformations,
animation, lighting, and scene transitions. It supports day and night
scenes, an automatic day/night cycle, and animated rain, traffic, pedestrians, shops and a playground.

## 2. Main Features

-   **Village environment:** Houses, school, roads, trees, pond, and
    mountains.
-   **Shop:** A small village shop with a tin roof, striped awning,
    goods on shelves and a shopkeeper.
-   **Super shop:** A larger shop with a lit "SUPER SHOP" sign, display
    windows full of products, and a glass door.
-   **Footpaths:** Paved footpaths on both sides of the main road.
-   **Traffic:** A traffic signal (green / yellow / red cycle). Cars and
    a school bus stop at the stop line on red, keep a safe distance from
    each other and move again on green.
-   **Humans:** People walk along the footpaths, pedestrians use the
    zebra crossing only while the traffic is stopped, and children play
    in the playground. People open umbrellas when it rains.
-   **Playground:** Animated swings, a seesaw and a slide with children
    playing.
-   **Day mode:** Displays the village in daytime.
-   **Night mode:** Changes the scene to a nighttime appearance.
-   **Automatic day/night cycle:** Gradually transitions between day and
    night.
-   **Moving vehicles:** Two cars and a school bus travel on the road.
-   **Animated rain:** Rain, puddle ripples and splashes can be
    enabled and stopped using the keyboard.
-   **Lighting effects:** Streetlights and ambient brightness contribute
    to the scene's appearance.
-   **Animation control:** Animation can be paused and resumed.

## 3. Teacher's Requirement Checklist

  Requirement     Where it appears in the scene
  --------------- -------------------------------------------------------
  Shop            Small shop at the far left of the village row
  Traffic         Traffic lights + vehicles that obey them
  Human           Walking people, zebra-crossing pedestrians, children
  School          "SUNRISE SCHOOL" building with flag
  Super shop      "SUPER SHOP" building next to the side road
  Footpath        Paved footpaths on both sides of the road
  Rain            Animated rain, ripples, splashes (key `R`)
  Playground      Swings, seesaw and slide at the right side

## 4. Project Structure

The following is the intended project structure. Keep the names and
locations consistent with the files in your actual project.

``` text
SmartVillage_Project/
├── build/
│   └── glfw3.dll
├── include/
│   ├── glad.h
│   └── glfw3.h
├── lib/
│   └── (GLFW library files, if provided locally)
├── src/
│   ├── glad.c
│   └── main.cpp
├── Makefile
└── README.md
```

**Note:** The exact contents of `include/`, `lib/`, and `build/` depend
on your downloaded GLFW and GLAD files. The GLFW DLL must be available
beside the executable when running on Windows.

## 5. Keyboard Controls

  Key       Action
  --------- ----------------------------------
  `D`       Switch to day mode
  `N`       Switch to night mode
  `A`       Enable automatic day/night cycle
  `R`       Toggle rain on or off
  `S`       Stop rain
  `Space`   Pause or resume animation
  `ESC`     Exit the application

## 6. Technologies Used

-   **C++** --- Main programming language.
-   **OpenGL 3.3 Core** --- Rendering 2D graphics.
-   **GLFW** --- Window creation and keyboard input handling.
-   **GLAD** --- Loading OpenGL functions.
-   **GLSL shaders** --- Processing vertex and fragment data for
    rendering.

## 7. Windows Setup (MSYS2 MinGW 64-bit)

### Step 1: Install MSYS2

Download and install MSYS2 from:

https://www.msys2.org/

### Step 2: Install the required packages

Open the **MSYS2 MinGW 64-bit** terminal and run:

``` bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-glfw
```

### Step 3: Configure PATH

Add the following directory to your Windows PATH if it is not already
included:

``` text
C:\msys64\mingw64\bin
```

### Step 4: Open the project directory

Use the MSYS2 MinGW 64-bit terminal and navigate to the folder
containing the `Makefile`.

### Step 5: Build and run

Run:

``` bash
mingw32-make win
```

If your environment uses `make`, run:

``` bash
make win
```

According to the intended Makefile setup, the Windows executable is
created at:

``` text
build/SmartVillage.exe
```

The `win` target should also launch the executable if the Makefile is
configured to do so.

**Important:** The command depends on the `win` target being present in
your Makefile. The GLFW DLL must be placed beside `SmartVillage.exe` in
the `build/` directory.

## 8. Linux Setup

On Ubuntu or a Debian-based distribution, install the compiler, Make,
and GLFW development package:

``` bash
sudo apt update
sudo apt install build-essential libglfw3-dev
```

Navigate to the project directory and run:

``` bash
make linux
```

**Important:** This command requires a `linux` target in your Makefile.
The exact build command may need adjustment to match your local GLAD and
GLFW configuration.

## 9. Graphics Concepts Demonstrated

This project demonstrates the following computer graphics concepts:

1.  **Geometric primitives:** Triangles, rectangles, circles, and
    polygons are used to construct scene objects.
2.  **Object positioning:** Objects are placed at specific coordinates
    to form the village environment.
3.  **Layering:** Drawing order is used to display foreground and
    background objects correctly.
4.  **Color and shading:** Different colors represent objects and help
    distinguish day and night scenes.
5.  **Animation:** Moving vehicles, walking people, swings, a seesaw, a
    traffic signal and falling rain create motion in the scene.
6.  **Lighting:** Streetlights and changes in ambient brightness
    contribute to the nighttime appearance.
7.  **Scene transition:** The day and night modes can change smoothly,
    including through the automatic cycle.
8.  **Keyboard interaction:** User input controls scene modes and
    animation.

## 10. Project Objectives

-   To create a 2D village environment using OpenGL.
-   To apply geometric primitives for drawing real-world objects.
-   To demonstrate object positioning and layering.
-   To implement day and night scene transitions.
-   To create moving vehicle, pedestrian, playground and rain
    animations.
-   To simulate simple traffic rules (traffic signal, stop line,
    zebra crossing).
-   To demonstrate basic lighting effects.
-   To use keyboard input for interactive scene control.

## 11. Important Notes

-   `build/SmartVillage.exe` is not included in this version. Rebuild it
    with `mingw32-make win` so that it contains the latest scene.
-   The project requests an **OpenGL 3.3 Core** context. A compatible
    graphics driver is required.
-   GLAD source and header files must be included and compiled as part
    of the project.
-   The required GLFW development library must be available to the
    compiler and linker.
-   On Windows, keep the required GLFW DLL beside the executable when
    running the application.
-   The supplied example ZIP may not include the MinGW GLFW import
    library (`libglfw3.a`). If it is missing, install the MSYS2 GLFW
    package shown in the Windows setup section.
-   Make sure the file paths and Makefile commands match your actual
    project structure.

## 12. Conclusion

The Smart Village project demonstrates how a 2D environment can be
created and animated using C++ and OpenGL. By combining geometric
primitives, object positioning, layering, lighting, keyboard
interaction, and animation, the project presents a village scene with
day and night appearances, moving vehicles, and animated rain, traffic, pedestrians, shops and a playground.
