#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#define ROWS 10
#define COLS 10
#define DEFAULT_CURSOR_POS ((Position) { .x = 0, .y = 0 })
#define BOMBS_PERCENTAGE 10

typedef enum {
    Closed,
    Open,
    Marked
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
} Cell;

typedef struct {
    bool generated;
    Cell** cells;
    Size size;
    Position cursor;
} Field;

void field_reset(Field* field, size_t rows, size_t cols, Position cursor) {
    field->generated = false;

    if (field->cells  != NULL) free(field->cells);
    
    field->cells = (Cell**)(malloc(sizeof(Cell*) * rows));
    for (size_t row = 0; row < rows; row++) {
        field->cells[row] = (Cell*)(malloc(sizeof(Cell) * cols));
        for (size_t col = 0; col < cols; col++) {
            field->cells[row][col] = (Cell) {
                .is_bomb = false,
                .state   = Closed,
            };
        }
    }

    field->size = (Size) {
        .rows = rows,
        .cols = cols
    };

    field->cursor = cursor;
}

bool is_cursor_on_cell(Field* field, size_t row, size_t col) {
    return field->cursor.x == col && field->cursor.y == row;
}

size_t cell_count_nbor_bombs(Field* field, size_t row, size_t col) {
    size_t nbor_bombs = 0;
    for (int drow = -1; drow <= 1; drow++) {
        int r = (int)row + drow;
        if (r < 0 || r >= (int)field->size.rows) continue;
        for (int dcol = -1; dcol <= 1; dcol++) {
            int c = (int)col + dcol;
            if (c < 0 || c >= (int)field->size.cols) continue;
            nbor_bombs += field->cells[r][c].is_bomb;
        }
    }
    return nbor_bombs;
}

size_t cell_count_marked_nbors(Field* field, size_t row, size_t col) {
    size_t open_nbor_cells = 0;
    for (int drow = -1; drow <= 1; drow++) {
        int r = (int)row + drow;
        if (r < 0 || r >= (int)field->size.rows) continue;
        for (int dcol = -1; dcol <= 1; dcol++) {
            int c = (int)col + dcol;
            if (c < 0 || c >= (int)field->size.cols) continue;
            open_nbor_cells += field->cells[r][c].state == Marked;
        }
    }
    return open_nbor_cells;

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
            printf("%c", is_cursor_on_cell(field, row, col) ? '[': ' ');
            switch (cell.state) {
                case Closed:
                    printf(".");
                    break;
                case Marked:
                    printf("*");
                    break;
                case Open: {
                    Cell cell = field->cells[row][col];
                    if (cell.is_bomb) {
                        printf("@");
                        break;
                    }
                    size_t nbors_bombs = cell_count_nbor_bombs(field, row, col);
                    if (nbors_bombs == 0) {
                        printf(" ");
                        break;
                    }
                    printf("%zu", nbors_bombs);
                    break;
                }
            }
            printf("%c", is_cursor_on_cell(field, row, col) ? ']': ' ');
        }
        printf("\n");
    }
}

void field_redisplay(Field* field) {
    printf("%c[%zuA", 27, field->size.rows);
    printf("%c[%luD", 27, field->size.cols*3);
    field_display(field);
}

void random_cell(Field* field, size_t* row, size_t* col) {
    *row = rand() % field->size.rows;
    *col = rand() % field->size.cols;
}

void field_randomize(Field* field, size_t bombs_percentage) {
    bombs_percentage = bombs_percentage > 100 ? 100 : bombs_percentage;
    size_t bombs_count = (bombs_percentage * field->size.rows * field->size.cols) / 100;
    for (size_t row = 0; row < field->size.rows; row++) {
        for (size_t col = 0; col < field->size.cols; col++) {
            field->cells[row][col].state = Closed;
            field->cells[row][col].is_bomb = false;
        }
    }
    for (size_t i = 0; i < bombs_count; i++) {
        size_t row, col;
        do {
            random_cell(field, &row, &col);
        } while (field->cells[row][col].is_bomb || row == field->cursor.y || col == field->cursor.x);
        field->cells[row][col].is_bomb = true;
    }
}

bool field_open_all_nbors(Field* field, size_t row, size_t col) {
    for (int drow = -1; drow <= 1; drow++) {
        int r = (int)row + drow;
        if (r < 0 || r >= (int)field->size.rows) continue;
        for (int dcol = -1; dcol <= 1; dcol++) {
            int c = (int)col + dcol;
            if (c < 0 || c >= (int)field->size.cols) continue;
            if (field->cells[r][c].state == Closed) {
                field->cells[r][c].state = Open;
                if (field->cells[r][c].is_bomb) return false;
                if (cell_count_nbor_bombs(field, r, c) == 0) {
                    field_open_all_nbors(field, r, c);
                }
            }
        }
    }
    return true;
}

bool field_open_at(Field* field, size_t row, size_t col) {
    if (!field->generated) {
        field_randomize(field, BOMBS_PERCENTAGE);
        field->generated = true;
    }
    Cell* cell = &field->cells[row][col];
    switch (cell->state) {
        case Closed: {
            cell->state = Open;
            if (cell->is_bomb) return false;
            size_t nbor_bombs = cell_count_nbor_bombs(field, row, col);
            if (nbor_bombs == 0) {
                return field_open_all_nbors(field, row, col);
            }
            break;
        }
        case Open: {
            size_t nbor_bombs = cell_count_nbor_bombs(field, row, col);
            size_t opened_nbor_bombs = cell_count_marked_nbors(field, row, col);
            if (nbor_bombs == opened_nbor_bombs) {
                return field_open_all_nbors(field, row, col);
            }
            break;
        }
        default: {
            break;
        }
    }
    return true;
}

void field_mark_at(Field* field, size_t row, size_t col) {
    if (!field->generated) {
        field_randomize(field, BOMBS_PERCENTAGE);
        field->generated = true;
    }
    Cell* cell = &field->cells[row][col];
    if (cell->state == Open) return;
    cell->state = cell->state == Closed ? Marked : Closed;
}

void field_open_all_cells(Field* field) {
    for (size_t row = 0; row < field->size.rows; row++) {
        for (size_t col = 0; col < field->size.cols; col++) {
            field_open_at(field, row, col);
        }
    }
}

void field_open_all_bombs(Field* field) {
    for (size_t row = 0; row < field->size.rows; row++) {
        for (size_t col = 0; col < field->size.cols; col++) {
            Cell* cell = &field->cells[row][col];
            if (cell->is_bomb) {
                cell->state = Open;
            }
        }
    }
}

void cursor_move_up(Field* field) {
    field->cursor.y = (field->cursor.y + field->size.rows - 1) % field->size.rows;
}

void cursor_move_down(Field* field) {
    field->cursor.y = (field->cursor.y + field->size.rows + 1) % field->size.rows;
}

void cursor_move_left(Field* field) {
    field->cursor.x = (field->cursor.x + field->size.cols - 1) % field->size.cols;
}

void cursor_move_right(Field* field) {
    field->cursor.x = (field->cursor.x + field->size.cols + 1) % field->size.cols;
}

int main(void) {
    srand(time(NULL));

    Field field;
    field_reset(&field, ROWS, COLS, DEFAULT_CURSOR_POS);
    field_display(&field);
    field_randomize(&field, BOMBS_PERCENTAGE);

    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);          
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int exit_code = 0;

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
            case ' ': {
                if (!field_open_at(&field, field.cursor.y, field.cursor.x)) {
                    field_open_all_bombs(&field);
                    quit = true;
                    field_redisplay(&field);
                    printf("You lost!");
                    exit_code = 1;
                } else {
                    field_redisplay(&field);
                }
                break;
            }
            case 'f':
                field_mark_at(&field, field.cursor.y, field.cursor.x);
                field_redisplay(&field);
                break;
            case 'r':
                field_reset(&field, ROWS, COLS, field.cursor);
                field_redisplay(&field);
                break;
            case 'q':
                quit = true;
        }
    }
    field_delete(&field);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return exit_code;
}
