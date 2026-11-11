# Smart Village: Day and Night Scenario
## Computer Graphics Project Report

### 1. Project description
This project presents a smart village environment designed using computer graphics. The scene contains village houses, a shop, a super shop, a school, a playground, roads with footpaths and a zebra crossing, a traffic signal, walking people, trees, a pond, mountains, vehicles, street lights and weather effects. Day and night modes change the visual appearance of the environment. Rain and vehicle movement add animation.

### 2. Objectives
- Create a village scene from 2D geometric objects.
- Demonstrate object positioning and layering.
- Show different day and night appearances.
- Animate rain, moving vehicles, walking people and playground equipment.
- Simulate simple traffic control with a traffic signal and a zebra crossing.
- Demonstrate smooth scene transitions and lighting effects.

### 3. Tools and technologies
- Programming language: C++
- Graphics API: OpenGL 3.3 Core
- Window and input library: GLFW
- OpenGL function loader: GLAD
- Shader language: GLSL

### 4. Main modules
1. **Window and OpenGL initialization:** creates the rendering window and loads OpenGL functions.
2. **Primitive drawing:** uses triangles, rectangles, circles, lines and polygons to construct objects.
3. **Environment:** draws sky, mountains, fields, river and road.
4. **Buildings:** draws houses, a school, a small shop and a super shop with roofs, doors, windows, signs and awnings.
5. **Footpaths and traffic:** draws footpaths, a zebra crossing and stop lines; a timer-driven traffic signal (green, yellow, red) controls cars and a school bus that slow down, queue and restart.
6. **People:** draws walking people, pedestrians who cross only while the traffic is stopped, and children in the playground; umbrellas appear during rain.
7. **Playground:** animated swings, seesaw and slide.
8. **Lighting and time modes:** changes ambient brightness and sky color between day and night; street lamps become visible at night.
9. **Animation:** moves vehicles and people, swings the playground equipment and animates rain when enabled.
10. **Input handling:** keyboard controls switch modes and animation states.

### 5. Requirement checklist (from the teacher)
| Requirement | Implementation |
|---|---|
| Shop | Small village shop with sign, awning, goods and shopkeeper |
| Traffic | Traffic lights; vehicles stop on red and queue |
| Human | Walking people, zebra-crossing pedestrians, children |
| School | Sunrise School building with flag |
| Super shop | Super Shop building with lit sign and display windows |
| Footpath | Paved footpaths on both sides of the road |
| Rain | Animated rain, puddle ripples, splashes, umbrellas |
| Playground | Swings, seesaw and slide |

### 6. Controls
| Key | Function |
|---|---|
| D | Day mode |
| N | Night mode |
| A | Automatic day/night cycle |
| R | Toggle rain |
| S | Stop rain |
| Space | Pause/resume |
| ESC | Exit |

### 7. Expected output
A window displaying a 2D village landscape. The user can switch between day and night, enable rain, pause the animation and watch cars obey the traffic signal while people walk, cross the road and play.

### 8. Conclusion
The project demonstrates how OpenGL can be used to build a layered 2D environment from geometric primitives. It combines static village objects with animation, keyboard interaction, day/night scene changes and lighting effects.

### 9. Group members
1. Ashraful Haque Zani — ID: 0432410005101075 — Section: 6B1
2. Md. Nasim Talukder — ID: 0432410005101084 — Section: 6B1
3. Asif Bin Saif — ID: 0432410005101092 — Section: 6B1
4. Jannatul Ferdous Nadia — ID: 0432410005101094 — Section: 6B1

*This report follows the supplied proposal. Add screenshots of the running program, instructor-specific formatting, and any additional implementation details required by your course before submission.*
