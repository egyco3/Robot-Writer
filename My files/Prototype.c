#include <stdlib.h>
#include <stdio.h>

// FONT DATA STRUCTS //

typedef struct // Creates a struct to hold stroke information
{
    float x, y, pen; // Creates a float value to hold the x, y and pen data for each position for a character
} Strokes;           // Creates an alias to allow the struct to be called without typing "struct" each time

typedef struct // Creates a struct to hold character information
{
    int ascii;         // Holds the ascii value of each character
    int StrokeCount;   // Holds the stroke count of each letter
    Strokes *pStrokes; // Points to the 'Strokes' struct to allow data to be put there
} Character;

// FUNCTION DECLARATIONS //

int LoadFontData(void);
int GetFontSize(void);
float CalculateScaleFactor(int FontSize);
int ReadWord(void);
int NewLineChecker(const char *Word);
void SetNewLine(void);
void GenerateGCode(const char *Word);
int NewWordChecker(FILE *pFontDataFile);

int main(void)
{
    printf("RobotWriter Program - Callum O'Neill 20576144\n\n"); // Prints the project title

    LoadFontData(); // Calls the 'LoadFontData' function

    int FontSize = GetFontSize(); // Stores the return value 'FontSize' as an int with the same name

    float ScaleFactor = CalculateScaleFactor(FontSize);
}

int LoadFontData(void)
{
    FILE *pFontDataFile = fopen("SingleStrokeFont.txt", "r"); // Creates a file pointer to point to the font data file

    if (pFontDataFile == NULL) // Checks if the pointer doesn't have an address of the file
    {
        printf("The file could not be opened\n\n"); // Prints an error message
        return -1;                                  // Returns error value
    }

    Character *pCharacter;                       // Creaates a pointer to the struct 'Letter'
    pCharacter = calloc(128, sizeof(Character)); // Allocates the space needed for all

    int Marker, ascii, StrokeCount; // Creates an int for the 3 variables

    while (fscanf(pFontDataFile, "%d", &Marker) == 1) // Scans every word in the font data file
    {
        if (Marker == 999) // Checks for the new word marker '999'
        {
            fscanf(pFontDataFile, " %d %d", &ascii, &StrokeCount);              // Loads the ascii and stroke count data
            pCharacter[ascii].ascii = ascii;                                    // Stores the ascii value for the specific letter
            pCharacter[ascii].StrokeCount = StrokeCount;                        // Stores the stroke counr for the specific letter
            pCharacter[ascii].pStrokes = malloc(sizeof(Strokes) * StrokeCount); // Allocates enough memory needed for each letter depending of it's amount of strokes

            for (int i = 0; i < StrokeCount; i++) // Iterates through all strokes for each character
            {
                fscanf(pFontDataFile, " %f %f %f",          // Scans the x, y, and pen data
                       &pCharacter[ascii].pStrokes[i].x,    // Stores the x data for the specific stroke
                       &pCharacter[ascii].pStrokes[i].y,    // Stores the y data for the specific stroke
                       &pCharacter[ascii].pStrokes[i].pen); // Stores the pen data for the specific stroke
            }
        }
    }

    fclose(pFontDataFile); // Closes the file
    return 0;              // Returns success value
}

int GetFontSize(void)
{
    int FontSize;
    while (1) // Repeats until a value is returned
    {
        // Creates an int to store the font size
        printf("Enter an integer font size between 4 and 10:\n\n"); // Prompts the user for a font size
        scanf("%d", &FontSize);                                     // Stores the users input as 'FontSize'

        if (FontSize >= 4 && FontSize <= 10) // Checks if 'FontSize' is in the given integer range
        {
            printf("Selected font size: %d", FontSize); // Prints the font size
            return FontSize;                            // Returns 'FontSize', breaking the while loop
        }
        else // If 'FontSize' is not a valid integer
        {
            printf("This is an invalid font size\n\n"); // Prints error statement
            // No return value so the while loop repeats
        }
    }
}

float CalculateScaleFactor(int FontSize)
{
    float ScaleFactor = (float)FontSize / 18.0;  // Converts 'FontSize' to a float and calculates the scale factor
    printf("Scale factor: %f\n\n", ScaleFactor); // Prints scale factor
    return ScaleFactor;                          // Returns 'ScaleFactor'
}

// int ReadWord(void)
// {
// }

// int NewLineChecker(const char *word)
// {
// }

// void SetNewLine(void)
// {
// }

// void GenerateGCode(const char *word)
// {
// }