#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define INITIAL_SIZE 256
#define GROWTH_FACTOR 2

// Helper Functions
void swap(char *a, char *b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

char *string_slice(const char *input, size_t start, size_t end) {
    if (input == NULL || start > end || end > strlen(input)) {
        return NULL; // Invalid input or range
    }

    size_t slice_length = end - start;
    char *result = (char *)malloc(slice_length + 1); // Allocate memory for slice (+1 for '\0')
    if (result == NULL) {
        perror("Failed to allocate memory");
        exit(1);
    }

    strncpy(result, input + start, slice_length); // Copy the slice
    result[slice_length] = '\0'; // Null-terminate the string
    return result;
}

char *strsep(char **stringp, const char *delim) {
    char *start = *stringp;
    char *p;

    if (start == NULL) return NULL;  // No more tokens

    p = strpbrk(start, delim);  // Find the first delimiter in the string
    if (p) {
        *p = '\0';  // Replace delimiter with null terminator
        *stringp = p + 1;  // Update string pointer to the next token
    } else {
        *stringp = NULL;  // No more delimiters, set string pointer to NULL
    }

    return start;
}

// Global Variables
int line_number;
int cursor_line;
int cursor_highlight_start;
bool highlight_flag = 0;
int highlight_start;
int highlight_end;
int temp_flag;
int render_y_off = 25;
int currentTextBlockHeight;
int scroll_count;
int netWidth;
int netHeight;

const int MENU_HEIGHT = 20;
const int MENU_ITEM_WIDTH = 40;

SDL_Color highlightColor = {173, 200, 255, 255};
SDL_Color textColor = {0, 0, 0, 255}; // Black color
enum MenuItem { FILE_QUIT, FILE_COUNT };

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
TTF_Font* font_menu = NULL;

void draw_cursor() {
    SDL_SetRenderDrawColor(renderer, 0, 150, 150, 100);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    int cursorY = 20 + ((cursor_line * TTF_FontHeight(font)) + render_y_off);
   
    if (cursorY >= 20 && cursorY < netHeight) {
        cursor_highlight_start = 25 + ((cursor_line * TTF_FontHeight(font)) + scroll_count * TTF_FontHeight(font)) % netHeight;
        SDL_Rect cursor_highlight = {0, cursor_highlight_start, netWidth, TTF_FontHeight(font)};
        SDL_RenderFillRect(renderer, &cursor_highlight);
    }
}

void draw_menu_bar() {
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    SDL_Rect menuBar = {0, 0, netWidth, MENU_HEIGHT};
    SDL_RenderFillRect(renderer, &menuBar);

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_Rect fileItem = {0, 0, MENU_ITEM_WIDTH, MENU_HEIGHT};
    SDL_RenderFillRect(renderer, &fileItem);

    font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);
    SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, "File", textColor);
    if (!textSurface) {
        SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    int textWidth = 0, textHeight = 0;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect textRect = {fileItem.x + (fileItem.w - textWidth) / 2, fileItem.y + (fileItem.h - textHeight) / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
}

void save_file(FILE *file, char *textBuffer, int bufferIndex) {
    ftruncate(fileno(file), 0);
    rewind(file);
    for (int cursor = 0; cursor < bufferIndex; cursor++) {
        fputc(textBuffer[cursor], file);
    }
    fclose(file);
}

void handle_input(SDL_Event *e, char **textBuffer, int *bufferIndex, int *cursor, int buffer_size, FILE *file) {
    if (e->type == SDL_KEYDOWN) {
        SDL_Keymod mod = SDL_GetModState();
        if (e->key.keysym.sym == SDLK_BACKSPACE && *cursor > 0) {
            if (highlight_flag == 0) {
                for (int i = *cursor - 1; i < *bufferIndex; i++) {
                    (*textBuffer)[i] = (*textBuffer)[i + 1];
                }
                (*cursor)--;
                (*bufferIndex)--;
            } else {
                if (*cursor <= highlight_start) {
                    for (int i = highlight_start; i < highlight_end; i++) {
                        swap(&(*textBuffer)[*cursor], &(*textBuffer)[*cursor + 1]);
                        (*cursor)++;
                    }
                }
                for (int i = highlight_end; i > highlight_start; i--) {
                    for (int j = 1; j < *bufferIndex; j++) {
                        (*textBuffer)[*cursor + j] = (*textBuffer)[*cursor + j + 1];
                    }
                    (*bufferIndex)--;
                    (*cursor)--;
                }
            }
        } else if (e->key.keysym.sym == SDLK_RETURN) {
            for (int i = *bufferIndex; i >= *cursor; i--) {
                (*textBuffer)[i + 1] = (*textBuffer)[i];
            }
            (*textBuffer)[*cursor] = '\n';
            (*cursor)++;
            (*bufferIndex)++;
        } else if (e->key.keysym.sym == SDLK_LEFT && *cursor > 0) {
            swap(&(*textBuffer)[*cursor - 1], &(*textBuffer)[*cursor]);
            (*cursor)--;
        } else if (e->key.keysym.sym == SDLK_RIGHT && *cursor < *bufferIndex) {
            swap(&(*textBuffer)[*cursor + 1], &(*textBuffer)[*cursor]);
            (*cursor)++;
        } else if (e->key.keysym.sym == SDLK_UP) {
            int i = 0;
            int j;
            int flag_1 = 0;
            while (true) {
                if ((*textBuffer)[*cursor - 1] == '\n' && flag_1 == 0) {
                    j = i; 
                    flag_1 = 1;
                } else if ((*textBuffer)[*cursor - 1] == '\n' && flag_1 == 1 || *cursor == 0 && flag_1 == 1) {
                    i = 0;
                    while (i < j) {
                        if ((*textBuffer)[*cursor + 1] == '\n')
                            break;
                        i++;
                        swap(&(*textBuffer)[*cursor + 1], &(*textBuffer)[*cursor]);
                        (*cursor)++;
                    }
                    flag_1 = 0;
                    break;
                } else if (*cursor == 0)
                    break;
                i++;
                swap(&(*textBuffer)[*cursor - 1], &(*textBuffer)[*cursor]);
                (*cursor)--;
            }
        } else if (e->key.keysym.sym == SDLK_DOWN) {
            if (*cursor < *bufferIndex - 1) {
                int currentCol = 0;
                int tempCursor = *cursor;
                while (tempCursor > 0 && (*textBuffer)[tempCursor - 1] != '\n') {
                    tempCursor--;
                    currentCol++;
                }
                while (*cursor < *bufferIndex && (*textBuffer)[*cursor + 1] != '\n') {
                    swap(&(*textBuffer)[*cursor], &(*textBuffer)[*cursor + 1]);
                    (*cursor)++;
                }
                if (*cursor < *bufferIndex - 1) {
                    swap(&(*textBuffer)[*cursor], &(*textBuffer)[*cursor + 1]);
                    (*cursor)++;
                    int movedCol = 0;
                    while (movedCol < currentCol && *cursor < *bufferIndex - 1 && (*textBuffer)[*cursor + 1] != '\n') {
                        swap(&(*textBuffer)[*cursor], &(*textBuffer)[*cursor + 1]);
                        (*cursor)++;
                        movedCol++;
                    }
                }
            }
        } else if ((e->key.keysym.mod & KMOD_CTRL) && e->key.keysym.sym == SDLK_v) {
            char* copied_text = SDL_GetClipboardText();
            char* cleaned_text = malloc(strlen(copied_text) + 1);
            size_t clean_index = 0;
            for (size_t i = 0; i < strlen(copied_text); i++) {
                if (copied_text[i] != '\r') {
                    cleaned_text[clean_index++] = copied_text[i];
                }
            }
            cleaned_text[clean_index] = '\0';
            if (cleaned_text) {
                size_t paste_len = strlen(cleaned_text);
                size_t buffer_len = strlen(*textBuffer);
                while ((*bufferIndex + paste_len) > buffer_size) {
                    buffer_size = buffer_size * GROWTH_FACTOR;
                    *textBuffer = (char*)realloc(*textBuffer, buffer_size);
                    if (*textBuffer == NULL) {
                        perror("realloc failed");
                        free(*textBuffer);
                        fclose(file);
                        exit(1);
                    }
                }
                if ((*bufferIndex + paste_len) < buffer_size) {
                    for (int i = buffer_len; i >= *cursor; i--) {
                        (*textBuffer)[i + paste_len] = (*textBuffer)[i];
                    }
                    for (size_t i = 0; i < paste_len; i++) {
                        (*textBuffer)[*cursor + i] = cleaned_text[i];
                    }
                    *bufferIndex += paste_len;
                    *cursor += paste_len;
                }
                SDL_free(copied_text);
            }
        } else if ((e->key.keysym.mod & KMOD_CTRL) && e->key.keysym.sym == SDLK_c) {
            if (highlight_flag == 1) {
                SDL_SetClipboardText(string_slice(*textBuffer, highlight_start, highlight_end));
            }
        } else if ((e->key.keysym.mod & KMOD_SHIFT) && (e->key.keysym.sym == SDLK_RIGHT)) {
            highlight_flag = 1; 
        } else if (mod & KMOD_SHIFT) {
			printf("\nShift is pressed");
            temp_flag = 0;
            int highlight_anchor;
            if (!highlight_flag) {
                highlight_flag = 1;
                highlight_anchor = *cursor;
                highlight_start = *cursor;
                highlight_end = *cursor;
            }
            if (e->key.keysym.sym == SDLK_LEFT) {
                temp_flag = 1;
                if (*cursor >= 0) {
                    if (*cursor <= highlight_anchor) {
                        highlight_start = *cursor;
                        highlight_end = highlight_anchor + 1;
                    } else {
                        highlight_start = highlight_anchor;
                        highlight_end = *cursor + 1;
                    }
                }
            }
            if (e->key.keysym.sym == SDLK_UP) {
                temp_flag = 1;
                if (*cursor >= 0) {
                    if (*cursor <= highlight_anchor) {
                        highlight_start = *cursor;
                        highlight_end = highlight_anchor + 1;
                    } else {
                        highlight_start = highlight_anchor;
                        highlight_end = *cursor + 1;
                    }
                }
            }
            if (e->key.keysym.sym == SDLK_RIGHT) {
                temp_flag = 1;
                if (*cursor <= *bufferIndex) {
                    if (*cursor >= highlight_anchor) {
                        highlight_start = highlight_anchor;
                        highlight_end = *cursor + 1;
                    } else {
                        highlight_start = *cursor;
                        highlight_end = highlight_anchor + 1;
                    }
                }
            }
            if (e->key.keysym.sym == SDLK_DOWN) {
                temp_flag = 1;
                if (*cursor <= *bufferIndex) {
                    if (*cursor >= highlight_anchor) {
                        highlight_start = highlight_anchor;
                        highlight_end = *cursor + 1;
                    } else {
                        highlight_start = *cursor;
                        highlight_end = highlight_anchor + 1;
                    }
                }
            }
            if (temp_flag) {
                if (*cursor <= highlight_start)
                    highlight_start = highlight_start + 1;
                else if (highlight_end >= *cursor)
                    highlight_end = highlight_end - 1;
            }
        } else if (mod & KMOD_CTRL) {
            if (e->key.keysym.sym == SDLK_a) {
                highlight_flag = 1;
                highlight_start = 0;
                highlight_end = *bufferIndex;
                while (*cursor < *bufferIndex) {
                    swap(&(*textBuffer)[*cursor], &(*textBuffer)[*cursor + 1]);
                    (*cursor)++;
                }
            }
        } else {
            highlight_flag = 0;
        }
    } else if (e->type == SDL_TEXTINPUT) {
        if (*bufferIndex < buffer_size - 1) {
            for (int i = *bufferIndex; i >= *cursor; i--) {
                (*textBuffer)[i + 1] = (*textBuffer)[i];
            }
            (*textBuffer)[*cursor] = *e->text.text;                    
            (*bufferIndex)++;
            (*cursor)++;
        }
    }
}

void render_text(char *textBuffer, int cursor, int bufferIndex, size_t buffer_size) {
    int temp_cursor = cursor;
    int temp_cnt = 0;
    char* tempBuffer = (char*)malloc(buffer_size);

    tempBuffer[0] = '\0';
    while (temp_cursor >= 0 && temp_cnt < 50) {
        if (textBuffer[temp_cursor] == '\n') {
            temp_cnt++;
        }
        temp_cursor--;
    }
    int start_pos = temp_cursor + 1;
    temp_cursor = cursor;
    temp_cnt = 0;
    while (temp_cursor <= bufferIndex && temp_cnt < 50) {
        if (textBuffer[temp_cursor] == '\n') {
            temp_cnt++;
        }
        temp_cursor++;
    }
    int end_pos = temp_cursor;
    int length = end_pos - start_pos;
    if (length > 0) {
        strncpy(tempBuffer, textBuffer + start_pos, length);
        tempBuffer[length] = '\0';
    } else {
        printf("Error: Buffer size exceeded or invalid length.\n");
    }

    char* token;
    char* str = tempBuffer; 
    int y_off = 0;
    int tokenCnt = 0;
    int highlight_text_index = 0;
    int totalCharsProcessed = 0;

    draw_cursor();
    while ((token = strsep(&str, "\n")) != NULL) {
        tokenCnt++;
        int lineLength = strlen(token);
        char* textToRender = strlen(token) == 0 ? " " : token;
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, textToRender, textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture) {
                SDL_Rect textRect = {0, y_off + render_y_off, textSurface->w, textSurface->h};
                y_off += textSurface->h;
                if (highlight_flag == 1) {
                    int lineStartIdx = totalCharsProcessed;
                    int lineEndIdx = lineStartIdx + lineLength;
                    if (highlight_start < lineEndIdx && highlight_end > lineStartIdx) {
                        int highlightBegin = (highlight_start > lineStartIdx) ? (highlight_start - lineStartIdx) : 0;
                        int highlightEndIdx = (highlight_end < lineEndIdx) ? (highlight_end - lineStartIdx) : lineLength;
                        if (highlightBegin < highlightEndIdx) {
                            int preHighlightWidth;
                            char* preHighlightText = malloc(highlightBegin + 1);
                            strncpy(preHighlightText, textToRender, highlightBegin);
                            preHighlightText[highlightBegin] = '\0';
                            TTF_SizeText(font, preHighlightText, &preHighlightWidth, NULL);
                            free(preHighlightText);

                            int highlightWidth;
                            int highlightLength = highlightEndIdx - highlightBegin;
                            char* highlightText = malloc(highlightLength + 1);
                            strncpy(highlightText, textToRender + highlightBegin, highlightLength);
                            highlightText[highlightLength] = '\0';
                            TTF_SizeText(font, highlightText, &highlightWidth, NULL);
                            free(highlightText);

                            SDL_Rect highlightRect = {textRect.x + preHighlightWidth, textRect.y, highlightWidth, textRect.h};
                            SDL_SetRenderDrawColor(renderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
                            SDL_RenderFillRect(renderer, &highlightRect);
                        }
                    }
                }
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
        currentTextBlockHeight = y_off;
        totalCharsProcessed += lineLength + 1;
    }
    draw_menu_bar();
    line_number = tokenCnt;
    SDL_RenderPresent(renderer);
    free(tempBuffer);
}

int main(int argc, char* argv[]) {
    size_t buffer_size = INITIAL_SIZE;
    char* textBuffer = (char*)malloc(buffer_size); // Buffer to store user input
    if (textBuffer == NULL) {
        perror("Initializing buffer failed!");
        return 1;
    }
    textBuffer[0] = '\0';

    char* tempBuffer = (char*)malloc(buffer_size);
    if (tempBuffer == NULL) {
        perror("Initializing temp buffer failed!");
        free(textBuffer);
        return 1;
    }

    int quit = 0;
    int cursor = 0;
    int bufferIndex = 0;
    FILE *file = NULL;

    if (argc == 2) {
        char *filename = argv[1];  // Get the filename from command-line arguments
        file = fopen(filename, "r+");  // Open the file in read mode

        if (file == NULL) {
            perror("Error opening file");
            free(textBuffer);
            free(tempBuffer);
            return 1;
        }

        size_t bytesRead;
        while ((bytesRead = fread(textBuffer + bufferIndex, 1, buffer_size - bufferIndex, file)) > 0) {
            bufferIndex += bytesRead;

            // If we exceed the current buffer size, grow the buffer
            if (bufferIndex >= buffer_size) {
                buffer_size *= GROWTH_FACTOR; // Increase buffer size
                textBuffer = (char*)realloc(textBuffer, buffer_size);
                tempBuffer = (char*)realloc(tempBuffer, buffer_size);
                if (textBuffer == NULL || tempBuffer == NULL) {
                    perror("realloc failed");
                    fclose(file);
                    return 1;
                }
                printf("Buffer size updated: %zu\n", buffer_size);
            }
        }
        cursor = bufferIndex;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        free(textBuffer);
        free(tempBuffer);
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        free(textBuffer);
        free(tempBuffer);
        return 1;
    }

    window = SDL_CreateWindow("Text Editor",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        free(textBuffer);
        free(tempBuffer);
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        free(textBuffer);
        free(tempBuffer);
        return 1;
    }

    font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 16);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        free(textBuffer);
        free(tempBuffer);
        return 1;
    }

    SDL_StartTextInput();

    Uint32 cursorBlinkTime = SDL_GetTicks(); // Cursor Blink time 
    int showCursor = 1;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    while (!quit) {
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                if (argc == 2) {
                    save_file(file, textBuffer, bufferIndex);
                }
                quit = 1;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_GetWindowSize(window, &netWidth, &netHeight);
                    SDL_Log("Window resized to %d x %d", netWidth, netHeight);
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                if ((e.wheel.y > 0 || currentTextBlockHeight + render_y_off > netHeight) && (render_y_off < 25 || e.wheel.y < 0)) {
                    SDL_Event event;
                    if (e.wheel.y == 1) {
                        event.type = SDL_KEYDOWN;
                        event.key.keysym.sym = SDLK_DOWN;
                        SDL_PushEvent(&event);
                    } else if (e.wheel.y == -1) {
                        event.type = SDL_KEYDOWN;
                        event.key.keysym.sym = SDLK_UP;
                        SDL_PushEvent(&event);
                    }
                    scroll_count += e.wheel.y;
                    render_y_off += (e.wheel.y * TTF_FontHeight(font));
                }
                if (render_y_off > 25) render_y_off = 25;
            } else {
                handle_input(&e, &textBuffer, &bufferIndex, &cursor, buffer_size, file);
            }
        }

        if (SDL_GetTicks() - cursorBlinkTime >= 500) {
            showCursor = !showCursor;
            cursorBlinkTime = SDL_GetTicks();
        }

        if (showCursor) {
            textBuffer[cursor] = '|';
        } else {
            textBuffer[cursor] = ' ';
        }

        cursor_line = 0;
        for (int i = cursor; i >= 0; i--) {
            if (textBuffer[i] == '\n')
                cursor_line++;
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
        SDL_RenderClear(renderer);

        render_text(textBuffer, cursor, bufferIndex, buffer_size);
    }

    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    free(textBuffer);
    free(tempBuffer);
    return 0;
}