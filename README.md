# Battleship Game (C)

## Overview

This project is a console-based implementation of the classic **Battleship strategy game** developed in C.
It was created as part of a programming coursework project to practice structured programming, game logic, and basic client–server communication.

The game supports both **single-player** and **two-player multiplayer modes** using separate terminals.

## Game Modes

### Single Player

* One player plays against the computer.
* The system handles ship placement and move validation.

### Two Player (Client–Server Mode)

* Two players run the game on separate terminals.
* One terminal acts as the **server**.
* The other terminal acts as the **client**.
* Moves are sent and received between the two players.

## Features

* Console-based user interface
* Ship placement and coordinate tracking
* Hit, miss, and sunk detection
* Turn-based gameplay logic
* Multiplayer communication using client–server model
* Modular program structure across multiple project stages

## Project Structure

This repository contains multiple stages of the Battleship project:

* Battleship1 – Basic single-player version
* Battleship2 – Improved game logic
* Battleship3 – Multiplayer setup
* Battleship4 – Client–server gameplay

## Technologies Used

* C Programming Language
* Linux Terminal
* Structured programming concepts

## How to Compile and Run

In a Linux terminal:

1. Compile the program:

```
gcc BattleshipX.c -o battleship
```

2. Run the program:

```
./battleship
```

For multiplayer mode:

* Open two terminals.
* Run the server in one terminal.
* Run the client in the second terminal.

## Learning Outcomes

* Implemented game logic using C
* Practiced modular and structured programming
* Learned basic client–server communication
* Worked with terminal-based applications
