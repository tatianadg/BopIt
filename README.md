# BopIt
## EC 450 Final Project in C


### Goal: 
The goal of our final microprocessors project was to develop and implement a similar game to “Bop-It!”, which is a simple & interactive game for young children. Our game specifically is centered more on the basic actions that “Bop-It!” has, given the peripherals we have with the EDU-MKII Booster Pack. We thought this project was a great way to display what we have learned this year in a fun, engaging game that we could demonstrate all of our friends and be proud of.

### Requirements:
A way to display the commands on the EDU-MKII Booster Pack’s LCD screen.
A way to randomly choose between the actions that will be delivered to the user via the EDU-MKII Booster Pack’s LCD screen).
A way to make sure the MSP432P401R microcontroller knows when the user shakes it, as it is one of the commands (‘Shake’).
A way to make the speed of the game increase when the user correctly performs 5 actions.
A way to store the highest level achieved by any user onto Flash memory and display at the end of the game.

### Design of the Project: 
To fulfill the requirements specified above, we designed our game for the MSP432P401R microcontroller and we used the EDU-MKII Booster Pack to display the commands, to display the level achieved and to use its peripherals for the actions we incorporated into our game. Specifically we used the EDU-MKII Booster Pack’s LCD screen to display which command to execute and, at the end of the game it also displayed to the user how far it got in the game (aka the level achieved and the score they got).  To handle the inputs (the execution of the actions by the user),  we used the EDU-MKII Booster Pack’s joystick, buttons and accelerometer.

The overall design of the project was to have a game, “Bop-It!”, that tested to see if the user was able to perform accurate actions, even when the speed was turned up, aka when the game got increasingly harder. The basic design was to have the game turn on and when the user was ready and pressed the start button, the game went into game mode, where it would then randomly choose which action to make the user perform. It would then give the user a fixed amount of time to perform said action (which could be seen visually with the dots filling up at the top of the screen) and if they did not perform the action it in the specified amount of time or if they performed the wrong action, it would end the game and display the level and score they achieved. Every time the user performed 5 correct actions, the level of the game would increase by one and in turn. it will then increase the speed by a fixed time so that the next time the user was asked to perform a random action, they would have to react faster than before.


### Rules of the game: 
The user needs to perform the action displayed on the EDU-MKII Booster Pack’s LCD Screen as quickly as possible.
Every 5 correct actions performed (in a timely manner), the level will increase (aka the speed up the game will increase).
Once the user performs the wrong action or if they do not do it in the specified amount of time, it will end the game and display what level the user has reached and the score achieved.


### Button Key:
Button 1 - Start the game

### Actions:
Up: 	    Move the Joystick up
Down:    Move the Joystick Down
Right:     Move the Joystick Right
Left:	   Move the Joystick Left
Bop-it:    Press the Joystick
Shake:   Shake the BoosterPack
Button:   Press the button 2


### Implementation: 
*States of the game*: The game contains four states that's implemented in the WatchDog Timer. 
First state: This is the beginning state of the game, it is where the game resets everything except the high score and displays the start screen. If button one is pressed, then the game starts.

*Second State*: This is the state where the game resets the counter, the ADC values and the button flags. The difficulty and the level of the player is determined here too. It increments the level everytime the user achieves 5 points and then sets the difficulty according to that. It picks a random instruction to proceed with and then changes the state to the third stage.

*Third Stage*: This state checks if the correct action was taken, and if it is correct, it then goes back to the second stage and increments the score by one. If the action was wrong,  it proceeds to the fourth stage.
Fourth Stage: This is the end stage. Here it shows the score and the level achieved by the user and if they got the new high score it displays the new high score, and if they didn’t get the new high score, it shows the highest one achieved on the game by any user. This state then changes states back to the first state when the button is pressed.

*Joystick*: 
The joystick was implemented by using an ADC (Analog to Digital Converter). Used in the game. When a direction (up, down, left, right) is given to the ADC we used the ADC converter value to check if that action is taken.

### Buttons:
*Button1*: Changes states (When pressed the game starts.)

*Button2 and JoyStick Button*: Used as a few of the different actions the user could perform during the game, and it is checked when the appropriate action was called.

### Display:
*First State*: Displays the name of the game and gives instructions to start the game.

*Second/Third State*: Displays the instructions, the score and the level. Also has a loading display at the top of the EDU-MKII Booster Pack’s LCD Screen to show the time left to do the instruction.

*Fourth State*: Shows the all time high score, and the score and level of the game that just finished.

*Accelerometer*:
The z-value is converted by the ADC and and compared to a reference value to see if the user does the “shake it” instruction.

*Flash Memory*:
Here we saved a high score variable into Flash Memory and it only was changed when the user was able to achieve a higher score than what was saved into Flash Memory.

*WatchDog Timer*:
The WatchDog Timer was the general timer we decided to use for the game. It also contains the game logic.



 
### Assessment on the success of the project: 
After looking at the requirements we had decided that our game should have, and the fact that we got all of them to work properly, we felt that our final project was a success! While we did encounter a few challenges along the way (ie. implementing Flash Memory and getting the Accelerometer to work properly) we were able to figure it out and have our final product work just as we imagined.


### Next Steps: 
In every project you are apart of, there is always room for improvement, and for this specific project, there were lots of ideas that we would like to implement if we had more time to dedicate to it. 
First, instead of displaying the commands to be performed by the user on the EDU-MKII Booster Pack’s LCD screen, we would have liked to make the directions that the user needs to perform to be delivered through a voice command. To us, we thought this would make the game more similar to the actual “Bop-It!” Because we thought it would make the game more intense. Because when playing any game, it really is all about the user’s experience and we thought this way would be a great way to enhance the user’s experience. 
Next, we would like to enhance the color indicators that displays to the user if they got the command right or wrong. We thought by adding in some fun noises that indicated when they got the command right or wrong would again, this would make the game more engaging for the user.. 
Another fun little extra extension we would have liked to have an option to enter in their name to display on the Leaderboard, that will be saved onto Flash memory, so that everytime the game ends it would display the top 3 leaders of the game. 
Also, once we get the commands to be delivered through a voice command, we would have an option where at the beginning of the game the user will be able to choose which game mode they would like to play, the voice command one or the EDU-MKII Booster Pack’s LCD Display one. 


 
### Summary of team members' contributions to the project: 
We both contributed an even amount of work into this project, which we achieved by working on it together (aka in person). By working together, we were able to put our previous knowledge together, so when either one of us got stuck, the other partner was there to help them out. We figured out the logic and overall implementation of the project together and split the amount of code we needed to finish evenly, to make the most out of our time. We also split up the Final Report, since we each were a little more knowledgeable about different parts of the project. Group projects are always a great way to learn from each other and having a partner is great because they are there to keep each other focused and motivated. 
