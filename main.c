#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_STUDENTS 200

typedef struct {
    char roll[20];
    char name[60];
    char course[40];
    float marks;
} Student;

Student students[MAX_STUDENTS];
int studentCount = 0;
int selectedIndex = -1;
int scrollOffset = 0;

typedef enum {
    LIST,
    FORM
} Screen;

Screen currentScreen = LIST;

int editMode = 0;
int focusedField = -1;

char formRoll[20] = "";
char formName[60] = "";
char formCourse[40] = "";
char formMarks[20] = "";
char searchText[60] = "";
char statusMessage[100] = "Welcome! Manage your student records.";
char formErrorMessage[100] = "";

// Modern Dark Slate Palette
Color CUSTOM_BG = {15, 23, 42, 255};
Color CUSTOM_PANEL = {30, 41, 59, 255};
Color CUSTOM_BLUE = {59, 130, 246, 255};
Color CUSTOM_WHITE = {248, 250, 252, 255};
Color CUSTOM_MUTED = {148, 163, 184, 255};
Color CUSTOM_GREEN = {16, 185, 129, 255};
Color CUSTOM_RED = {239, 68, 68, 255};
Color CUSTOM_HOVER = {37, 99, 235, 255};
Color CUSTOM_ROW_ALT = {35, 48, 68, 255};

// Load data from file
void LoadData(void)
{
    FILE *file = fopen("students.txt", "r");
    if (file == NULL) return;

    while (studentCount < MAX_STUDENTS &&
        fscanf(file, " %19[^|]|%59[^|]|%39[^|]|%f",
        students[studentCount].roll,
        students[studentCount].name,
        students[studentCount].course,
        &students[studentCount].marks) == 4)
    {
        studentCount++;
    }
    fclose(file);
}

// Save data to file
void SaveData(void)
{
    FILE *file = fopen("students.txt", "w");
    if (file == NULL)
    {
        strcpy(statusMessage, "Error saving records!");
        return;
    }

    for (int i = 0; i < studentCount; i++)
    {
        fprintf(file, "%s|%s|%s|%.2f\n",
            students[i].roll,
            students[i].name,
            students[i].course,
            students[i].marks);
    }
    fclose(file);
}

// Helper to draw clean buttons
int DrawButton(Rectangle rect, const char *text, Color color)
{
    Vector2 mouse = GetMousePosition();
    int hover = CheckCollisionPointRec(mouse, rect);

    DrawRectangleRounded(rect, 0.2f, 8, hover ? CUSTOM_HOVER : color);

    int fontSize = 16;
    int textWidth = MeasureText(text, fontSize);
    DrawText(text,
        rect.x + (rect.width - textWidth) / 2,
        rect.y + (rect.height - fontSize) / 2,
        fontSize, CUSTOM_WHITE);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Helper to draw input fields
void DrawInput(Rectangle rect, const char *label, const char *value, int field)
{
    DrawText(label, rect.x, rect.y - 22, 15, CUSTOM_MUTED);
    DrawRectangleRounded(rect, 0.15f, 8, CUSTOM_PANEL);
    DrawRectangleLinesEx(rect, 2, focusedField == field ? CUSTOM_BLUE : (Color){71, 85, 105, 255});

    if (strlen(value) > 0) {
        DrawText(value, rect.x + 14, rect.y + 12, 17, CUSTOM_WHITE);
    } else {
        DrawText("Type here...", rect.x + 14, rect.y + 12, 17, (Color){100, 116, 139, 255});
    }
}

// Handle text typing
void HandleInput(char *buffer, int maxLength, int numberOnly)
{
    int key = GetCharPressed();
    while (key > 0)
    {
        int valid = 0;
        if (numberOnly)
        {
            valid = (key >= '0' && key <= '9') ||
                    (key == '.' && strchr(buffer, '.') == NULL);
        }
        else
        {
            valid = key >= 32 && key <= 125 && key != '|';
        }

        int len = strlen(buffer);
        if (valid && len < maxLength - 1)
        {
            buffer[len] = (char)key;
            buffer[len + 1] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        int len = strlen(buffer);
        if (len > 0) buffer[len - 1] = '\0';
    }
}

// Form logic
void OpenAddForm(void)
{
    strcpy(formRoll, "");
    strcpy(formName, "");
    strcpy(formCourse, "");
    strcpy(formMarks, "");
    strcpy(formErrorMessage, "");

    selectedIndex = -1;
    editMode = 0;
    focusedField = 0;
    currentScreen = FORM;
}

void OpenEditForm(void)
{
    if (selectedIndex < 0) return;

    Student s = students[selectedIndex];
    strcpy(formRoll, s.roll);
    strcpy(formName, s.name);
    strcpy(formCourse, s.course);
    strcpy(formErrorMessage, "");

    snprintf(formMarks, sizeof(formMarks), "%.2f", s.marks);

    editMode = 1;
    focusedField = 0;
    currentScreen = FORM;
}

void SaveForm(void)
{
    strcpy(formErrorMessage, "");

    if (strlen(formRoll) == 0 || strlen(formName) == 0 ||
        strlen(formCourse) == 0 || strlen(formMarks) == 0)
    {
        strcpy(formErrorMessage, "Please fill all fields!");
        return;
    }

    float marks = strtof(formMarks, NULL);
    if (marks < 0 || marks > 100)
    {
        strcpy(formErrorMessage, "Marks must be between 0 and 100!");
        return;
    }

    for (int i = 0; i < studentCount; i++)
    {
        if (strcmp(students[i].roll, formRoll) == 0)
        {
            if (!editMode || i != selectedIndex)
            {
                strcpy(formErrorMessage, "Student with this Roll No already exists!");
                return;
            }
        }
    }

    if (!editMode && studentCount >= MAX_STUDENTS)
    {
        strcpy(formErrorMessage, "Student limit reached!");
        return;
    }

    int index = editMode ? selectedIndex : studentCount;
    if (index < 0 || index >= MAX_STUDENTS) return;

    strcpy(students[index].roll, formRoll);
    strcpy(students[index].name, formName);
    strcpy(students[index].course, formCourse);
    students[index].marks = marks;

    if (!editMode) studentCount++;

    SaveData();
    selectedIndex = index;
    currentScreen = LIST;
    focusedField = -1;

    strcpy(statusMessage, editMode ? "Student updated successfully!" : "Student added successfully!");
}

void DeleteStudent(void)
{
    if (selectedIndex < 0) return;

    for (int i = selectedIndex; i < studentCount - 1; i++)
    {
        students[i] = students[i + 1];
    }

    studentCount--;
    selectedIndex = -1;
    SaveData();
    strcpy(statusMessage, "Student deleted successfully!");
}

int MatchesSearch(Student s)
{
    if (strlen(searchText) == 0) return 1;

    char name[60], roll[20], search[60];
    strcpy(name, s.name);
    strcpy(roll, s.roll);
    strcpy(search, searchText);

    for (int i = 0; name[i]; i++) name[i] = (char)tolower((unsigned char)name[i]);
    for (int i = 0; roll[i]; i++) roll[i] = (char)tolower((unsigned char)roll[i]);
    for (int i = 0; search[i]; i++) search[i] = (char)tolower((unsigned char)search[i]);

    return strstr(name, search) != NULL || strstr(roll, search) != NULL;
}

// Draw Dashboard Screen
void DrawDashboard(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawText("STUDENT MANAGEMENT SYSTEM", 40, 25, 28, CUSTOM_WHITE);
    DrawText("C LANGUAGE  |  RAYLIB MODERN WEB DASHBOARD", 42, 60, 14, CUSTOM_MUTED);

    float cardWidth = (sw - 110) / 4.0f;
    if (cardWidth < 200) cardWidth = 200;

    // Stat Cards
    DrawRectangleRounded((Rectangle){40, 95, cardWidth, 80}, 0.15f, 8, CUSTOM_PANEL);
    DrawText("TOTAL STUDENTS", 55, 110, 13, CUSTOM_MUTED);
    DrawText(TextFormat("%d", studentCount), 55, 132, 26, CUSTOM_GREEN);

    DrawRectangleRounded((Rectangle){50 + cardWidth, 95, cardWidth, 80}, 0.15f, 8, CUSTOM_PANEL);
    DrawText("SELECTED STUDENT", 65 + cardWidth, 110, 13, CUSTOM_MUTED);
    DrawText(selectedIndex >= 0 ? students[selectedIndex].roll : "---", 65 + cardWidth, 132, 22, CUSTOM_BLUE);

    // Action Buttons
    float btnY = 190;
    if (DrawButton((Rectangle){40, btnY, 130, 40}, "+ Add Student", CUSTOM_BLUE)) OpenAddForm();
    if (DrawButton((Rectangle){180, btnY, 95, 40}, "Edit", CUSTOM_GREEN)) OpenEditForm();
    if (DrawButton((Rectangle){285, btnY, 95, 40}, "Delete", CUSTOM_RED)) DeleteStudent();

    // Search Bar
    float searchWidth = 280;
    float searchX = sw - searchWidth - 40;
    Rectangle searchRect = {searchX, btnY, searchWidth, 40};
    DrawText("Search Student:", searchX, btnY - 22, 14, CUSTOM_MUTED);
    DrawRectangleRounded(searchRect, 0.15f, 8, CUSTOM_PANEL);
    DrawRectangleLinesEx(searchRect, 2, focusedField == 10 ? CUSTOM_BLUE : (Color){71, 85, 105, 255});
    DrawText(searchText, searchX + 12, btnY + 11, 16, CUSTOM_WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), searchRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        focusedField = 10;
    }
    if (focusedField == 10) HandleInput(searchText, sizeof(searchText), 0);

    // Table Header
    float tableY = 250;
    float tableWidth = sw - 80;
    DrawRectangleRounded((Rectangle){40, tableY, tableWidth, 40}, 0.1f, 8, CUSTOM_BLUE);

    DrawText("ROLL NO.", 55, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("STUDENT NAME", 200, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("COURSE", tableWidth * 0.55f, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("MARKS", tableWidth * 0.78f, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("RESULT", tableWidth * 0.90f, tableY + 12, 15, CUSTOM_WHITE);

    // Filtered Rows
    int visible[MAX_STUDENTS];
    int total = 0;
    for (int i = 0; i < studentCount; i++)
    {
        if (MatchesSearch(students[i])) visible[total++] = i;
    }

    int maxRows = (sh - 380) / 42;
    if (maxRows < 4) maxRows = 4;

    if (scrollOffset > total - maxRows)
        scrollOffset = total > maxRows ? total - maxRows : 0;
    if (scrollOffset < 0) scrollOffset = 0;

    for (int r = 0; r < maxRows; r++)
    {
        int pos = r + scrollOffset;
        if (pos >= total) break;

        int i = visible[pos];
        float rowY = tableY + 48 + (r * 42);
        Rectangle rowRect = {40, rowY, tableWidth, 38};

        DrawRectangleRounded(rowRect, 0.1f, 8,
            selectedIndex == i ? CUSTOM_ROW_ALT : CUSTOM_PANEL);

        DrawText(students[i].roll, 55, rowY + 10, 15, CUSTOM_WHITE);
        DrawText(students[i].name, 200, rowY + 10, 15, CUSTOM_WHITE);
        DrawText(students[i].course, tableWidth * 0.55f, rowY + 10, 15, CUSTOM_WHITE);
        DrawText(TextFormat("%.2f", students[i].marks), tableWidth * 0.78f, rowY + 10, 15, CUSTOM_WHITE);

        Color resColor = students[i].marks >= 40 ? CUSTOM_GREEN : CUSTOM_RED;
        DrawText(students[i].marks >= 40 ? "PASS" : "REVIEW", tableWidth * 0.90f, rowY + 10, 15, resColor);

        if (CheckCollisionPointRec(GetMousePosition(), rowRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            selectedIndex = i;
        }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        scrollOffset -= (int)wheel;
        if (scrollOffset < 0) scrollOffset = 0;
        if (scrollOffset > total - maxRows) scrollOffset = total > maxRows ? total - maxRows : 0;
    }

    // Status Footer
    DrawText(TextFormat("Showing %d of %d students", total, studentCount), 40, sh - 35, 14, CUSTOM_MUTED);
    int statusWidth = MeasureText(statusMessage, 14);
    int statusX = sw - statusWidth - 40;
    if (statusX < 40) statusX = 40;
    DrawText(statusMessage, statusX, sh - 35, 14, CUSTOM_GREEN);
}

// Draw Form Screen
void DrawForm(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 150});

    float formWidth = 500;
    float formHeight = 520;
    float formX = (sw - formWidth) / 2;
    float formY = (sh - formHeight) / 2;

    DrawRectangleRounded((Rectangle){formX, formY, formWidth, formHeight}, 0.05f, 10, CUSTOM_PANEL);
    DrawRectangleLinesEx((Rectangle){formX, formY, formWidth, formHeight}, 1, (Color){51, 65, 85, 255});

    DrawText(editMode ? "EDIT STUDENT RECORD" : "ADD NEW STUDENT", formX + 30, formY + 25, 22, CUSTOM_WHITE);
    DrawText("Fill out the required information below", formX + 30, formY + 55, 14, CUSTOM_MUTED);

    if (strlen(formErrorMessage) > 0)
    {
        DrawText(formErrorMessage, formX + 30, formY + 80, 14, CUSTOM_RED);
    }

    float inputWidth = formWidth - 60;
    Rectangle roll = {formX + 30, formY + 110, inputWidth, 40};
    Rectangle name = {formX + 30, formY + 185, inputWidth, 40};
    Rectangle course = {formX + 30, formY + 260, inputWidth, 40};
    Rectangle marks = {formX + 30, formY + 335, inputWidth, 40};

    DrawInput(roll, "Roll Number", formRoll, 0);
    DrawInput(name, "Student Name", formName, 1);
    DrawInput(course, "Course Name", formCourse, 2);
    DrawInput(marks, "Marks (0-100)", formMarks, 3);

    if (CheckCollisionPointRec(GetMousePosition(), roll) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) focusedField = 0;
    if (CheckCollisionPointRec(GetMousePosition(), name) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) focusedField = 1;
    if (CheckCollisionPointRec(GetMousePosition(), course) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) focusedField = 2;
    if (CheckCollisionPointRec(GetMousePosition(), marks) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) focusedField = 3;

    if (focusedField == 0) HandleInput(formRoll, sizeof(formRoll), 0);
    if (focusedField == 1) HandleInput(formName, sizeof(formName), 0);
    if (focusedField == 2) HandleInput(formCourse, sizeof(formCourse), 0);
    if (focusedField == 3) HandleInput(formMarks, sizeof(formMarks), 1);

    float btnWidth = (inputWidth - 15) / 2;
    if (DrawButton((Rectangle){formX + 30, formY + 415, btnWidth, 45}, "Save Record", CUSTOM_GREEN))
    {
        SaveForm();
    }

    if (DrawButton((Rectangle){formX + 30 + btnWidth + 15, formY + 415, btnWidth, 45}, "Cancel", CUSTOM_RED))
    {
        currentScreen = LIST;
        focusedField = -1;
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1100, 720, "Student Management System - Modern UI");
    SetTargetFPS(60);

    LoadData();

    while (!WindowShouldClose())
    {
        // Use the portable raylib Delete key constant; KEY_KP_DELETE is not
        // available in every raylib target (including the web build).
        if (currentScreen == LIST && IsKeyPressed(KEY_DELETE))
        {
            DeleteStudent();
        }

        BeginDrawing();
        ClearBackground(CUSTOM_BG);

        if (currentScreen == LIST)
            DrawDashboard();
        else
            DrawForm();

        EndDrawing();
    }

    SaveData();
    CloseWindow();

    return 0;
}
