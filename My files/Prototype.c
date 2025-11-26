#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MaxAscii 128
#define MaxWordLength 100
#define LineLength 100
#define LineSpacing 5.0f

typedef struct
{
    float X, Y;
    int Pen;
} Strokes;

typedef struct
{
    int ascii;
    int StrokeCount;
    Strokes *pStrokes;
} Character;

Character *FontArray = NULL;
float XOffset = 0.0, YOffset = 0.0, ScaleFactor = 0.0;

int LoadFontData(void);
float GetFontSize(void);
float CalculateScaleFactor(float FontSize);
float GetWordWidth(const char *Word);
void ReadWord(FILE *pTestDataFile, float FontSize);
void GenerateGCode(const char *Word);
void SetNewLine(float FontSize);
void FreeFontData(void);

int main(void)
{
    printf("RobotWriter Program - Callum O'Neill\n\n");

    if (LoadFontData() != 0)
    {
        return -1;
    }
    float FontSize = GetFontSize();
    ScaleFactor = CalculateScaleFactor(FontSize);

    FILE *pTestDataFile = fopen("TestData.txt", "r");
    if (!pTestDataFile)
    {
        printf("Could not open TestData.txt\n");
        return -1;
    }

    ReadWord(pTestDataFile, FontSize);

    fclose(pTestDataFile);

    FreeFontData();

    return 0;
}

int LoadFontData(void)
{
    FILE *pSingleStrokeFont = fopen("SingleStrokeFont.txt", "r");
    if (!pSingleStrokeFont)
    {
        printf("Could not open SingleStrokeFont.txt\n");
        return -1;
    }

    FontArray = calloc(MaxAscii, sizeof(Character));
    int Marker, ascii, StrokeCount;

    while (fscanf(pSingleStrokeFont, "%d", &Marker) == 1)
    {
        if (Marker == 999)
        {
            fscanf(pSingleStrokeFont, "%d %d", &ascii, &StrokeCount);
            FontArray[ascii].ascii = ascii;
            FontArray[ascii].StrokeCount = StrokeCount;
            FontArray[ascii].pStrokes = malloc((size_t)StrokeCount * sizeof(Strokes));

            for (int i = 0; i < StrokeCount; i++)
            {
                fscanf(pSingleStrokeFont, "%f %f %d",
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
    while (1)
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

float GetWordWidth(const char *Word)
{
    float WordWidth = 0.0;

    for (size_t i = 0; i < strlen(Word); i++)
    {
        int ascii = (int)Word[i];
        Character CurrentCharacter = FontArray[ascii];
        if (CurrentCharacter.StrokeCount > 0)
        {
            float WordEnd = CurrentCharacter.pStrokes[CurrentCharacter.StrokeCount - 1].X;
            WordWidth += WordEnd * ScaleFactor;
        }
    }

    return WordWidth;
}

void ReadWord(FILE *pTestDataFile, float FontSize)
{
    char Word[MaxWordLength];
    int WordIndex = 0;
    int CurrentCharacter;

    while ((CurrentCharacter = fgetc(pTestDataFile)) != EOF)
    {
        if (CurrentCharacter == ' ' || CurrentCharacter == '\t' || CurrentCharacter == 10 || CurrentCharacter == 13)
        {
            if (WordIndex > 0)
            {
                Word[WordIndex] = '\0';

                if (XOffset + GetWordWidth(Word) > LineLength)
                {
                    SetNewLine(FontSize);
                }
                GenerateGCode(Word);

                WordIndex = 0;
            }

            if (CurrentCharacter == ' ')
            {
                XOffset += FontSize; // Edit to space ascii
            }

            if (CurrentCharacter == 10 || CurrentCharacter == 13)
                SetNewLine(FontSize);

            continue;
        }

        if (WordIndex < MaxWordLength - 1)
        {
            Word[WordIndex++] = (char)CurrentCharacter;
        }
    }

    if (WordIndex > 0) // Final Word in input file
    {
        Word[WordIndex] = '\0';
        if (XOffset + GetWordWidth(Word) > LineLength)
        {
            SetNewLine(FontSize);
            GenerateGCode(Word);
        }
    }
}

void GenerateGCode(const char *Word)
{
    for (size_t i = 0; i < strlen(Word); i++)
    {
        int ascii = (int)Word[i];
        Character CurrentCharacter = FontArray[ascii];

        for (int j = 0; j < CurrentCharacter.StrokeCount; j++)
        {
            float X = XOffset + CurrentCharacter.pStrokes[j].X * ScaleFactor;
            float Y = YOffset + CurrentCharacter.pStrokes[j].Y * ScaleFactor;
            int Pen = CurrentCharacter.pStrokes[j].Pen;

            if (Pen == 1)
            {
                printf("G1 X%.2f Y%.2f\n", X, Y);
            }
            else
            {
                printf("G0 X%.2f Y%.2f\n", X, Y);
            }
        }

        if (CurrentCharacter.StrokeCount > 0)
        {
            XOffset += CurrentCharacter.pStrokes[CurrentCharacter.StrokeCount - 1].X * ScaleFactor;
        }
    }
}

void SetNewLine(float FontSize)
{
    XOffset = 0.0f;
    YOffset -= (FontSize + LineSpacing);
}

void FreeFontData(void)
{
    for (int i = 0; i < MaxAscii; i++)
    {
        free(FontArray[i].pStrokes);
    }
    free(FontArray);
}
