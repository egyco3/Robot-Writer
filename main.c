#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>

// MODE OPTIONS

#define TERMINAL_MODE 1 // Set to 1 for simulation mode (prints G-code to console), 0 for actual robot mode

#if TERMINAL_MODE == 0

#include "rs232.h"
#include "serial.h"
#define bdrate 115200 /* 115200 baud */

void SendCommands(char *buffer);

#endif

// DEFINITIONS

#define MaxAscii 128
#define MaxWordLength 100
#define LineLength 100
#define LineSpacing 2.0f

// STRUCTS

typedef struct // Struct to hold stroke data
{
    float X, Y;
    int Pen;
} Strokes;

typedef struct // Struct to hold character data
{
    int ascii;
    int StrokeCount;
    Strokes *pStrokes;
} Character;

// GLOBAL VARIABLES

Character *FontArray = NULL; // Array to hold font data
float XOffset = 0.0, YOffset = 0.0, ScaleFactor = 0.0;

// FUNCTION DECLARATIONS

int LoadFontData(void);
float GetFontSize(void);
float CalculateScaleFactor(float FontSize);
float CalculateWordWidth(const char *Word);
int ProcessWord(float FontSize);
void GenerateGCode(const char *Word);
void SetNewLine(float FontSize);
void ResetPen(void);
void FreeFontData(void);

// FUNCTIONS

int main(void)
{
    printf("RobotWriter Program - Callum O'Neill\n\n");

    LoadFontData();

    float FontSize = GetFontSize();               // Assigns FontSize from return value
    ScaleFactor = CalculateScaleFactor(FontSize); // Calculates the scale factor based on the font size

#if TERMINAL_MODE == 0
    // char mode[] = {'8', 'N', '1', 0};

    char buffer[100];

    // If we cannot open the port then give up immediately
    if (CanRS232PortBeOpened() == -1)
    {
        printf("\nUnable to open the COM port (specified in serial.h) ");
        exit(0);
    }

    // Time to wake up the robot
    printf("\nAbout to wake up the robot\n");

    // We do this by sending a new-line
    sprintf(buffer, "\n");
    // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer(&buffer[0]);
    Sleep(100);

    // This is a special case - we wait  until we see a dollar ($)
    WaitForDollar();

    printf("\nThe robot is now ready to draw\n");

    // These commands get the robot into 'ready to draw mode' and need to be sent before any writing commands
    sprintf(buffer, "G1 X0 Y0 F1000\n");
    SendCommands(buffer);
    sprintf(buffer, "M3\n");
    SendCommands(buffer);
    sprintf(buffer, "S0\n");
    SendCommands(buffer);

#endif

    ProcessWord(FontSize); // Processes each word in the test data file

    printf("\nG-Code Sent.\n\n");

    FreeFontData(); // Frees the memory allocated for font data

    printf("Font data memory freed\n\n");

#if TERMINAL_MODE == 0
    CloseRS232Port();
    printf("Com port now closed\n");
#endif

    return 0;
}

int LoadFontData(void)
{
    FILE *pSingleStrokeFont = fopen("SingleStrokeFont.txt", "r"); // Creates a file pointer to the font data file
    if (!pSingleStrokeFont || pSingleStrokeFont == NULL)          // Checks if the file pointer is NULL
    {
        printf("Could not open SingleStrokeFont.txt\n");
        return -1;
    }

    FontArray = calloc(MaxAscii, sizeof(Character)); // Allocates memory for the font array
    if (FontArray == NULL)                           // Checks if memory allocation was successful
    {
        printf("Memory allocation failed for FontArray\n");
        fclose(pSingleStrokeFont);
        return -2;
    }

    int Marker, ascii, StrokeCount;

    while (fscanf(pSingleStrokeFont, "%d", &Marker) == 1) // Reads every number and makes it a marker
    {
        if (Marker == 999) // Checks for the end marker
        {
            fscanf(pSingleStrokeFont, "%d %d", &ascii, &StrokeCount);                  // Reads the ASCII value and stroke count
            FontArray[ascii].ascii = ascii;                                            // Assigns the ASCII value to the character
            FontArray[ascii].StrokeCount = StrokeCount;                                // Assigns the stroke count to the character
            FontArray[ascii].pStrokes = malloc((size_t)StrokeCount * sizeof(Strokes)); // Allocates memory for the strokes

            for (int i = 0; i < StrokeCount; i++) // Loops through each stroke
            {
                fscanf(pSingleStrokeFont, "%f %f %d", // Reads the stroke data
                       &FontArray[ascii].pStrokes[i].X,
                       &FontArray[ascii].pStrokes[i].Y,
                       &FontArray[ascii].pStrokes[i].Pen);
            }
        }
    }

    fclose(pSingleStrokeFont);
    return 0;
}

float GetFontSize(void)
{
    float FontSize;
    while (1) // Infinite loop to ensure valid font size input
    {
        printf("Enter a font size between 4 and 10:\n\n");
        scanf("%f", &FontSize);

        if (FontSize >= 4 && FontSize <= 10)
        {
            printf("\nSelected font size: %f\n\n", FontSize);
            return FontSize;
        }
        else
        {
            printf("\nThis is an invalid font size\n\n");
            getchar(); // Stops infinite loop because of the '\n'
        }
    }
}

float CalculateScaleFactor(float FontSize)
{
    return FontSize / 18.0f;
}

float CalculateWordWidth(const char *Word)
{
    float WordWidth = 0.0;

    for (size_t i = 0; i < strlen(Word); i++) // Loops through each character in the word
    {
        int ascii = (int)Word[i]; // Converts character to ASCII value
        Character CurrentCharacter = FontArray[ascii];
        if (CurrentCharacter.StrokeCount > 0)
        {
            float WordEnd = CurrentCharacter.pStrokes[CurrentCharacter.StrokeCount - 1].X; // Gets the X coordinate of the last stroke
            WordWidth += WordEnd * ScaleFactor;
        }
    }

    return WordWidth;
}

int ProcessWord(float FontSize)
{
    FILE *pTestDataFile = fopen("TestData.txt", "rb"); // Creates a file pointer to the test data file
    if (!pTestDataFile || pTestDataFile == NULL)       // Checks if the file pointer is NULL
    {
        printf("Could not open TestData.txt\n");
        return -1;
    }

    char Word[MaxWordLength];
    int WordIndex = 0;
    int CurrentCharacter;

    while ((CurrentCharacter = fgetc(pTestDataFile)) != EOF)
    {
        if (CurrentCharacter == ' ' || CurrentCharacter == '\t' || CurrentCharacter == '\n' || CurrentCharacter == '\r')
        {
            if (WordIndex > 0)
            {
                Word[WordIndex] = '\0';

                if (XOffset + CalculateWordWidth(Word) > LineLength) // New line check
                {
                    SetNewLine(FontSize);
                }

                GenerateGCode(Word);
                WordIndex = 0;
            }

            if (CurrentCharacter == ' ') // Handle space
            {
                XOffset += FontSize;
            }

            if (CurrentCharacter == '\t') // Handle tab (typically 4 spaces; adjust as needed)
            {
                XOffset += 4 * FontSize;
            }

            if (CurrentCharacter == '\n' || CurrentCharacter == '\r') // Handle new line
            {
                SetNewLine(FontSize);
            }

            continue; // Prevents whitespace being added to the next word
        }

        if (WordIndex < MaxWordLength - 1) // Add character to the word if there's space
        {
            Word[WordIndex++] = (char)CurrentCharacter;
        }
    }

    if (WordIndex > 0) // Process any remaining word after EOF
    {
        Word[WordIndex] = '\0';

        if (XOffset + CalculateWordWidth(Word) > LineLength)
        {
            SetNewLine(FontSize);
        }

        GenerateGCode(Word);
    }

    ResetPen(); // Ensures pen is reset at the end

    fclose(pTestDataFile);

    printf("\nTestData.txt closed\n");
    return 0;
}

void GenerateGCode(const char *Word)
{
    for (size_t i = 0; i < strlen(Word); i++)
    {
        int ascii = (int)Word[i];
        Character CurrentCharacter = FontArray[ascii];

        for (int j = 0; j < CurrentCharacter.StrokeCount; j++)
        {
            float X = XOffset + CurrentCharacter.pStrokes[j].X * ScaleFactor; // Calculates the X coordinate with the scale factor
            float Y = YOffset + CurrentCharacter.pStrokes[j].Y * ScaleFactor; // Calculates the Y coordinate with the scale factor
            int Pen = CurrentCharacter.pStrokes[j].Pen;

#if TERMINAL_MODE == 0
            char WordBuffer[100];
#endif
            if (Pen == 1)
            {
#if TERMINAL_MODE == 1
                {
                    printf("S1000\n"); // Pen down command
                    printf("G0 X%.2f Y%.2f\n", X, Y);
                }
#endif
#if TERMINAL_MODE == 0
                {
                    sprintf(WordBuffer, "S1000\n");
                    SendCommands(WordBuffer);
                    sprintf(WordBuffer, "G0 X%.2f Y%.2f\n", X, Y);
                    SendCommands(WordBuffer);
                }
#endif
            }
            else
            {
#if TERMINAL_MODE == 1
                {
                    printf("S0\n");
                    printf("G0 X%.2f Y%.2f\n", X, Y);
                }
#endif
#if TERMINAL_MODE == 0
                {
                    sprintf(WordBuffer, "S0\n"); // Pen up command
                    SendCommands(WordBuffer);
                    sprintf(WordBuffer, "G0 X%.2f Y%.2f\n", X, Y);
                    SendCommands(WordBuffer);
                }
#endif
            }
        }

        if (CurrentCharacter.StrokeCount > 0)
        {
            XOffset += CurrentCharacter.pStrokes[CurrentCharacter.StrokeCount - 1].X * ScaleFactor; // Updates the XOffset to the end of the current character
        }
    }
}

void SetNewLine(float FontSize)
{
    XOffset = 0.0f;
    YOffset -= (FontSize + LineSpacing); // Moves the YOffset down for the new line
}

void ResetPen(void)
{
#if TERMINAL_MODE == 1
    {
        printf("S0\n");       // Pen up command
        printf("G0 X0 Y0\n"); // Move to origin
    }
#endif
#if TERMINAL_MODE == 0
    {
        char WordBuffer[100];
        sprintf(WordBuffer, "S0\n"); // Pen up command
        SendCommands(WordBuffer);
        sprintf(WordBuffer, "G0 X0 Y0\n"); // Move to origin
        SendCommands(WordBuffer);
    }
#endif
}

void FreeFontData(void)
{
    for (int i = 0; i < MaxAscii; i++)
    {
        free(FontArray[i].pStrokes);
    }
    free(FontArray);
}

#if TERMINAL_MODE == 0
void SendCommands(char *buffer)
{
    // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer(&buffer[0]);
    WaitForReply();
    Sleep(100); // Can omit this when using the writing robot but has minimal effect
    // getch(); // Omit this once basic testing with emulator has taken place
}
#endif