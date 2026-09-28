# A.L.I.V.E. — Active Dog Leash

**EPFL — ME-410 Mechanical Product Design and Development**

A.L.I.V.E. (**Active Leash for Intelligent Vigilance & Ease**) is an active dog-leash prototype designed to improve safety and comfort during walking and running.

The system combines a motorized winding mechanism with an active braking system to regulate leash length, reduce sudden pulling forces, and keep the leash under controlled tension.

## Main Features

- Motorized leash winding and retraction
- Active braking for sudden pulls
- PID control of the braking mechanism
- Motor position, velocity, and load feedback
- Friction and inertia compensation
- Arduino-based embedded control
- Experimental force and position measurements

## Results

Experimental testing showed:

- ~**75% reduction** in peak force during simulated dog acceleration
- ~**83% reduction** in peak force during sudden stops
- Automatic braking and rewind based on the selected leash length
- Real-time control of leash tension and distance

## Repository Structure

```text
Data Monitoring/       Data acquisition and monitoring scripts
acceleration_model/    Motor acceleration characterization
acceleration_test/     Acceleration experiments
brakescontrol/         Braking-system control
check_connections/     Hardware and communication tests
data/                  Experimental data
load_conversion/       Motor load conversion and calibration
main/                  Main integrated control implementation
torque_model/          Motor torque characterization
Poster.pdf             Project poster
Report.pdf             Full project report
Video.mp4              Prototype demonstration
```

## My Contributions

My work focused mainly on:

- Braking-system PID control
- Brake calibration
- Software implementation
- System integration
- Debugging and testing
- Supporting calculations and hardware development

## Team

- Andrea Tarabay
- Aude-Line Fleury
- Bastien Ney
- Zoé Philbois
- Alfonso Monna
- Vincente Galdini

## Documentation

📄 [Full Project Report](Report.pdf)  
📊 [Project Poster](Poster.pdf)  
🎥 [Demo Video](Video.mp4)
