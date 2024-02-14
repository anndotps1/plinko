#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

// Define constants
#define ESCAPEKEY 27
#define TABKEY 9
#define ENTERKEY 10
#define MOVEMENT_FAST 10000
#define MOVEMENT_NORMAL 300000

// Define the board
#define BOARD_PEG '.'
#define BOARD_SPACE ' '
#define BOARD_CUPWALL '|'
#define BOARD_CUPEMPTY '_'
#define BOARD_BALL 'o'

void playPlinko(int numRows);
void drawBoard(int numRows, bool* spaceContainsBall);
void drawCups(const bool* cupContainsBall, int numCups);
void setBoolArrayToFalse(bool* boolArray, int numItems);
void disableRawMode();
void enableRawMode();

struct termios originalTermios;

int main()
{
    srand((unsigned int)time(NULL)); // Seed the rand() function
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK); // Set input to nonblocking
    int numRows = 10;
    enableRawMode();
    playPlinko(numRows);
}

void playPlinko(int numRows)
{ 
    // Allows us to delay drawing of the board by a certain time interval
    clock_t endTimeMs = clock(); 
    char c; // Needed for reading from STDIN
    bool simMode = false;
    int movementDelay = MOVEMENT_NORMAL;

    // The total number of spaces on the board is equal to the sum of every number before boardRows
    int numTotalSpaces = 0; 
    for(int i = 1; i < numRows; numTotalSpaces += i, i++);

    // Arrays to track whether the spaces and cups contain balls
    bool spaceContainsBall[numTotalSpaces];
    setBoolArrayToFalse(spaceContainsBall, numTotalSpaces);
    int numCups = numRows - 1;
    bool cupContainsBall[numCups];
    setBoolArrayToFalse(cupContainsBall, numCups);

    // Variables used to modify space and cup state after the board has been drawn
    bool newSpacecontainsBall[numTotalSpaces];
    setBoolArrayToFalse(newSpacecontainsBall, numTotalSpaces);
    int currentSpace = 0;
    int newSpace = 0;
    int lastRowStart = numTotalSpaces - numRows + 1;

    // Main game loop
    for(;;)
    {
        // Check if the user has pressed a key and put that key in c
        if(read(STDIN_FILENO, &c, 1) == 1) 
        {
            if(c == ESCAPEKEY)
            {
                exit(EXIT_SUCCESS);
            }
            else if(c == TABKEY)
            {
                simMode = simMode ? false : true;
                movementDelay = simMode ? MOVEMENT_FAST : MOVEMENT_NORMAL;
            }
            else if(c == ENTERKEY)
            {
                setBoolArrayToFalse(cupContainsBall, numCups); // Reset cups
            }
            else if(simMode == false)
            {
                spaceContainsBall[0] = true; // Add a new ball to the board 
            }
        }
        if(clock() >= endTimeMs)
        {
            // Automatically drop balls in sim mode
            if(simMode)
            {
                spaceContainsBall[0] = true;
            }

            // Draw the board
            system("clear");
            drawBoard(numRows, spaceContainsBall);
            drawCups(cupContainsBall, numCups);

            // Move the balls by recalculating their position
            setBoolArrayToFalse(newSpacecontainsBall, numTotalSpaces);
            currentSpace = 0;
            newSpace = 0;
            for(int i = 1; i <= numRows; i++)
            {   
                for(int j = 1; j < i; j++) // Track spaces per row
                {
                    if(i < numRows) // If we are not on the last row, update the spaces array
                    {
                        if(spaceContainsBall[currentSpace])
                        {   
                            // We can cause the ball to move right by adding the number of spaces + 1
                            // Which is also equal to the row number (i)
                            newSpace = currentSpace + i; 
                            if(rand() % 2)
                            {
                                // We can cause the ball to move left by the number of spaces
                                // Since we added the number of spaces + 1 above, we can just subtract one
                                newSpace--;
                            }
                            newSpacecontainsBall[newSpace] = true;
                        }
                    }
                    else // Otherwise, update the cups instead
                    {
                        if(spaceContainsBall[currentSpace]) 
                        {
                            // The position of the ball in the cups array is equal to its position on the last row
                            cupContainsBall[currentSpace - lastRowStart] = true;
                        }
                    }
                    currentSpace++;
                }
            }
            // Overwrite the current array with the one containing the newly calculated positions
            memcpy(spaceContainsBall, newSpacecontainsBall, sizeof(newSpacecontainsBall));

            endTimeMs = clock() + movementDelay; 
        }
    }
}

void drawBoard(int numRows, bool* spaceContainsBall)
{
    int currentSpace = 0;
    // Draw the board
    printf("Press any key to drop a ball or Escape to quit.\n");
    printf("Debugging commands: Tab to toggle simulation mode, enter to clear cups.\n");
    for(int i = 1; i <= numRows; i++) // On each row
    {
        for(int j = 1; j <= (numRows - i); j++) // Draw leading spaces
        {
            putchar(BOARD_SPACE);
        }
        for(int j = 1; j < i; j++) // Track the number of spaces in between pegs on the current row
        {
            putchar(BOARD_PEG);
            if(spaceContainsBall[currentSpace])
            {   
                putchar(BOARD_BALL);
            }       
            else
            {
                putchar(BOARD_SPACE);
            }
            currentSpace++;
        }
        printf("%c\n",BOARD_PEG); // Move to the next row
    }
}

void drawCups(const bool* cupContainsBall, int numCups)
{ 
    // Draw the cups
    for(int i = 0; i < numCups; i++)
    {
        printf("%c%c", BOARD_CUPWALL, (cupContainsBall[i]) ? BOARD_BALL : BOARD_CUPEMPTY);
    }
    printf("%c\n", BOARD_CUPWALL);
}

void setBoolArrayToFalse(bool* boolArray, int numItems)
{
    for(int i = 0; i < numItems; i++)
    {
        boolArray[i] = false;
    }
}

void disableRawMode() 
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
}

void enableRawMode() 
{
    tcgetattr(STDIN_FILENO, &originalTermios); 
    atexit(disableRawMode);
    struct termios raw = originalTermios;
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
