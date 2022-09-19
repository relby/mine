#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define ROWS 10
#define COLS 10
#define BOMBS_PERCENTAGE 20

typedef enum {
    Closed,
    Open
} CellState;

typedef struct {
    size_t x, y;
} Position;

typedef struct {
    size_t rows, cols;
} Size;

typedef struct {
    bool is_bomb;
    CellState state;
    Position pos;
} Cell;


typedef struct {
    bool generated;
    Cell** cells;
    Size size;
    Position cursor;
} Field;

void field_reset(Field* field, size_t rows, size_t cols) {
    field->generated = false;

    if (field->cells  != NULL) free(field->cells);
    
    field->cells = (Cell**)(malloc(sizeof(Cell*) * rows));
    for (size_t row = 0; row < rows; row++) {
        field->cells[row] = (Cell*)(malloc(sizeof(Cell) * cols));
        for (size_t col = 0; col < cols; col++) {
            field->cells[row][col] = (Cell) {
                .is_bomb = false,
                .state   = Closed,
                .pos     = { .x = row, .y = col }
            };
        }
    }

    field->size = (Size) {
        .rows = rows,
        .cols = cols
    };

    field->cursor = (Position) {
        .x = 0,
        .y = 0
    };
}

bool is_cursor_on_cell(Field* field, Cell* cell) {
    return field->cursor.x == cell->pos.x && field->cursor.y == cell->pos.y;
}

void field_delete(Field* field) {
    for (size_t row = 0; row < field->size.rows; row++) {
        free(field->cells[row]);
    }
    free(field->cells);
}

void field_display(Field* field) {
    for (size_t row = 0; row < field->size.rows; row++) {
        for (size_t col = 0; col < field->size.cols; col++) {
            Cell cell = field->cells[row][col];
            printf("%c", is_cursor_on_cell(field, &cell) ? '[': ' ');
            printf(".");
            printf("%c", is_cursor_on_cell(field, &cell) ? ']': ' ');
        }
        printf("\n");
    }
}

void field_redisplay(Field* field) {
    printf("%c[%zuA", 27, field->size.rows);
    printf("%c[%luD", 27, field->size.cols*3);
    field_display(field);
}

void cursor_move_up(Field* field) {
    field->cursor.x = (field->cursor.x + field->size.rows - 1) % field->size.rows;
}

void cursor_move_down(Field* field) {
    field->cursor.x = (field->cursor.x + field->size.rows + 1) % field->size.rows;
}

void cursor_move_left(Field* field) {
    field->cursor.y = (field->cursor.y + field->size.rows - 1) % field->size.rows;
}

void cursor_move_right(Field* field) {
    field->cursor.y = (field->cursor.y + field->size.rows + 1) % field->size.rows;
}

int main(void) {
    Field field;
    field_reset(&field, ROWS, COLS);
    field_display(&field);

    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);          
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    bool quit = false;
    while (!quit) {
        char cmd = getchar();
        switch (cmd) {
            case 'j':
                cursor_move_down(&field);
                field_redisplay(&field);
                break;
            case 'k':
                cursor_move_up(&field);
                field_redisplay(&field);
                break;
            case 'h':
                cursor_move_left(&field);
                field_redisplay(&field);
                break;
            case 'l':
                cursor_move_right(&field);
                field_redisplay(&field);
                break;
            case 'q':
                quit = true;
        }
    }
    field_delete(&field);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}