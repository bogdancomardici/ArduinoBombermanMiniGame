# ArduinoBombermanMiniGame

## Description

<p align = center>
  <img src="ArduinoBombermanMiniGame/bomberman.jpeg" width = 50%>
</p>

The classic Bomberman game is emulated on an 8x8 matrix, featuring an LCD Display that displays various game information,
including player status, welcome and end game messages, and the number of lives remaining.
The game comprises three elements: players (blinking slowly), bombs (blinking rapidly), and walls (non-blinking).
A randomly generated map, with destructible walls (unbreakable borders), is created each time the game starts.
Players navigate using a joystick, placing bombs that destroy walls in a plus pattern. The objective is to demolish
as many walls as possible without succumbing to the blast. The game is timed, and the score is determined by
the number of walls destroyed. Players have a set number of lives, which can be lost if caught in a bomb's blast
radius during detonation. The logical game matrix is 16x16, but only an 8x8 portion is displayed, utilizing the
fog of war concept. At the end of the game, the final score is showcased, and players are prompted if they have beaten
the highscore. Each player can choose a three-character name, saved in the leaderboard along with their score.
Highscores can be reset, setting all scores to 0 and player names to XXX. The game includes informative sections about
the game and its creator, as well as a guide on how to play. Players can select from three difficulty levels (low, medium, high),
affecting gameplay elements such as the number of lives, bomb explosion speed, map density, and points awarded for each destroyed wall.
The game also features a settings menu, allowing players to adjust the LCD and matrix brightness, toggle sound on/off, and change their name.
Sounds are played when navigating the menu, placing bombs, and detonating them. The game is controlled using a joystick, and the
LCD contrast can be adjusted using a potentiometer. Various icons are displayed on the LCD, including a heart for lives, arrows for navigation,
and various images on the led matrix showcasing the menu option selected.

## Backstory

The decision to undertake the development of this game was rooted in its unique combination of simplicity, intuitiveness, and captivating gameplay. While I may not have had the opportunity to play it on the original NES console, my enjoyment of the game flourished through online emulators. The charm of this particular game lies in its ability to showcase a myriad of cool concepts, adding an extra layer of complexity and interest. Notably, the implementation of a "fog of war" mechanism adds an intriguing dimension, creating a dynamic and unpredictable environment for players to navigate. Additionally, the incorporation of dynamic object rendering enhances the overall gaming experience, providing a visual richness that engages players on a deeper level. Through this project, I aim to capture and extend the essence of what made the game enjoyable for me and share it with others who appreciate the blend of simplicity, innovation, and nostalgic charm that defines classic gaming experiences.

## Menu Structure
1. Start game
2. Settings
   - LCD brightness control
   - Matrix brightness control
   - Sounds - ON / OFF
   - Player Name
3. About
4. Highscores
5. Reset Highscores
6. How to play
7. Difficulty

## Hardware
      Input:
        1 x Joystick - used for player movement - pins A0, A1, 13
      
      Output:
        1 x MAX7219 Led matrix driver - pins 10, 11, 12
        1 x 8x8 Led matrix controlled by the MAX7219 driver
        1 x 10uF capacitor to reduce power spikes when the matrix is fully on
        1 x 0.1uF capacitor to filter the noise on 5V
        1 x 10K resitor for the ISET pin on the matrix driver
        1 x LCD Display for displaying various info - pins 3, 4, 5, 6, 7, 8, 9
        1 x 50K Resistors to adjust LCD Contrast
        1 X Buzzer for sound - pin 2
## General Requirements:
A game on a 16x16 logical matrix that implements either multiple rooms, visibility / fog of war and/or multiple
physical matrix.
It should be intuitive and straight down obvious how to use it.
You should have a feeling of progression in difficulty. Depending
on the dynamic of the game, this is done in the same level or with
multiple levels. You can make them progress dynamically or have
a number of fixed levels with an endgame.

## Menu Requirements:

- Intro Message:
Display a greeting message when powering up the game.
- Menu Categories:
  - Start Game
  - Highscore: Initially 0; update after each game. Save top 3+ values in EEPROM with name and score.
  - Settings
  - Enter name (shown in highscore).
  - LCD brightness control (save to EEPROM).
  - Matrix brightness control (save to EEPROM).
  - Sounds on/off (save to EEPROM).
  - About: Display details about the game creator(s), including game name, author, and GitHub link.
  - How to play
- Provide a short and informative description.
  - While Playing: Display relevant in-game information: Lives, Level, Score, Time, Player name, etc.
  - Upon Game Ending:
    - Screen 1: Display a congratulatory message with level/score achievements, switchable to Screen 2 upon interaction.
    - Screen 2: Display relevant game info, inform if the high score is beaten.
## Game Requirements:

- Components: Use a minimal set of components: LCD, joystick, buzzer, and LED matrix.
- Sounds: Implement basic sounds for interactions (eating food, dying, finishing the level, etc.).
- Game Levels:
Work on a 16x16 matrix.
Apply visibility/fog of war or room concepts.
## Gameplay:
- Ensure the game is intuitive, fun, and makes sense in the given setup.
- Progress difficulty gradually.
- Include a feeling of progression in difficulty, introducing some randomness.
