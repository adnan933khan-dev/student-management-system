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
char formErrorMessage[100] = ""; // Form screen ke upar error dikhane ke liye

float panelX = 0;

// Custom Colors (Raylib conflict se bachne ke liye)
Color CUSTOM_BG = {18, 25, 42, 255};
Color CUSTOM_PANEL = {29, 39, 60, 255};
Color CUSTOM_BLUE = {48, 115, 235, 255};
Color CUSTOM_WHITE = {240, 245, 255, 255};
Color CUSTOM_MUTED = {155, 170, 195, 255};
Color CUSTOM_GREEN = {40, 190, 130, 255};
Color CUSTOM_RED = {225, 75, 85, 255};

// Load student records
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

// Save student records
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

// Draw button
int DrawButton(Rectangle rect, const char *text, Color color)
{
    Vector2 mouse = GetMousePosition();

    int hover = CheckCollisionPointRec(mouse, rect);

    DrawRectangleRounded(rect, 0.18f, 8,
                         hover ? CUSTOM_BLUE : color);

    int fontSize = 18;
    int textWidth = MeasureText(text, fontSize);

    DrawText(text,
        rect.x + (rect.width - textWidth) / 2,
        rect.y + (rect.height - fontSize) / 2,
        fontSize, CUSTOM_WHITE);

    return hover &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Draw input field
void DrawInput(Rectangle rect, const char *label,
               const char *value, int field)
{
    DrawText(label, rect.x, rect.y - 25, 16, CUSTOM_MUTED);

    DrawRectangleRounded(rect, 0.12f, 8, CUSTOM_PANEL);

    DrawRectangleLinesEx(rect, 2,
        focusedField == field ? CUSTOM_BLUE : CUSTOM_MUTED);

    DrawText(value, rect.x + 12,
             rect.y + 12, 18, CUSTOM_WHITE);
}

// Handle keyboard input
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
            valid = key >= 32 && key <= 125 &&
                    key != '|';
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

        if (len > 0)
            buffer[len - 1] = '\0';
    }
}

// Open add form
void OpenAddForm(void)
{
    strcpy(formRoll, "");
    strcpy(formName, "");
    strcpy(formCourse, "");
    strcpy(formMarks, "");
    strcpy(formErrorMessage, ""); // Error reset

    selectedIndex = -1;
    editMode = 0;
    focusedField = 0;
    currentScreen = FORM;
}

// Open edit form
void OpenEditForm(void)
{
    if (selectedIndex < 0) return;

    Student s = students[selectedIndex];

    strcpy(formRoll, s.roll);
    strcpy(formName, s.name);
    strcpy(formCourse, s.course);
    strcpy(formErrorMessage, ""); // Error reset

    snprintf(formMarks, sizeof(formMarks), "%.2f", s.marks);

    editMode = 1;
    focusedField = 0;
    currentScreen = FORM;
}

// Save form
void SaveForm(void)
{
    strcpy(formErrorMessage, ""); // Purana error clear karein

    if (strlen(formRoll) == 0 ||
        strlen(formName) == 0 ||
        strlen(formCourse) == 0 ||
        strlen(formMarks) == 0)
    {
        strcpy(formErrorMessage, "Please fill all fields!");
        return;
    }

    float marks = strtof(formMarks, NULL);

    if (marks < 0 || marks > 100)
    {
        strcpy(formErrorMessage, "Marks must be 0 to 100!");
        return;
    }

    // Check duplicate roll number
    for (int i = 0; i < studentCount; i++)
    {
        if (strcmp(students[i].roll, formRoll) == 0)
        {
            if (!editMode || i != selectedIndex)
            {
                strcpy(formErrorMessage, "Student already exists!");
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

    if (index < 0 || index >= MAX_STUDENTS)
        return;

    strcpy(students[index].roll, formRoll);
    strcpy(students[index].name, formName);
    strcpy(students[index].course, formCourse);
    students[index].marks = marks;

    if (!editMode)
        studentCount++;

    SaveData();

    selectedIndex = index;
    currentScreen = LIST;
    focusedField = -1;

    strcpy(statusMessage,
           editMode ? "Student updated successfully!"
                    : "Student added successfully!");
}

// Delete selected student
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

// Search matching students
int MatchesSearch(Student s)
{
    if (strlen(searchText) == 0)
        return 1;

    char name[60], roll[20], search[60];

    strcpy(name, s.name);
    strcpy(roll, s.roll);
    strcpy(search, searchText);

    for (int i = 0; name[i]; i++)
        name[i] = (char)tolower((unsigned char)name[i]);

    for (int i = 0; roll[i]; i++)
        roll[i] = (char)tolower((unsigned char)roll[i]);

    for (int i = 0; search[i]; i++)
        search[i] = (char)tolower((unsigned char)search[i]);

    return strstr(name, search) != NULL ||
           strstr(roll, search) != NULL;
}

// Draw dashboard
void DrawDashboard(void)
{
    DrawText("STUDENT MANAGEMENT SYSTEM", 35, 25, 30, CUSTOM_WHITE);
    DrawText("C LANGUAGE  |  RAYLIB PROJECT", 38, 65, 15, CUSTOM_MUTED);

    // Statistics
    DrawRectangleRounded((Rectangle){35, 100, 280, 90}, 0.12f, 8, CUSTOM_PANEL);
    DrawText("TOTAL STUDENTS", 55, 120, 16, CUSTOM_MUTED);
    DrawText(TextFormat("%d", studentCount), 55, 145, 30, CUSTOM_GREEN);

    DrawRectangleRounded((Rectangle){335, 100, 280, 90}, 0.12f, 8, CUSTOM_PANEL);
    DrawText("SELECTED STUDENT", 355, 120, 16, CUSTOM_MUTED);
    DrawText(selectedIndex >= 0 ? students[selectedIndex].roll : "---", 355, 145, 25, CUSTOM_BLUE);

    // Buttons
    if (DrawButton((Rectangle){35, 215, 125, 42}, "+ Add Student", CUSTOM_BLUE))
    {
        OpenAddForm();
    }

    if (DrawButton((Rectangle){175, 215, 105, 42}, "Edit", CUSTOM_GREEN))
    {
        OpenEditForm();
    }

    if (DrawButton((Rectangle){295, 215, 105, 42}, "Delete", CUSTOM_RED))
    {
        DeleteStudent();
    }

    // Search box
    Rectangle searchRect = {640, 215, 320, 42};
    DrawText("Search:", 640, 190, 15, CUSTOM_MUTED);
    DrawRectangleRounded(searchRect, 0.12f, 8, CUSTOM_PANEL);
    DrawRectangleLinesEx(searchRect, 2, focusedField == 10 ? CUSTOM_BLUE : CUSTOM_MUTED);
    DrawText(searchText, 652, 227, 18, CUSTOM_WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), searchRect) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        focusedField = 10;
    }

    if (focusedField == 10)
        HandleInput(searchText, sizeof(searchText), 0);

    // Table heading
    DrawRectangleRounded((Rectangle){35, 280, 925, 42}, 0.08f, 8, CUSTOM_BLUE);
    DrawText("ROLL NO.", 50, 293, 16, CUSTOM_WHITE);
    DrawText("STUDENT NAME", 190, 293, 16, CUSTOM_WHITE);
    DrawText("COURSE", 485, 293, 16, CUSTOM_WHITE);
    DrawText("MARKS", 735, 293, 16, CUSTOM_WHITE);
    DrawText("RESULT", 850, 293, 16, CUSTOM_WHITE);

    // Filtered records
    int visible[MAX_STUDENTS];
    int total = 0;

    for (int i = 0; i < studentCount; i++)
    {
        if (MatchesSearch(students[i]))
            visible[total++] = i;
    }

    int maxRows = 8;

    if (scrollOffset > total - maxRows)
        scrollOffset = total > maxRows ? total - maxRows : 0;

    for (int r = 0; r < maxRows; r++)
    {
        int pos = r + scrollOffset;

        if (pos >= total) break;

        int i = visible[pos];
        float y = 330 + r * 40;
        Rectangle row = {35, y, 925, 36};

        DrawRectangleRounded(row, 0.08f, 8,
            selectedIndex == i ? (Color){45, 75, 115, 255}
                               : CUSTOM_PANEL);

        DrawText(students[i].roll, 50, y + 9, 15, CUSTOM_WHITE);
        DrawText(students[i].name, 190, y + 9, 15, CUSTOM_WHITE);
        DrawText(students[i].course, 485, y + 9, 15, CUSTOM_WHITE);
        DrawText(TextFormat("%.2f", students[i].marks), 735, y + 9, 15, CUSTOM_WHITE);
        DrawText(students[i].marks >= 40 ? "PASS" : "REVIEW", 850, y + 9, 15, students[i].marks >= 40 ? CUSTOM_GREEN : CUSTOM_RED);

        if (CheckCollisionPointRec(GetMousePosition(), row) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            selectedIndex = i;
        }
    }

    // Scroll
    float wheel = GetMouseWheelMove();

    if (wheel != 0)
    {
        scrollOffset -= (int)wheel;

        if (scrollOffset < 0)
            scrollOffset = 0;

        if (scrollOffset > total - maxRows)
            scrollOffset = total > maxRows ? total - maxRows : 0;
    }

    DrawText(TextFormat("Showing %d of %d students", total, studentCount), 40, 665, 15, CUSTOM_MUTED);
    DrawText(statusMessage, 400, 665, 15, CUSTOM_GREEN);
}

// Draw student form
void DrawForm(void)
{
    DrawRectangleRounded((Rectangle){230, 95, 540, 580}, 0.06f, 10, CUSTOM_PANEL);
    
    DrawText(editMode ? "UPDATE STUDENT" : "ADD NEW STUDENT", 290, 120, 27, CUSTOM_WHITE);
    DrawText("Enter student information below", 290, 155, 15, CUSTOM_MUTED);

    // Form screen par error show karne ke liye
    if (strlen(formErrorMessage) > 0)
    {
        DrawText(formErrorMessage, 290, 180, 16, CUSTOM_RED);
    }

    Rectangle roll = {290, 225, 420, 42};
    Rectangle name = {290, 305, 420, 42};
    Rectangle course = {290, 385, 420, 42};
    Rectangle marks = {290, 465, 420, 42};

    DrawInput(roll, "Roll Number", formRoll, 0);
    DrawInput(name, "Student Name", formName, 1);
    DrawInput(course, "Course", formCourse, 2);
    DrawInput(marks, "Marks (0-100)", formMarks, 3);

    if (CheckCollisionPointRec(GetMousePosition(), roll) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        focusedField = 0;

    if (CheckCollisionPointRec(GetMousePosition(), name) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        focusedField = 1;

    if (CheckCollisionPointRec(GetMousePosition(), course) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        focusedField = 2;

    if (CheckCollisionPointRec(GetMousePosition(), marks) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        focusedField = 3;

    if (focusedField == 0)
        HandleInput(formRoll, sizeof(formRoll), 0);

    if (focusedField == 1)
        HandleInput(formName, sizeof(formName), 0);

    if (focusedField == 2)
        HandleInput(formCourse, sizeof(formCourse), 0);

    if (focusedField == 3)
        HandleInput(formMarks, sizeof(formMarks), 1);

    if (DrawButton((Rectangle){290, 545, 190, 48}, "Save Student", CUSTOM_GREEN))
    {
        SaveForm();
    }

    if (DrawButton((Rectangle){510, 545, 200, 48}, "Cancel", CUSTOM_RED))
    {
        currentScreen = LIST;
        focusedField = -1;
    }
}

int main(void)
{
    InitWindow(1000, 710, "Student Management System");
    SetTargetFPS(60);

    LoadData();

    while (!WindowShouldClose())
    {
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