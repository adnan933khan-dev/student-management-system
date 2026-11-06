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
int showDeleteDialog = 0;

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

// Modern Dark Theme Colors
Color CUSTOM_BG = {15, 23, 42, 255};
Color CUSTOM_PANEL = {30, 41, 59, 255};
Color CUSTOM_BLUE = {59, 130, 246, 255};
Color CUSTOM_WHITE = {248, 250, 252, 255};
Color CUSTOM_MUTED = {148, 163, 184, 255};
Color CUSTOM_GREEN = {16, 185, 129, 255};
Color CUSTOM_RED = {239, 68, 68, 255};
Color CUSTOM_HOVER = {37, 99, 235, 255};
Color CUSTOM_ROW_ALT = {35, 48, 68, 255};

static void TrimInPlace(char *str)
{
    if (str == NULL) return;

    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
    {
        str[--len] = '\0';
    }

    char *start = str;
    while (*start != '\0' && isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
}

static void CopyLowercase(const char *src, char *dst, size_t dstSize)
{
    if (src == NULL || dst == NULL || dstSize == 0) return;

    size_t i = 0;
    while (src[i] != '\0' && i < dstSize - 1)
    {
        dst[i] = (char)tolower((unsigned char)src[i]);
        i++;
    }
    dst[i] = '\0';
}

static int IsValidMarksString(const char *text)
{
    if (text == NULL || text[0] == '\0') return 0;

    char temp[32];
    strncpy(temp, text, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    TrimInPlace(temp);
    if (temp[0] == '\0') return 0;

    char *end = NULL;
    double value = strtod(temp, &end);
    if (end == temp) return 0;
    while (*end != '\0' && isspace((unsigned char)*end)) end++;
    return *end == '\0' && value >= 0.0 && value <= 100.0;
}

void OpenDeleteDialog(void)
{
    if (selectedIndex < 0 || selectedIndex >= studentCount) return;
    showDeleteDialog = 1;
}

void CloseDeleteDialog(void)
{
    showDeleteDialog = 0;
}

void LoadData(void)
{
    FILE *file = fopen("students.txt", "r");
    if (file == NULL) return;

    studentCount = 0;
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
        fprintf(file, "%s|%s|%s|%.2f\n", students[i].roll, students[i].name,
                students[i].course, students[i].marks);
    }
    fclose(file);
}

int DrawButton(Rectangle rect, const char *text, Color color)
{
    Vector2 mouse = GetMousePosition();
    int hover = CheckCollisionPointRec(mouse, rect);
    DrawRectangleRounded(rect, 0.2f, 8, hover ? CUSTOM_HOVER : color);

    int fontSize = 16;
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, rect.x + (rect.width - textWidth) / 2,
             rect.y + (rect.height - fontSize) / 2, fontSize, CUSTOM_WHITE);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawInput(Rectangle rect, const char *label, const char *value, int field)
{
    DrawText(label, rect.x, rect.y - 22, 15, CUSTOM_MUTED);
    DrawRectangleRounded(rect, 0.15f, 8, CUSTOM_PANEL);
    DrawRectangleLinesEx(rect, 2, focusedField == field ? CUSTOM_BLUE : (Color){71, 85, 105, 255});
    DrawText(strlen(value) > 0 ? value : "Type here...", rect.x + 14, rect.y + 12,
             17, strlen(value) > 0 ? CUSTOM_WHITE : (Color){100, 116, 139, 255});
}

void HandleInput(char *buffer, int maxLength, int numberOnly)
{
    int key = GetCharPressed();
    while (key > 0)
    {
        int valid = numberOnly
            ? ((key >= '0' && key <= '9') || (key == '.' && strchr(buffer, '.') == NULL))
            : (key >= 32 && key <= 125 && key != '|');
        int len = (int)strlen(buffer);
        if (valid && len < maxLength - 1)
        {
            buffer[len] = (char)key;
            buffer[len + 1] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        size_t len = strlen(buffer);
        if (len > 0) buffer[len - 1] = '\0';
    }
}

void ResetFormFields(void)
{
    formRoll[0] = formName[0] = formCourse[0] = formMarks[0] = formErrorMessage[0] = '\0';
}

void OpenAddForm(void)
{
    ResetFormFields();
    selectedIndex = -1;
    editMode = 0;
    focusedField = 0;
    currentScreen = FORM;
    CloseDeleteDialog();
}

void OpenEditForm(void)
{
    if (selectedIndex < 0 || selectedIndex >= studentCount) return;

    Student s = students[selectedIndex];
    snprintf(formRoll, sizeof(formRoll), "%s", s.roll);
    snprintf(formName, sizeof(formName), "%s", s.name);
    snprintf(formCourse, sizeof(formCourse), "%s", s.course);
    snprintf(formMarks, sizeof(formMarks), "%.2f", s.marks);
    formErrorMessage[0] = '\0';
    editMode = 1;
    focusedField = 0;
    currentScreen = FORM;
    CloseDeleteDialog();
}

void SaveForm(void)
{
    char roll[sizeof(formRoll)], name[sizeof(formName)];
    char course[sizeof(formCourse)], marks[sizeof(formMarks)];
    strcpy(roll, formRoll);
    strcpy(name, formName);
    strcpy(course, formCourse);
    strcpy(marks, formMarks);
    TrimInPlace(roll);
    TrimInPlace(name);
    TrimInPlace(course);
    TrimInPlace(marks);
    formErrorMessage[0] = '\0';

    if (roll[0] == '\0' || name[0] == '\0' || course[0] == '\0' || marks[0] == '\0')
    {
        strcpy(formErrorMessage, "Please fill all fields!");
        return;
    }
    if (!IsValidMarksString(marks))
    {
        strcpy(formErrorMessage, "Marks must be between 0 and 100!");
        return;
    }

    float marksValue = strtof(marks, NULL);
    for (int i = 0; i < studentCount; i++)
    {
        if (strcmp(students[i].roll, roll) == 0 && (!editMode || i != selectedIndex))
        {
            strcpy(formErrorMessage, "Student with this Roll No already exists!");
            return;
        }
    }
    if (!editMode && studentCount >= MAX_STUDENTS)
    {
        strcpy(formErrorMessage, "Student limit reached!");
        return;
    }

    int index = editMode ? selectedIndex : studentCount;
    if (index < 0 || index >= MAX_STUDENTS || (editMode && index >= studentCount)) return;
    strcpy(students[index].roll, roll);
    strcpy(students[index].name, name);
    strcpy(students[index].course, course);
    students[index].marks = marksValue;
    if (!editMode) studentCount++;

    SaveData();
    selectedIndex = index;
    currentScreen = LIST;
    focusedField = -1;
    scrollOffset = 0;
    strcpy(statusMessage, editMode ? "Student updated successfully!" : "Student added successfully!");
    snprintf(formRoll, sizeof(formRoll), "%s", roll);
    snprintf(formName, sizeof(formName), "%s", name);
    snprintf(formCourse, sizeof(formCourse), "%s", course);
    snprintf(formMarks, sizeof(formMarks), "%s", marks);
}

void DeleteStudent(void)
{
    if (selectedIndex < 0 || selectedIndex >= studentCount) return;

    for (int i = selectedIndex; i < studentCount - 1; i++) students[i] = students[i + 1];
    studentCount--;
    selectedIndex = -1;
    scrollOffset = 0;
    SaveData();
    strcpy(statusMessage, "Student deleted successfully!");
    CloseDeleteDialog();
}

int MatchesSearch(Student s)
{
    if (searchText[0] == '\0') return 1;
    char name[60], roll[20], search[60];
    CopyLowercase(s.name, name, sizeof(name));
    CopyLowercase(s.roll, roll, sizeof(roll));
    CopyLowercase(searchText, search, sizeof(search));
    return strstr(name, search) != NULL || strstr(roll, search) != NULL;
}

void DrawDeleteDialog(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 150});

    float modalWidth = 420, modalHeight = 180;
    float modalX = (sw - modalWidth) / 2, modalY = (sh - modalHeight) / 2;
    DrawRectangleRounded((Rectangle){modalX, modalY, modalWidth, modalHeight}, 0.12f, 12, CUSTOM_PANEL);
    DrawRectangleLinesEx((Rectangle){modalX, modalY, modalWidth, modalHeight}, 2, CUSTOM_RED);
    DrawText("Delete Student?", modalX + 25, modalY + 25, 24, CUSTOM_WHITE);

    const char *nameText = students[selectedIndex].name;
    const char *rollText = students[selectedIndex].roll;
    DrawText(TextFormat("You are about to delete %s", nameText), modalX + 25, modalY + 68, 17, CUSTOM_MUTED);
    DrawText(TextFormat("Roll No: %s", rollText), modalX + 25, modalY + 95, 17, CUSTOM_BLUE);
    DrawText("This action cannot be undone.", modalX + 25, modalY + 120, 15, CUSTOM_MUTED);

    float btnWidth = 150, btnY = modalY + modalHeight - 48;
    if (DrawButton((Rectangle){modalX + 25, btnY, btnWidth, 40}, "Cancel", CUSTOM_PANEL))
        CloseDeleteDialog();
    if (DrawButton((Rectangle){modalX + modalWidth - btnWidth - 25, btnY, btnWidth, 40}, "Delete", CUSTOM_RED))
        DeleteStudent();
}

void DrawDashboard(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawText("STUDENT MANAGEMENT SYSTEM", 40, 25, 28, CUSTOM_WHITE);
    DrawText("C LANGUAGE  |  RAYLIB MODERN WEB/DESKTOP APP", 42, 60, 14, CUSTOM_MUTED);

    float cardWidth = (sw - 110) / 4.0f;
    if (cardWidth < 200) cardWidth = 200;
    DrawRectangleRounded((Rectangle){40, 95, cardWidth, 80}, 0.15f, 8, CUSTOM_PANEL);
    DrawText("TOTAL STUDENTS", 55, 110, 13, CUSTOM_MUTED);
    DrawText(TextFormat("%d", studentCount), 55, 132, 26, CUSTOM_GREEN);
    DrawRectangleRounded((Rectangle){50 + cardWidth, 95, cardWidth, 80}, 0.15f, 8, CUSTOM_PANEL);
    DrawText("SELECTED STUDENT", 65 + cardWidth, 110, 13, CUSTOM_MUTED);
    DrawText(selectedIndex >= 0 && selectedIndex < studentCount ? students[selectedIndex].roll : "---",
             65 + cardWidth, 132, 22, CUSTOM_BLUE);

    float btnY = 190;
    if (DrawButton((Rectangle){40, btnY, 130, 40}, "+ Add Student", CUSTOM_BLUE)) OpenAddForm();
    if (DrawButton((Rectangle){180, btnY, 95, 40}, "Edit", CUSTOM_GREEN)) OpenEditForm();
    if (DrawButton((Rectangle){285, btnY, 95, 40}, "Delete", CUSTOM_RED)) OpenDeleteDialog();

    float searchWidth = 280, searchX = sw - 280 - 40;
    Rectangle searchRect = {searchX, btnY, searchWidth, 40};
    DrawText("Search Student:", searchX, btnY - 22, 14, CUSTOM_MUTED);
    DrawRectangleRounded(searchRect, 0.15f, 8, CUSTOM_PANEL);
    DrawRectangleLinesEx(searchRect, 2, focusedField == 10 ? CUSTOM_BLUE : (Color){71, 85, 105, 255});
    DrawText(searchText, searchX + 12, btnY + 11, 16, CUSTOM_WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), searchRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        focusedField = 10;
    if (focusedField == 10) HandleInput(searchText, sizeof(searchText), 0);

    float tableY = 250, tableWidth = sw - 80;
    DrawRectangleRounded((Rectangle){40, tableY, tableWidth, 40}, 0.1f, 8, CUSTOM_BLUE);
    DrawText("ROLL NO.", 55, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("STUDENT NAME", 200, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("COURSE", tableWidth * 0.55f, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("MARKS", tableWidth * 0.78f, tableY + 12, 15, CUSTOM_WHITE);
    DrawText("RESULT", tableWidth * 0.90f, tableY + 12, 15, CUSTOM_WHITE);

    int visible[MAX_STUDENTS], total = 0;
    for (int i = 0; i < studentCount; i++)
        if (MatchesSearch(students[i])) visible[total++] = i;

    int maxRows = (sh - 380) / 42;
    if (maxRows < 4) maxRows = 4;
    if (scrollOffset > total - maxRows) scrollOffset = total > maxRows ? total - maxRows : 0;
    if (scrollOffset < 0) scrollOffset = 0;

    for (int r = 0; r < maxRows; r++)
    {
        int pos = r + scrollOffset;
        if (pos >= total) break;
        int i = visible[pos];
        float rowY = tableY + 48 + r * 42;
        Rectangle rowRect = {40, rowY, tableWidth, 38};
        DrawRectangleRounded(rowRect, 0.1f, 8, selectedIndex == i ? CUSTOM_ROW_ALT : CUSTOM_PANEL);
        DrawText(students[i].roll, 55, rowY + 10, 15, CUSTOM_WHITE);
        DrawText(students[i].name, 200, rowY + 10, 15, CUSTOM_WHITE);
        DrawText(students[i].course, tableWidth * 0.55f, rowY + 10, 15, CUSTOM_WHITE);
        DrawText(TextFormat("%.2f", students[i].marks), tableWidth * 0.78f, rowY + 10, 15, CUSTOM_WHITE);
        DrawText(students[i].marks >= 40 ? "PASS" : "REVIEW", tableWidth * 0.90f, rowY + 10, 15,
                 students[i].marks >= 40 ? CUSTOM_GREEN : CUSTOM_RED);
        if (CheckCollisionPointRec(GetMousePosition(), rowRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            selectedIndex = i;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        scrollOffset -= (int)wheel;
        if (scrollOffset < 0) scrollOffset = 0;
        if (scrollOffset > total - maxRows) scrollOffset = total > maxRows ? total - maxRows : 0;
    }

    DrawText(TextFormat("Showing %d of %d students", total, studentCount), 40, sh - 35, 14, CUSTOM_MUTED);
    int statusWidth = MeasureText(statusMessage, 14);
    int statusX = sw - statusWidth - 40;
    if (statusX < 40) statusX = 40;
    DrawText(statusMessage, statusX, sh - 35, 14, CUSTOM_GREEN);

    if (showDeleteDialog && selectedIndex >= 0 && selectedIndex < studentCount)
        DrawDeleteDialog();
}

void DrawForm(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 150});
    float formWidth = 500, formHeight = 520;
    float formX = (sw - formWidth) / 2, formY = (sh - formHeight) / 2;
    DrawRectangleRounded((Rectangle){formX, formY, formWidth, formHeight}, 0.05f, 10, CUSTOM_PANEL);
    DrawRectangleLinesEx((Rectangle){formX, formY, formWidth, formHeight}, 1, (Color){51, 65, 85, 255});
    DrawText(editMode ? "EDIT STUDENT RECORD" : "ADD NEW STUDENT", formX + 30, formY + 25, 22, CUSTOM_WHITE);
    DrawText("Fill out the required information below", formX + 30, formY + 55, 14, CUSTOM_MUTED);
    if (formErrorMessage[0] != '\0')
        DrawText(formErrorMessage, formX + 30, formY + 80, 14, CUSTOM_RED);

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
    if (DrawButton((Rectangle){formX + 30, formY + 415, btnWidth, 45}, "Save Record", CUSTOM_GREEN)) SaveForm();
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
        if (currentScreen == LIST)
        {
            if (showDeleteDialog)
            {
                if (IsKeyPressed(KEY_ESCAPE)) CloseDeleteDialog();
                if (IsKeyPressed(KEY_ENTER)) DeleteStudent();
            }
            else
            {
                if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_KP_DELETE)) OpenDeleteDialog();
                if (IsKeyPressed(KEY_TAB) && selectedIndex >= 0 && studentCount > 0)
                    selectedIndex = (selectedIndex + 1) % studentCount;
            }
        }
        else if (currentScreen == FORM)
        {
            if (IsKeyPressed(KEY_TAB)) focusedField = (focusedField + 1) % 4;
            if (IsKeyPressed(KEY_ENTER)) SaveForm();
            if (IsKeyPressed(KEY_ESCAPE))
            {
                currentScreen = LIST;
                focusedField = -1;
            }
        }

        BeginDrawing();
        ClearBackground(CUSTOM_BG);
        if (currentScreen == LIST) DrawDashboard();
        else DrawForm();
        EndDrawing();
    }

    SaveData();
    CloseWindow();
    return 0;
}
