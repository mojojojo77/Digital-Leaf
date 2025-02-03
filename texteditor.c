#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "lib\tinyfiledialogs\tinyfiledialogs.h"

#define INITIAL_SIZE 256
#define GROWTH_FACTOR 2

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
TTF_Font* font_menu = NULL;

// Buffer and cursor state
char* textBuffer = NULL;
char* tempBuffer = NULL;
int* lineNumbers = NULL;
int cursor = 0;
int bufferIndex = 0;
int render_y_off = 25;  // Initialize to 25 as that's the menu bar height
int cursor_highlight_start = 0;
int netWidth = 0, netHeight = 0;

// Line tracking
int line_number = 0;
int cursor_line = 0;
int current_cursor_line = 0;
int current_cursor_char = 0;
int linesToDisplay = 0;
int total_lines = 0;
int x = 0;  // Cursor x position

// Scroll state
int currentTextBlockHeight = 0;
int scroll_count = 0;
int scroll_drag_offset = 0;
bool scrollbar_clicked_flag = false;

// Cursor highlight
int highlight_start = 0;
int highlight_end = 0;
int highlight_anchor = 0;
bool highlight_flag = false;

// Menu dimensions
const int FILE_ITEM_HEIGHT = 20;
const int FILE_ITEM_WIDTH = 40;
const int THEME_ITEM_HEIGHT = 20;
const int THEME_ITEM_WIDTH = 40;

// Menu state
bool file_item_drop_down_flag = false;
bool themes_item_drop_down_flag = false;

// Font settings
int current_font_size = 16;

// Temporary flags
int temp_flag = 0;
int t_flag = 0;

// Color variables for themes
SDL_Color background_color;
SDL_Color highlightColor;
SDL_Color text_color;
SDL_Color drawertext_color;
SDL_Color menu_bar_color;
SDL_Color scroll_bar_color;
SDL_Color text_select_color;
SDL_Color dropdown_color;
SDL_Color menubar_item_font_color;
SDL_Color menu_bar_item_color;
SDL_Color cursor_color;
SDL_Color horizontal_line_below_text_in_menu_color;
SDL_Color horizontal_line_below_text_in_menu_color_bold;
SDL_Color notification_background_color;
SDL_Color notification_text_color;

// File state
bool fileSaved = false;

// Viewport management
int virtual_cursor_line = 0;
int viewport_top_line = 0;
int viewport_bottom_line = 0;

// Global buffer size variable
int buffer_size = INITIAL_SIZE;

// Manual scrollbar dragging flag
bool manual_scrollbar_drag = false;
int manual_scroll_mouseY = 0;

// Function prototypes
void theme_tiles();
void theme_wood();
void theme_mountain();
void theme_bubblegum();
void theme_original();
void theme_obsidian();
void theme_forest();
void drawscroll(int scroll_y);
void drawcursor();
void drawMenuBar();
void draw_file_dropdown();
void drawThemesBar();
void draw_themes_dropddown();
void showNotification(SDL_Renderer *renderer, TTF_Font *font, const char *message);
void ensure_cursor_visible();
void update_viewport();
void manage_buffer_size();
void manage_scroll();

// SDL Clickable regions 
SDL_Rect fileItem = {0,0,0,0}; 
SDL_Rect themeItem = {0,0,0,0};
SDL_Rect scrollbar = {0,0,0,0};

SDL_Rect fileItem_DROP_DOWN = {0,0,0,0};
SDL_Rect themeItem_DROP_DOWN = {0,0,0,0};

SDL_Rect textRect;

// Scrollbar variables
bool scrollbar_flag = false;
int scroll_y_pos;
int scroll_offset = 0;
bool is_scrolling = false;

// Mouse functionality variables	
bool isDragging = false;
int mouseX; 
int mouseY;
int temp_y; 

// Is mouse over a certain region (SDL_Rect)
bool isMouseOver(SDL_Rect rect, int mouseX, int mouseY) {
	
    return mouseX >= rect.x && mouseX <= rect.x + rect.w &&
           mouseY >= rect.y && mouseY <= rect.y + rect.h;
}

bool isClickedOn(SDL_Rect rect, int mouseX, int mouseY){
    return mouseX >= rect.x && mouseX <= rect.x + rect.w &&
           mouseY >= rect.y && mouseY <= rect.y + rect.h;	
}

// Check if the pressed key is alphanumeric or a special character
int is_alnum_or_special(SDL_Keycode key) {
    // Check if the key is alphanumeric
    if (isalnum(key)) {
        return 1;
    }
    // Check if the key is a printable special character
    if (ispunct(key)) {
        return 1;
    }
    return 0;
}

// Easy Swap Function 
void swap(char *a, char *b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

// String slice function to implement the copy to clipboard 
char* string_slice(const char* input, size_t start, size_t end) {
    // Validate input
    if (!input) {
        printf("ERROR: Null input to string_slice\n");
        return NULL;
    }

    size_t input_len = strlen(input);
    
    // Validate and adjust start and end indices
    if (start >= input_len) {
        printf("ERROR: Start index %d out of bounds (input length %d)\n", start, input_len);
        return NULL;
    }
    
    // Adjust end index if it's out of bounds
    if (end > input_len) {
        printf("WARNING: Adjusting end index from %d to %d\n", end, input_len);
        end = input_len;
    }
    
    // Ensure start is less than end
    if (start >= end) {
        printf("ERROR: Invalid slice range (start %d, end %d)\n", start, end);
        return NULL;
    }
    
    // Calculate length and check for extreme sizes
    size_t len = end - start;
    if (len > 10 * 1024 * 1024) {  // Limit to 10MB
        printf("ERROR: Slice too large: %d bytes\n", len);
        return NULL;
    }

    // Allocate memory with error checking
    char* result = (char*)malloc(len + 1);
    if (!result) {
        printf("ERROR: Memory allocation failed for %d bytes\n", len + 1);
        return NULL;
    }
    
    // Safe copy
    strncpy(result, input + start, len);
    result[len] = '\0';
    return result;
}

// String tokenization function used to render newlines, especially used instead of strtok to implement empty tokens as well 
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

// File path or file name
char* filename = NULL;
FILE *file = NULL;

// Virtual Cursor 
int virtual_cursor; 
int virtual_cursor_line;

// Drawer Flags 
bool new_drawer_flag = false;
bool open_drawer_flag = false;
bool save_drawer_flag = false;
bool saveas_drawer_flag = false;
bool exit_drawer_flag = false;

bool new_drawer_flag_clicked = false;
bool open_drawer_flag_clicked = false;
bool save_drawer_flag_clicked = false;
bool saveas_drawer_flag_clicked = false;
bool exit_drawer_flag_clicked = false;

bool theme_drawer_forest_flag = false;
bool theme_drawer_mountain_flag = false;
bool theme_drawer_bubblegum_flag = false;
bool theme_drawer_wood_flag = false;
bool theme_drawer_tiles_flag = false;
bool theme_drawer_obsidian_flag = false;

bool theme_drawer_forest_flag_clicked = false;
bool theme_drawer_mountain_flag_clicked = false;
bool theme_drawer_bubblegum_flag_clicked = false;
bool theme_drawer_wood_flag_clicked = false;
bool theme_drawer_tiles_flag_clicked = false;
bool theme_drawer_obsidian_flag_clicked = false;

bool mouse_clicked_flag = false;

// Notification
char* notificationMessage;
bool notificationFlag;
clock_t notifStartTime;

// Theme
int theme = 2; 

void theme_tiles(){
background_color = (SDL_Color){240, 240, 240, 250}; // Light cool gray
highlightColor = (SDL_Color){180, 180, 190, 90}; // Soft stone gray
text_color = (SDL_Color){30, 30, 40, 255}; // Deep charcoal
drawertext_color = (SDL_Color){50, 50, 60, 250}; // Dark gray
menu_bar_color = (SDL_Color){220, 220, 230, 255}; // Pale cool gray
scroll_bar_color = (SDL_Color){160, 160, 170, 255}; // Medium stone gray
text_select_color = (SDL_Color){200, 200, 210, 255}; // Light slate gray
dropdown_color = (SDL_Color){250, 250, 255, 255}; // Ultra-light gray-white
menubar_item_font_color = (SDL_Color){20, 20, 30, 255}; // Near-black
menu_bar_item_color = (SDL_Color){230, 230, 240, 255}; // Soft gray
cursor_color = (SDL_Color){170, 170, 180, 100}; // Translucent stone gray
horizontal_line_below_text_in_menu_color = (SDL_Color){190, 190, 200, 50}; // Pale gray
horizontal_line_below_text_in_menu_color_bold = (SDL_Color){170, 170, 180, 250}; // Medium gray
notification_background_color = (SDL_Color){200, 200, 210, 150}; // Visible gray
notification_text_color = (SDL_Color){20, 20, 30, 250}; // Near-black
}

void theme_wood(){
background_color = (SDL_Color){45, 35, 25, 250}; // Deep rich brown
highlightColor = (SDL_Color){120, 90, 60, 90}; // Soft wood grain brown
text_color = (SDL_Color){240, 230, 210, 255}; // Warm cream
drawertext_color = (SDL_Color){220, 210, 190, 255}; // Light beige
menu_bar_color = (SDL_Color){35, 25, 20, 255}; // Dark espresso brown
scroll_bar_color = (SDL_Color){100, 70, 50, 255}; // Warm cedar brown
text_select_color = (SDL_Color){80, 60, 40, 255}; // Dark wood tone
dropdown_color = (SDL_Color){30, 20, 15, 255}; // Almost black brown
menubar_item_font_color = (SDL_Color){250, 240, 220, 255}; // Warm ivory
menu_bar_item_color = (SDL_Color){25, 20, 15, 255}; // Very dark brown
cursor_color = (SDL_Color){110, 80, 60, 100}; // Translucent wood brown
horizontal_line_below_text_in_menu_color = (SDL_Color){90, 70, 50, 50}; // Soft wood grain
horizontal_line_below_text_in_menu_color_bold = (SDL_Color){120, 90, 70, 250}; // Rich wood tone
notification_background_color = (SDL_Color){70, 50, 40, 150}; // Deep brown
notification_text_color = (SDL_Color){240, 230, 210, 250};     // Warm cream		
}

void theme_mountain(){
background_color = (SDL_Color){240, 248, 255, 250}; // Bright, pure icy white
highlightColor = (SDL_Color){135, 206, 235, 90}; // Soft sky blue with transparency
text_color = (SDL_Color){30, 30, 50, 255}; // Deep navy for contrast
drawertext_color = (SDL_Color){50, 50, 70, 250}; // Dark slate gray
menu_bar_color = (SDL_Color){200, 230, 255, 255}; // Pale ice blue
scroll_bar_color = (SDL_Color){100, 170, 220, 255}; // Bright winter blue
text_select_color = (SDL_Color){150, 200, 230, 255}; // Soft ice blue
dropdown_color = (SDL_Color){230, 240, 255, 255}; // Very light blue-white
menubar_item_font_color = (SDL_Color){20, 20, 40, 255}; // Near-black
menu_bar_item_color = (SDL_Color){210, 230, 255, 255}; // Pale ice blue
cursor_color = (SDL_Color){100, 180, 220, 100}; // Translucent bright blue
horizontal_line_below_text_in_menu_color = (SDL_Color){180, 210, 240, 50}; // Pale blue
horizontal_line_below_text_in_menu_color_bold = (SDL_Color){150, 190, 230, 250}; // Bright ice blue
notification_background_color = (SDL_Color){170, 210, 240, 150}; // Vibrant ice blue
notification_text_color = (SDL_Color){20, 20, 40, 250}; // Near-black
}

void theme_bubblegum(){
background_color = (SDL_Color){255, 200, 230, 250}; // Soft pastel pink
highlightColor = (SDL_Color){255, 150, 200, 90}; // Translucent bright pink
text_color = (SDL_Color){30, 30, 30, 255}; // Near-black for contrast
drawertext_color = (SDL_Color){50, 50, 50, 250}; // Dark gray
menu_bar_color = (SDL_Color){255, 170, 210, 255}; // Bright pink
scroll_bar_color = (SDL_Color){255, 120, 180, 255}; // Vibrant pink
text_select_color = (SDL_Color){255, 100, 160, 255}; // Saturated pink
dropdown_color = (SDL_Color){255, 230, 240, 255}; // Very light pink
menubar_item_font_color = (SDL_Color){20, 20, 20, 255}; // Almost black
menu_bar_item_color = (SDL_Color){255, 190, 220, 255}; // Soft pink
cursor_color = (SDL_Color){255, 140, 190, 100}; // Translucent bright pink
horizontal_line_below_text_in_menu_color = (SDL_Color){255, 200, 230, 50}; // Pale pink
horizontal_line_below_text_in_menu_color_bold = (SDL_Color){255, 170, 210, 250}; // Bold pink
notification_background_color = (SDL_Color){255, 220, 240, 50}; // Very light, translucent pink
notification_text_color = (SDL_Color){250, 250, 250, 250}; // Very light gray
}

void theme_original(){
background_color = (SDL_Color){0,0,0,250};
highlightColor = (SDL_Color){33, 150, 243, 0.35*255}; //{}

text_color = (SDL_Color){255,255,255,255};
drawertext_color = (SDL_Color){250,250,250,250};

menu_bar_color = (SDL_Color){27, 40, 48, 255};
scroll_bar_color = (SDL_Color){0, 150, 150, 255};
text_select_color = (SDL_Color){80,80,80,255	};

dropdown_color = (SDL_Color){3, 20, 31, 255};

menubar_item_font_color = (SDL_Color){255, 255, 255, 255};
menu_bar_item_color = (SDL_Color){3, 20, 31, 255};
cursor_color = (SDL_Color){0, 150, 150, 100};

horizontal_line_below_text_in_menu_color = (SDL_Color){250,250,250,50};
horizontal_line_below_text_in_menu_color_bold = (SDL_Color){250,250,250,250};

notification_background_color = (SDL_Color){150,150,150,50};
notification_text_color = (SDL_Color){250,250,250,250};	
}

// Obsidian/Original Theme
void theme_obsidian(){
background_color = (SDL_Color){12, 12, 12, 255};           // Deep black with a hint of gray
highlightColor = (SDL_Color){76, 76, 102, 180};           // Subtle blue-gray with slight transparency

text_color = (SDL_Color){220, 220, 220, 255};             // Light gray for good readability
drawertext_color = (SDL_Color){200, 200, 200, 255};       // Slightly darker gray for drawer text

menu_bar_color = (SDL_Color){20, 20, 20, 255};            // Dark gray for the menu bar
scroll_bar_color = (SDL_Color){45, 45, 45, 255};          // Medium gray for the scrollbar
text_select_color = (SDL_Color){100, 100, 140, 100};      // Soft purple-gray for text selection

dropdown_color = (SDL_Color){25, 25, 30, 255};            // Near-black with a hint of blue for dropdowns

menubar_item_font_color = (SDL_Color){220, 220, 220, 255}; // Same light gray as text for menu items
menu_bar_item_color = (SDL_Color){30, 30, 35, 255};        // Slightly lighter gray than the menu bar
cursor_color = (SDL_Color){76, 76, 102, 200};              // Blue-gray with transparency for the cursor

horizontal_line_below_text_in_menu_color = (SDL_Color){76, 76, 102, 120};       // Faint blue-gray line
horizontal_line_below_text_in_menu_color_bold = (SDL_Color){100, 100, 140, 255}; // Bold purple-gray line

notification_background_color = (SDL_Color){35, 35, 40, 200};  // Subtle gray-blue background with transparency
notification_text_color = (SDL_Color){230, 230, 230, 255};     // Very light gray for visibility
}

// Forest Theme
void theme_forest(){
background_color = (SDL_Color){15, 33, 25, 255};  // Deep forest green
highlightColor = (SDL_Color){76, 175, 80, 128};  // Soft moss green with transparency
text_color = (SDL_Color){240, 245, 235, 255};    // Pale, almost white green
drawertext_color = (SDL_Color){200, 220, 200, 255}; // Light sage green
menu_bar_color = (SDL_Color){25, 45, 35, 255};   // Dark forest green
scroll_bar_color = (SDL_Color){50, 120, 90, 255}; // Rich green
text_select_color = (SDL_Color){100, 180, 130, 150}; // Transparent green highlight
dropdown_color = (SDL_Color){20, 40, 30, 255};   // Very dark forest green
menubar_item_font_color = (SDL_Color){230, 240, 225, 255}; // Pale green-white
menu_bar_item_color = (SDL_Color){30, 50, 40, 255}; // Dark green
cursor_color = (SDL_Color){50, 200, 120, 100};   // Bright, transparent green
horizontal_line_below_text_in_menu_color = (SDL_Color){100, 150, 120, 50}; // Muted forest green
horizontal_line_below_text_in_menu_color_bold = (SDL_Color){150, 200, 160, 250}; // Brighter forest green
notification_background_color = (SDL_Color){40, 70, 50, 50}; // Very dark, transparent forest green
notification_text_color = (SDL_Color){220, 235, 215, 250}; // Pale green-white
}

int main(int argc, char* argv[]) {
	
	// Use global buffer_size instead of local declaration
	buffer_size = INITIAL_SIZE;
    
    // Initialize buffers using calloc for zero-initialization
    textBuffer = (char*)calloc(buffer_size, sizeof(char));
    tempBuffer = (char*)calloc(buffer_size, sizeof(char));
    lineNumbers = (int*)calloc(buffer_size, sizeof(int));

    // Proper error checking for all buffer allocations
    if (textBuffer == NULL || tempBuffer == NULL || lineNumbers == NULL) {
        perror("Failed to allocate buffers");
        // Clean up any successfully allocated buffers
        if (textBuffer) free(textBuffer);
        if (tempBuffer) free(tempBuffer);
        if (lineNumbers) free(lineNumbers);
        return 1;
    }

    // Initialize theme before SDL setup
    theme_mountain(); // Set default theme

    int quit = 0;
	int tmpBufferIndex = 0;
	
	textBuffer[0] = '\0';
		
	if (argc == 2){
		filename = argv[1];  // Get the filename from command-line arguments
		file = fopen(filename, "r+");  // Open the file in read mode

		if (file == NULL) {
			perror("Error opening file");
			free(textBuffer);
			free(tempBuffer);
			free(lineNumbers);
			return 1;
		}

		size_t bytesRead;
		while ((bytesRead = fread(textBuffer + bufferIndex, 1, buffer_size - bufferIndex - 1, file)) > 0) {
			bufferIndex += bytesRead;

			// If we're close to buffer capacity, grow it
			if (bufferIndex >= buffer_size - 1) {
				buffer_size *= GROWTH_FACTOR;
				char* new_textBuffer = (char*)realloc(textBuffer, buffer_size);
				char* new_tempBuffer = (char*)realloc(tempBuffer, buffer_size);
				int* new_lineNumbers = (int*)realloc(lineNumbers, buffer_size);

				if (new_textBuffer == NULL || new_tempBuffer == NULL || new_lineNumbers == NULL) {
					perror("Failed to reallocate buffers");
					free(textBuffer);
					free(tempBuffer);
					free(lineNumbers);
					fclose(file);
					return 1;
				}

				textBuffer = new_textBuffer;
				tempBuffer = new_tempBuffer;
				lineNumbers = new_lineNumbers;
				printf("Buffer size updated: %d\n", buffer_size);
			}
		}
		
		textBuffer[bufferIndex] = '\0';  // Ensure null termination
		cursor = bufferIndex;
	}
	
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("Text Editor",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
	SDL_Surface *icon = IMG_Load("icon.png");

	if (!icon) {
			printf("Could not load icon! SDL_Error: %s\n", SDL_GetError());
		} else {
			// Set the window icon
			SDL_SetWindowIcon(window, icon);
			SDL_FreeSurface(icon);
		}

	SDL_SetWindowOpacity(window, 1); 

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED| SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    font = TTF_OpenFont("fonts/MonaspaceKryptonFrozen-Regular.ttf", 16);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawColor(renderer, text_color.r, text_color.g, text_color.b, text_color.a); // Black color
    SDL_StartTextInput();

	Uint32 cursorBlinkTime = SDL_GetTicks(); // Cursor Blink time 
	int showCursor = 1;
	
	
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	
//	SDL_SetWindowOpacity(window, 0.9f);

/***	// Helper keypress event to adjust the aligning of the highlight 
    SDL_Event keyEvent;
    keyEvent.type = SDL_KEYDOWN;                // Key press event
    keyEvent.key.keysym.sym = SDLK_RIGHT;           // The key (e.g., 'A')
    keyEvent.key.keysym.mod = 1;                // modifiers (e.g., Shift, Ctrl)
    keyEvent.key.repeat = 0;                    // Not a repeated key press
***/

	while (!quit) {
		viewport_top_line = virtual_cursor_line - linesToDisplay / 2;
		viewport_bottom_line = virtual_cursor_line + linesToDisplay / 2;

/*** 
		printf("\n Cursor Line: %d", cursor_line);
		printf("\n Viewport Top Line: %d", viewport_top_line);
		printf("\n Viewport Bottom Line: %d", viewport_bottom_line);
		system("cls");
***/
		switch(theme){
			case 1: theme_forest();break;
			case 2:	theme_mountain();break;
			case 3:	theme_bubblegum();break;
			case 4: theme_wood();break;
			case 5: theme_tiles();break;
			case 6: theme_obsidian();break;
			default: 
				theme_mountain();
				//theme_original();
				break;
		}

        SDL_Event e;
		
        // Handle events
        while (SDL_PollEvent(&e)) {
			SDL_Keymod mod = SDL_GetModState();

            if (e.type == SDL_QUIT){
				if(argc == 2){
//					printf("\n %d",bufferIndex);

					ftruncate(fileno(file), 0);
					rewind(file);
					
					if (file) {
						memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
						cursor = bufferIndex;
						
						int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
						fflush(file);
						if (result < 0) {
							printf("Error while saving file");
						}
						fclose(file);
					}
					
					free(textBuffer);
					free(tempBuffer);
				}
                quit = 1;
            } else if (e.type == SDL_WINDOWEVENT) {
					// Handle window resize event
					if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						// Get the new window dimensions						
						SDL_GetWindowSize(window, &netWidth, &netHeight);
						// You can now use the new width and height for rendering or layout adjustments
						SDL_Log("Window resized to %d x %d", netWidth, netHeight);
					}
			} else if (e.type == SDL_MOUSEBUTTONDOWN) {
				
				fileSaved = false;
				mouseX = e.button.x;
				mouseY = e.button.y;
				
//				*x = mouseX; 
//				*y = mouseY;							
								
				if(new_drawer_flag)new_drawer_flag_clicked = true; 
				else if(open_drawer_flag)open_drawer_flag_clicked = true;
				else if(save_drawer_flag)save_drawer_flag_clicked = true;
				else if(saveas_drawer_flag)saveas_drawer_flag_clicked = true;
				else if(exit_drawer_flag)exit_drawer_flag_clicked = true;
				
				if(theme_drawer_forest_flag)theme_drawer_forest_flag_clicked = true;
				else if(theme_drawer_mountain_flag)theme_drawer_mountain_flag_clicked = true;
				else if(theme_drawer_bubblegum_flag)theme_drawer_bubblegum_flag_clicked = true;
				else if(theme_drawer_wood_flag)theme_drawer_wood_flag_clicked = true;
				else if(theme_drawer_tiles_flag)theme_drawer_tiles_flag_clicked = true;
				else if(theme_drawer_obsidian_flag)theme_drawer_obsidian_flag_clicked = true;


				if(highlight_flag)
					highlight_flag = 0;
				
				printf("\n Button Pressed");
				if(e.button.button == SDL_BUTTON_LEFT){
					mouse_clicked_flag = true;
					printf("%d", mouse_clicked_flag);

					if(isClickedOn(scrollbar, mouseX, mouseY)){
						scrollbar_clicked_flag = true;
					}

				if(!isMouseOver(scrollbar, mouseX, mouseY)){

				// Check if the click occurred within the clickable region
					
//						printf("\n JUMP HEREEEREEEEE!");
					scroll_offset = mouseY - scrollbar.y;
					isDragging = true;
					
					if(mouseY > 25){
						// Adjust mouse Y position by scroll offset for accurate cursor positioning
						int adjusted_mouseY = mouseY - render_y_off;
						current_cursor_line = (adjusted_mouseY) / TTF_FontHeight(font);
						current_cursor_line += viewport_top_line;
						
						// Add scroll offset in lines
						//current_cursor_line += (-render_y_off / TTF_FontHeight(font));
						
						// Ensure cursor line doesn't go negative
						if(current_cursor_line < 0) current_cursor_line = 0;
						
						current_cursor_char = floor(mouseX/9.5); // Change the approx text width value to a variable which can change as the font size changes
	//					printf("\n Current Cursor: %d , %d",current_cursor_line, current_cursor_char);
						
						int last_line;
						int temp = 0;
						int total_lines = 0;
						int total_cursor_character = 0;
						int flag = true;
						
						for(int i=0; i<bufferIndex; i++){
							if(textBuffer[i] == '\n'){
								total_lines++;
								last_line = i;
							}
						}
						
						if(total_lines == 0)
							last_line = 0;
						//printf("\n %d", last_line);
						
						if(current_cursor_line <= total_lines){
							if(current_cursor_line == total_lines && bufferIndex - last_line < current_cursor_char){
								goto jump_here_one;
							}
						// Shift the cursor to the mouse position when clicked 								
							else{
								for(int i=0; i<bufferIndex; i++){
									if(textBuffer[i] == '\n'){
										temp++; 
									//	printf("\n Current Cursor: %d , %d",current_cursor_line, temp);
									}

									if(temp == current_cursor_line){
										for(int j = current_cursor_char; j != '\n' && j < 0; j--){
											if(textBuffer[i+j] == '\n'){
												current_cursor_char = j;
												total_cursor_character += current_cursor_char;
												break;
											}
										}
										memmove(&textBuffer[cursor], &textBuffer[cursor+1], bufferIndex - cursor + 1);
										memmove(&textBuffer[i+(current_cursor_char)+1], &textBuffer[i +(current_cursor_char)], bufferIndex - i +(current_cursor_char));
										cursor = i+(current_cursor_char);
										break;
									}
								}
								
								// If mouse is clicked outside of text, recaliberate it to the end of the current line
								cursor_line = 0;
								
								for(int i=cursor; i >= 0; i--){
									if(textBuffer[i] == '\n')
										cursor_line++;
								}
								
								//printf("\n %d, %d", current_cursor_line, cursor_line);

								while(cursor_line > current_cursor_line){
									//printf("\n %d", cursor_line);
									while(textBuffer[cursor+1] != '\n'){
										swap(&textBuffer[cursor], &textBuffer[cursor-1]);
										cursor--;
									}
									swap(&textBuffer[cursor], &textBuffer[cursor-1]);
									cursor--;
									cursor_line--;
									if(cursor_line == current_cursor_line){
										swap(&textBuffer[cursor], &textBuffer[cursor+1]);
										cursor++;
									} 
								}
							}
						}
						else{
							jump_here_one:
								memmove(&textBuffer[cursor], &textBuffer[cursor+1], bufferIndex - cursor + 1);
								cursor = bufferIndex;
						}
					}
					
															
					if ((isMouseOver(fileItem, mouseX, mouseY) && file_item_drop_down_flag == false)) {
						file_item_drop_down_flag = true;
						themes_item_drop_down_flag = false; // Close other dropdowns
					}
					else if ((file_item_drop_down_flag == true)) {
						// Only close if not over dropdown area
						if (!isMouseOver(fileItem_DROP_DOWN, mouseX, mouseY)) {
							file_item_drop_down_flag = false;
						}
					}

					if ((isMouseOver(themeItem, mouseX, mouseY) && themes_item_drop_down_flag == false)) {
						themes_item_drop_down_flag = true;
						file_item_drop_down_flag = false; // Close other dropdowns
					}
					else if ((themes_item_drop_down_flag == true)) {
						// Only close if not over dropdown area
						if (!isMouseOver(themeItem_DROP_DOWN, mouseX, mouseY)) {
							themes_item_drop_down_flag = false;
						}
					}
					
					//if ((!isMouseOver(themeItem, mouseX, mouseY) && themes_item_drop_down_flag == true)) themes_item_drop_down_flag = false;

					//else themes_item_drop_down_flag = false;	
				}
				
				if(file_item_drop_down_flag && !isMouseOver(fileItem_DROP_DOWN, mouseX, mouseY)){
					new_drawer_flag_clicked = false;
					open_drawer_flag_clicked = false;
					save_drawer_flag_clicked = false;
					saveas_drawer_flag_clicked = false;
					exit_drawer_flag_clicked = false;
				}
				
				if(themes_item_drop_down_flag && !isMouseOver(themeItem_DROP_DOWN, mouseX, mouseY)){
					theme_drawer_forest_flag_clicked = false;
					theme_drawer_mountain_flag_clicked = false;
					theme_drawer_bubblegum_flag_clicked = false;
					theme_drawer_wood_flag_clicked = false;
					theme_drawer_tiles_flag_clicked = false;
					theme_drawer_obsidian_flag_clicked = false;
				}
			}					
			} else if(e.type == SDL_MOUSEMOTION){
				mouseX = e.button.x;
				mouseY = e.button.y;

				if(!isMouseOver(scrollbar, mouseX, mouseY)){		
				
				if(scrollbar_flag == true){
					is_scrolling = true;
				}
				
				// Mouse over file menu condition 
				if(e.button.button == SDL_BUTTON_LEFT){
					if(scrollbar_flag){
						scroll_y_pos =  e.motion.y;
						//printf("\n%d",scroll_initial_pos);
						//printf("\n HEREEEREEEEE!");
						
						if(scroll_y_pos < 25) scroll_y_pos = 25;
						if(scroll_y_pos > netHeight - scrollbar.h - 25) 
							scroll_y_pos = netHeight - scrollbar.h - 25;

						// Calculate scroll progress
						float scroll_progress = (float)(scroll_y_pos - 25) / 
												(netHeight - scrollbar.h - 50);

						// Calculate total number of lines in the document
						int total_lines = 0;
						for (int i = 0; i < bufferIndex; i++) {
							if (textBuffer[i] == '\n') {
								total_lines++;
							}
						}

						// Update virtual cursor line based on scroll progress
						virtual_cursor_line = (int)(scroll_progress * total_lines);

						// Find character index for the calculated line
						int current_line = 0;
						int char_index = 0;
						while (char_index < bufferIndex && current_line < virtual_cursor_line) {
							if (textBuffer[char_index] == '\n') {
								current_line++;
							}
							char_index++;
						}

						// Update virtual cursor and scrollbar position
						virtual_cursor = char_index;
						scrollbar.y = scroll_y_pos;

						// Adjust render offset to match virtual cursor
						render_y_off = -virtual_cursor_line * TTF_FontHeight(font);

						// Force viewport update
						update_viewport();
					}
					if(isDragging == true && (e.motion.x > 0 && e.motion.y > 25) && scrollbar_flag == false){

							
						is_scrolling = true;
						//if(e.button.button == SDL_BUTTON_LEFT){
						// Check if the click occurred within the clickable region
							if(mouseY > 25){

								// Adjust mouse Y position by scroll offset for accurate cursor positioning
								int adjusted_mouseY = mouseY - render_y_off;
								current_cursor_line = (adjusted_mouseY) / TTF_FontHeight(font);
								current_cursor_line += viewport_top_line;
								// Add scroll offset in lines
								//current_cursor_line += (-render_y_off / TTF_FontHeight(font));
								
								// Ensure cursor line doesn't go negative
								if(current_cursor_line < 0) current_cursor_line = 0;
								
								current_cursor_char = floor(mouseX/9.5); // Change the approx text width value to a variable which can change as the font size changes
			//					printf("\n Current Cursor: %d , %d",current_cursor_line, current_cursor_char);
								
								int last_line;
								int temp = 0;
								int total_lines = 0;
								int total_cursor_character = 0;
								int flag = true;
								
								for(int i=0; i<bufferIndex; i++){
									if(textBuffer[i] == '\n'){
										total_lines++;
										last_line = i;
									}
								}
								
								if(total_lines == 0)
									last_line = 0;
								//printf("\n %d", last_line);
								
								if(current_cursor_line <= total_lines){
									if(current_cursor_line == total_lines && bufferIndex - last_line < current_cursor_char){
										goto jump_here_two;
									}
								// Shift the cursor to the mouse position when clicked 								
									else{
										for(int i=0; i<bufferIndex; i++){
											if(textBuffer[i] == '\n'){
												temp++; 
											//	printf("\n Current Cursor: %d , %d",current_cursor_line, temp);
											}

											if(temp == current_cursor_line){
												for(int j = current_cursor_char; j != '\n' && j < 0; j--){
													if(textBuffer[i+j] == '\n'){
														current_cursor_char = j;
														total_cursor_character += current_cursor_char;
														break;
													}
												}
												memmove(&textBuffer[cursor], &textBuffer[cursor+1], bufferIndex - cursor + 1);
												memmove(&textBuffer[i+(current_cursor_char)+1], &textBuffer[i +(current_cursor_char)], bufferIndex - i +(current_cursor_char));
												cursor = i+(current_cursor_char);
												break;
											}
										}
										
										// If mouse is clicked outside of text, recaliberate it to the end of the current line
										cursor_line = 0;
										
										for(int i=cursor; i >= 0; i--){
											if(textBuffer[i] == '\n')
												cursor_line++;
										}
										
										//printf("\n %d, %d", current_cursor_line, cursor_line);

										while(cursor_line > current_cursor_line){
											//printf("\n %d", cursor_line);
											while(textBuffer[cursor+1] != '\n'){
												swap(&textBuffer[cursor], &textBuffer[cursor-1]);
												cursor--;
											}
											swap(&textBuffer[cursor], &textBuffer[cursor-1]);
											cursor--;
											cursor_line--;
											if(cursor_line == current_cursor_line){
												swap(&textBuffer[cursor], &textBuffer[cursor+1]);
												cursor++;
											} 
										}
										x = cursor; 
									}
								}
								else{
									jump_here_two:
										memmove(&textBuffer[cursor], &textBuffer[cursor+1], bufferIndex - cursor + 1);
										cursor = bufferIndex;
								}
							}											
					}
					
					temp_flag = 1;
					
					// Store current cursor position before any movement
					
					if (!highlight_flag) {
						// First time initiating highlight
						highlight_flag = 1;
						highlight_anchor = cursor; // Store the initial position where highlighting began
						highlight_start = cursor;
						highlight_end = cursor;
					}
					
					if(cursor < highlight_start){
						highlight_start = cursor+1;
						highlight_end = highlight_anchor+1;
					}else if(cursor > highlight_start){
						highlight_start = highlight_anchor;
						highlight_end = cursor;
					}
				}
			}if(scrollbar_clicked_flag && mouse_clicked_flag == true){
					// Implement smoother scrollbar dragging
					manual_scrollbar_drag = true;
					highlight_flag = 0;

					// Calculate the offset between mouse and scrollbar
					if (scroll_drag_offset == 0) {
						scroll_drag_offset = e.motion.y - scrollbar.y;
					}
					
					// Update manual scroll Y, accounting for the initial offset
					manual_scroll_mouseY = e.motion.y - scroll_drag_offset;
					
					//printf("Drag Offset: %d, Mouse Y: %d, Scrollbar Y: %d\n", 
					//	   scroll_drag_offset, e.motion.y, scrollbar.y);
				}

			} else if(e.type == SDL_MOUSEBUTTONUP){
				isDragging = false; 
				is_scrolling = false;
				scrollbar_flag = false;
				mouse_clicked_flag = false;
				manual_scrollbar_drag = false;  // Reset manual scrollbar drag flag
				scroll_offset = 0; 
				scroll_drag_offset = 0;
				scrollbar_clicked_flag = false;

			}  else if (e.type == SDL_MOUSEWHEEL){

				if(mod & KMOD_CTRL){
										
					// Then in your wheel event handler:
					int new_point_size = current_font_size + e.wheel.y;
					if (new_point_size < 1) {
						new_point_size = 1;
					}

					TTF_CloseFont(font);
					font = TTF_OpenFont("fonts/MonaspaceKryptonFrozen-Regular.ttf", new_point_size);
					if (!font) {
						printf("Failed to load font: %s\n", TTF_GetError());
					} else {
						current_font_size = new_point_size;  // Only update if font loaded successfully
					}
				}
			
				else{				
					if(true){
						manual_scrollbar_drag = true;
						// Use actual wheel delta for more precise scrolling
						int scroll_delta = e.wheel.y * 5;  // Multiply by a factor to increase sensitivity
						manual_scroll_mouseY += scroll_delta;
						
						// Clamp manual_scroll_mouseY within scrollbar bounds
						SDL_GetWindowSize(window, &netWidth, &netHeight);
						int scroll_start = 25;  // Top margin of scrollbar area
						int scroll_end = netHeight - scrollbar.h - 25;  // Bottom margin of scrollbar area
						
						manual_scroll_mouseY = fmax(scroll_start, fmin(manual_scroll_mouseY, scroll_end));
						
						// Optional: Add some acceleration for faster scrolling
						if (abs(e.wheel.y) > 1) {
							manual_scroll_mouseY += (e.wheel.y > 0 ? 1 : -1) * 10;
						}
						
						//printf("Wheel Delta: %d, Scroll Position: %d\n", e.wheel.y, manual_scroll_mouseY);
					}
//				printf("%d \n", line_number);
				}
			} else if (e.type == SDL_KEYDOWN) {
//				printf("%c" ,textBuffer[cursor-1]);
				
				if(bufferIndex + 10 > buffer_size){
					buffer_size = buffer_size * GROWTH_FACTOR;
					char* new_textBuffer = (char*)realloc(textBuffer, buffer_size);
					char* new_tempBuffer = (char*)realloc(tempBuffer, buffer_size);
					if (!new_textBuffer || !new_tempBuffer) {
						printf("Failed to reallocate buffer\n");
						free(textBuffer);
						free(tempBuffer);
						fclose(file);
						return 1;
					}
					textBuffer = new_textBuffer;
					tempBuffer = new_tempBuffer;
				}
				
//				highlight_flag = 0;
                if (((e.key.keysym.sym == SDLK_BACKSPACE && cursor >= 0) || (is_alnum_or_special(e.key.keysym.sym) && highlight_flag == 1 && temp_flag == 1) && (mod & ( KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0)) {
//					printf("HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");					
					if(highlight_flag == 0 && cursor > 0){
						// Move text left to overwrite the character at cursor-1
						memmove(&textBuffer[cursor - 1], &textBuffer[cursor], (bufferIndex - cursor + 1) * sizeof(char));
						// Update cursor and buffer index
						cursor--;
						bufferIndex--;
						manage_buffer_size();
					}else if(highlight_flag == 1){
						// If cursor is at the start of the highlighted section, move it to the end of the highlight
						if(cursor >= highlight_end){
//							printf("Here");
//							printf("%d, %d", highlight_start, highlight_end);
							memmove(&textBuffer[highlight_start+1], &textBuffer[highlight_start], (highlight_end - highlight_start)*sizeof(char));
							cursor = highlight_start;							
						}
						
						// Extra null characters created when the shifting operation has to be compensated by increasing the buffer_size 
						if(bufferIndex + bufferIndex - highlight_start > buffer_size){
							char* new_textBuffer = (char*) realloc(textBuffer, buffer_size*GROWTH_FACTOR*sizeof(char));
							int* new_lineNumbers = (int*) realloc(lineNumbers, buffer_size* GROWTH_FACTOR*sizeof(int));
							printf("Buffer size updated: %d", buffer_size);
							if (new_textBuffer == NULL || new_lineNumbers == NULL) {
								perror("Failed to reallocate buffers");
								free(textBuffer);
								free(tempBuffer);
								free(lineNumbers);
								fclose(file);
								return 1;
							}
							textBuffer = new_textBuffer;
							lineNumbers = new_lineNumbers;
						}

//						printf("%d, %d, %d", bufferIndex + bufferIndex - highlight_start, buffer_size, buffer_size*GROWTH_FACTOR);
					
						memset(&textBuffer[bufferIndex+1], '\0', (bufferIndex - highlight_start));
						memmove(&textBuffer[highlight_start], &textBuffer[highlight_end], (bufferIndex - highlight_start));
						bufferIndex = bufferIndex - (highlight_end - highlight_start);
						manage_buffer_size();
					}
					
					temp_flag = 0;
					
					
                } else if (e.key.keysym.sym == SDLK_RETURN) {
					
					memmove(&textBuffer[cursor+1],&textBuffer[cursor], bufferIndex - cursor);
                    textBuffer[cursor] = '\n'; // Insert a newline character on Return key
					bufferIndex++;
                    cursor++;
					
					SDL_GetWindowSize(window, &netWidth, &netHeight);
/***					if(80+(cursor_line*TTF_FontHeight(font)) >= netHeight){
//						printf("HEREREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
							SDL_Event event;
							// Set up the mouse wheel event
							event.type = SDL_MOUSEWHEEL;
							event.wheel.x = 0;  // Horizontal scroll amount (0 for vertical scroll)
							event.wheel.y = -1; // Negative for downward scroll
							event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
							
							// Push the event to SDL's event queue
							SDL_PushEvent(&event);
							scroll_count -= 1;
							render_y_off -= TTF_FontHeight(font);
					}						
***/
					ensure_cursor_visible();
                } else if (e.key.keysym.sym == SDLK_ESCAPE) { // Note : Move cursor to the end of the buffer and change it to null before saving the file 
					if(argc == 2){
						//printf("\n %d",bufferIndex);

						ftruncate(fileno(file), 0);
						rewind(file);
						
						if (file) {
							memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
							cursor = bufferIndex;
							
							int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
							fflush(file);
							if (result < 0) {
								printf("Error while saving file");
							}
							fclose(file);
						}
						
						free(textBuffer);
						free(tempBuffer);
					}
					quit = 1;
                } else if (e.key.keysym.sym == SDLK_LEFT && cursor > 0) {
					if(highlight_flag == 1 && (mod & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0 && cursor >= highlight_end){
						//printf("%d, %d", highlight_start, highlight_end);
						memmove(&textBuffer[highlight_start+1], &textBuffer[highlight_start], (highlight_end - highlight_start)*sizeof(char));
						cursor = highlight_start;							
					}						
					else{
						swap(&textBuffer[cursor - 1], &textBuffer[cursor]);
						--cursor;
					}
                } else if (e.key.keysym.sym == SDLK_RIGHT && cursor < bufferIndex) {
					if(highlight_flag == 1 && (mod & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0 && cursor < highlight_start){
						memmove(&textBuffer[cursor], &textBuffer[highlight_start], (highlight_end - highlight_start)*sizeof(char));
						cursor = highlight_end-1;													
					}
					else{
						swap(&textBuffer[cursor + 1], &textBuffer[cursor]);
						++cursor;
					}
                } else if (e.key.keysym.sym == SDLK_UP) {
					
/***					if(cursor_highlight_start <= 30){
						printf("HEREREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
							SDL_Event event;
							// Set up the mouse wheel event
							event.type = SDL_MOUSEWHEEL;
							event.wheel.x = 0;  // Horizontal scroll amount (0 for vertical scroll)
							event.wheel.y = 1; // Negative for downward scroll
							event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
							
							// Push the event to SDL's event queue
							SDL_PushEvent(&event);
							scroll_count += 1;
							render_y_off += TTF_FontHeight(font);
					}						

***/
					int i = 0;
					int j;
					int flag_1 = 0;
					while (true){
						if (textBuffer[cursor-1] == '\n' && flag_1 == 0){
							j = i; 
							flag_1 = 1;
						}
						else if((textBuffer[cursor-1] == '\n' && flag_1 == 1 ) || (cursor == 0 && flag_1 == 1)){
							i = 0;
							while (i<j){
								if(textBuffer[cursor + 1] == '\n')
									break;
								i++;
								swap(&textBuffer[cursor + 1], &textBuffer[cursor]);
								cursor++;
							}
							swap(&textBuffer[cursor], &textBuffer[cursor-1]);
							cursor--;
							flag_1 = 0;
							break;
						}
						else if(cursor == 0)
							break;
						
						i++;
						swap(&textBuffer[cursor - 1], &textBuffer[cursor]);
						cursor--;
					}	
//						printf("%d\n",cursor);
				} else if (e.key.keysym.sym == SDLK_DOWN) {

					if (cursor < bufferIndex - 1) {  // Ensure we're not at the end
						int currentCol = 0;
						int tempCursor = cursor;
						int current_line = 0;
						int cursor_y_pos;
						
						// Calculate current line number
						for(int i = 0; i < cursor; i++) {
							if(textBuffer[i] == '\n') {
								current_line++;
							}
						}
						
						// Step 1: Count characters to the left until newline to get current column
						while (tempCursor > 0 && textBuffer[tempCursor - 1] != '\n') {
							tempCursor--;
							currentCol++;
						}
						
						// Step 2: Move cursor to the end of current line while swapping
						while (cursor < bufferIndex && textBuffer[cursor + 1] != '\n') {
							swap(&textBuffer[cursor], &textBuffer[cursor + 1]);
							cursor++;
						}
						
						if (cursor < bufferIndex - 1) {
							// Step 3: Move past the newline
							swap(&textBuffer[cursor], &textBuffer[cursor + 1]);
							cursor++;
							
							// Step 4: Move right in the next line while swapping
							int movedCol = 0;
							while (movedCol < currentCol  && 
								   cursor < bufferIndex - 1 && 
								   textBuffer[cursor + 1] != '\n') {
								swap(&textBuffer[cursor], &textBuffer[cursor + 1]);
								cursor++;
								movedCol++;
							}
							
							// Calculate cursor position in window coordinates
							cursor_y_pos = (current_line + 1) * TTF_FontHeight(font) + render_y_off;
							
							// If cursor would be below visible area, scroll up
							if (cursor_y_pos > netHeight - TTF_FontHeight(font)) {
								// Scroll up by one line
								render_y_off -= TTF_FontHeight(font);
								
								// Update cursor highlight position
								cursor_highlight_start = render_y_off + ((current_line + 1) * TTF_FontHeight(font));
							}
						}
					}
//					printf("%d\n",cursor);
				} 
				else if (e.key.keysym.sym == SDLK_UP) {
					// Find the start of the current line
					{
						int current_line_start = cursor;
						while (current_line_start > 0 && textBuffer[current_line_start - 1] != '\n') {
							current_line_start--;
						}

						// Find the start of the previous line
						int prev_line_start = current_line_start - 1;
						while (prev_line_start > 0 && textBuffer[prev_line_start - 1] != '\n') {
							prev_line_start--;
						}

						if (prev_line_start >= 0) {
							// Calculate current column position
							int current_column = cursor - current_line_start;

							// Find the end of the previous line
							int prev_line_end = current_line_start - 1;

							// Calculate the length of the previous line
							int prev_line_length = prev_line_end - prev_line_start;
							
							// Move cursor to same column in previous line if possible
							if (current_column <= prev_line_length) {
								cursor = prev_line_start + current_column;
							} else {
								// If previous line is shorter, move cursor to end of previous line
								cursor = prev_line_end;
							}
							ensure_cursor_visible();
						}
					}
				}
				if(mod & KMOD_CTRL){
										
					if(e.key.keysym.sym == SDLK_s){	
						if(!file){
							const char *filters[2] = {"*.txt", "*.c"};
							const char *filter_description = "Text or C Files";
							
							filename = tinyfd_saveFileDialog(
								"Save File",            // Dialog title
								"default.txt",           // Default file name
								2,                      // Number of filters
								filters,                // File type filters
								filter_description      // Filter description
							);
							
							if(filename != NULL) {
								if(file) {
									fclose(file);
								}
								file = fopen(filename, "w");
								file = fopen(filename, "r+");
							}
						}
						
						if(filename){				
							if (file) {
								memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
								cursor = bufferIndex;
								
								//printf("Here");
								ftruncate(fileno(file), 0);
								rewind(file);
								int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
								fflush(file);
								if (result < 0) {
									printf("Error while saving file");
								}
								else{
									 notificationMessage = "File saved successfully!";
									 notificationFlag = true;
									 notifStartTime = SDL_GetTicks();
								}
							}else{
								 notificationMessage = "Error while saving the file!";
								 notificationFlag = true;
								 notifStartTime = SDL_GetTicks();
							}	
						}
						else{
							 notificationMessage = "File path or file extension incorrect!";
							 notificationFlag = true;
							 notifStartTime = SDL_GetTicks();
						}
					} else if(e.key.keysym.sym == SDLK_n){	
						const char *filters[2] = {"*.txt", "*.c"};	
						
						open_drawer_flag = false;
						file_item_drop_down_flag = false;
						
						if(file && !fileSaved){
							int result_d = tinyfd_messageBox(
								"",      // Title of the message box
								"Do you want to save the changes?", // The message
								"yesno",      // Type of dialog: yes/no, ok/cancel, etc.
								"question",   // Icon type (question, info, warning, error)
								0             // Default button (0 means no default button)
							);
							
							if(result_d == 1){
								// Save the current file
								ftruncate(fileno(file), 0);
								rewind(file);
								
								if (file) {
									memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
									cursor = bufferIndex;
									
									int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
									fflush(file);
									if (result < 0) {
										printf("Error while saving file");
									}
								}
							}else{
								fclose(file);
								bufferIndex = 0;
								showNotification(renderer,font, "Previous progress not saved!");
							}
						}
						
						const char *filters2[2] = {"*.txt", "*.c"};	
						const char *filter_description2 = "Text or C Files";
						
						filename = tinyfd_saveFileDialog(
							"Save File",            // Dialog title
							"default.txt",           // Default file name
							2,                      // Number of filters
							filters2,                // File type filters
							filter_description2     // Filter description
						);
						
						if(filename != NULL){
							if(file) {
								fclose(file);
							}
							file = fopen(filename, "w");
							file = fopen(filename, "r+");
						}
						else{
							showNotification(renderer, font, "Incorrect file path!");
						}
						
						bufferIndex = 0;
						cursor = 0;
						textBuffer[0] = '\0';
						fileSaved = true;
					}
				}
				if (mod & KMOD_SHIFT) {
//					printf("Shift is pressed.\n");
					
					temp_flag = 0;
					
					// Store current cursor position before any movement
					
					if (!highlight_flag) {
						// First time initiating highlight
						highlight_flag = 1;
						highlight_anchor = cursor; // Store the initial position where highlighting began
						highlight_start = cursor;
						highlight_end = cursor;
					}
					
					if (e.key.keysym.sym == SDLK_LEFT) {
						temp_flag = 1;
						if (cursor >= 0) {
							// Determine highlight boundaries based on anchor point
							if (cursor <= highlight_anchor) {
								highlight_start = cursor;
								highlight_end = highlight_anchor+1;
							} else {
								highlight_start = highlight_anchor;
								highlight_end = cursor+1;
							}
						}
					}
					
					if (e.key.keysym.sym == SDLK_UP) {
						temp_flag = 1;
						if (cursor >= 0) {
							// Determine highlight boundaries based on anchor point
							if (cursor <= highlight_anchor) {
								highlight_start = cursor;
								highlight_end = highlight_anchor+1;
							} else {
								highlight_start = highlight_anchor;
								highlight_end = cursor+1;
							}
//							printf("%d\n",cursor);
						}
					}
					
					if (e.key.keysym.sym == SDLK_RIGHT) {
						temp_flag = 1;
						if (cursor <= bufferIndex) {
								// Determine highlight boundaries based on anchor point
							if (cursor >= highlight_anchor) {
								highlight_start = highlight_anchor;
								highlight_end = cursor+1;
							} else {
								highlight_start = cursor;
								highlight_end = highlight_anchor+1;
							}
						}
					}
			
					if (e.key.keysym.sym == SDLK_DOWN) {
						temp_flag = 1;
						if (cursor <= bufferIndex) {
								// Determine highlight boundaries based on anchor point
							if (cursor >= highlight_anchor) {
								highlight_start = highlight_anchor;
								highlight_end = cursor+1;
							} else {
								highlight_start = cursor;
								highlight_end = highlight_anchor+1;
							}
//							printf("%d\n",cursor);
						}
					}
					
					// Patch to remove the highlight from the cursor 
					
					if(temp_flag){
						if(cursor <= highlight_start)
							highlight_start = highlight_start + 1;
						else if(highlight_end >= cursor)
							highlight_end = highlight_end - 1;
					}
				}                
				else if (mod & KMOD_CTRL){
//					printf("Control is pressed.\n");
					if(e.key.keysym.sym == SDLK_c){
						if(highlight_flag){
							// Validate highlight range
							if (highlight_start >= bufferIndex || highlight_end > bufferIndex) {
								printf("ERROR: Highlight range out of buffer bounds\n");
								printf("highlight_start: %d, highlight_end: %d, bufferIndex: %d\n", 
									   highlight_start, highlight_end, bufferIndex);
								continue;
							}

							// Calculate slice length and check for extreme cases
							size_t slice_length = highlight_end - highlight_start;
							if (slice_length > 1024 * 1024) {  // Limit to 1MB for safety
								printf("WARNING: Attempting to copy very large text: %d bytes\n", slice_length);
							}

							// Detailed debug logging
							printf("Copying: start=%d, end=%d, length=%d, total_lines=%d, virtual_cursor_line=%d, bufferIndex=%d\n", 
								   highlight_start, highlight_end, slice_length, total_lines, virtual_cursor_line, bufferIndex);
							
							// Safe memory allocation and copying
							char* copied_text = string_slice(textBuffer, highlight_start, highlight_end);
							if (copied_text) {
								// Verify copied text
								size_t copied_length = strlen(copied_text);
								printf("Successfully copied %d bytes\n", copied_length);

								// Use SDL's clipboard with error checking
								if (SDL_SetClipboardText(copied_text) != 0) {
									printf("SDL Clipboard Error: %s\n", SDL_GetError());
								}
								free(copied_text);
							} else {
								printf("Failed to copy text: invalid slice parameters\n");
							}
						}
					} else if(e.key.keysym.sym == SDLK_v){
						char* clipboard_text = SDL_GetClipboardText();
						if (clipboard_text) {
							// Debug logging for clipboard content
							size_t clipboard_len = strlen(clipboard_text);
							printf("Paste Debug:\n");
							printf("  Clipboard text length: %d bytes\n", clipboard_len);
							printf("  First 100 chars: %.100s\n", clipboard_text);
							
							// Check for extremely large clipboard content
							if (clipboard_len > 10 * 1024 * 1024) {  // 10MB limit
								printf("ERROR: Clipboard content too large (%d bytes)\n", clipboard_len);
								SDL_free(clipboard_text);
								continue;
							}

							// Clean up the text - remove \r characters
							char* cleaned_text = malloc(clipboard_len + 1);
							if (!cleaned_text) {
								printf("ERROR: Memory allocation failed for cleaned text\n");
								SDL_free(clipboard_text);
								continue;
							}
							
							size_t clean_index = 0;
							size_t carriage_returns_removed = 0;
							
							for (size_t i = 0; i < clipboard_len; i++) {
								if (clipboard_text[i] != '\r') {
									cleaned_text[clean_index++] = clipboard_text[i];
								} else {
									carriage_returns_removed++;
								}
							}
							cleaned_text[clean_index] = '\0';
							
							// Debug logging for cleaned text
							printf("  Carriage returns removed: %d\n", carriage_returns_removed);
							printf("  Cleaned text length: %d bytes\n", clean_index);
							
							size_t paste_len = clean_index;
							
							// Additional debug info about current buffer state
							printf("  Current buffer state:\n");
							printf("    bufferIndex: %d\n", bufferIndex);
							printf("    cursor: %d\n", cursor);
							printf("    total_lines: %d\n", total_lines);
							printf("    buffer_size: %d\n", buffer_size);

							// Ensure buffer has enough space with both textBuffer and tempBuffer
							while ((bufferIndex + paste_len) >= buffer_size) {
								// Debug buffer expansion
								printf("  Expanding buffer:\n");
								printf("    Current buffer_size: %d\n", buffer_size);
								printf("    Current bufferIndex: %d\n", bufferIndex);
								printf("    Paste length: %d\n", paste_len);
								
								buffer_size *= 2;
								
								// Reallocate only textBuffer
								char* new_textBuffer = realloc(textBuffer, buffer_size);
								
								if (!new_textBuffer) {
									printf("CRITICAL ERROR: Failed to expand textBuffer\n");
									free(textBuffer);
									free(tempBuffer);
									fclose(file);
									return 1;
								}
								
								textBuffer = new_textBuffer;
								
								printf("    New buffer_size: %d\n", buffer_size);
							}
							
							// Debug paste insertion
							printf("  Paste insertion:\n");
							printf("    Inserting at cursor: %d\n", cursor);
							printf("    Shifting remaining text\n");
							
							// Shift existing text to make room for pasted content
							memmove(&textBuffer[cursor + paste_len], &textBuffer[cursor], bufferIndex - cursor);
							
							// Copy pasted text
							memcpy(&textBuffer[cursor], cleaned_text, paste_len);
							
							// Update buffer indices
							bufferIndex += paste_len;
							cursor += paste_len;
							
							// Cleanup
							free(cleaned_text);
							SDL_free(clipboard_text);
							
							// Trigger viewport update
							update_viewport();
						}
					} else if(e.key.keysym.sym == SDLK_a){
						// Safety checks for Ctrl+A (Select All)
						if (bufferIndex > 0 && bufferIndex < buffer_size) {
							printf("Select All Debug:\n");
							printf("  bufferIndex: %d\n", bufferIndex);
							printf("  buffer_size: %d\n", buffer_size);
							
							highlight_flag = 1;
							highlight_start = 0;
							highlight_end = bufferIndex;
							highlight_anchor = 0;
							cursor = bufferIndex;
						} else {
							printf("ERROR: Invalid buffer state during Select All\n");
							printf("  bufferIndex: %d\n", bufferIndex);
							printf("  buffer_size: %d\n", buffer_size);
							// Optionally reset buffer or take corrective action
							bufferIndex = 0;
							buffer_size = INITIAL_SIZE;
							highlight_flag = 0;
						}
					}
				}
				else{ 

/***					
					if (is_alnum_or_special(e.key.keysym.sym) & !(mod & KMOD_SHIFT)) {
						printf("Here");
						if (highlight_flag) {
							// Simulate backspace to delete highlighted text
							simulate_backspace(&e);
							// Reset highlight flag after deletion
							highlight_flag = 0;
						}
						// Process the alphanumeric or special character key press
						//printf("Key pressed: %c\n", key);
						// Add your text insertion logic here
					}


						if(highlight_flag == 1){

						SDL_Keycode key = e.key.keysym.sym;
						
						if (isalnum(key) || ispunct(key)) {   
							t_flag = 1;
							printf("Key Pressed is not mod");
						
							SDL_Event simulatedEvent;
							simulatedEvent.type = SDL_KEYDOWN; // Key down event
							simulatedEvent.key.keysym.sym = SDLK_BACKSPACE; // Backspace key

							// Push the simulated event in.to the SDL event queue
							SDL_PushEvent(&simulatedEvent);
						}
					}	
***/					highlight_flag = 0;
				}

            } else if (e.type == SDL_TEXTINPUT) {
                fileSaved = false;
                
                // Get clipboard text
                char* clipboard_text = SDL_GetClipboardText();
                if (clipboard_text) {
                    size_t paste_length = strlen(clipboard_text);
                    
                    // Debug: Print paste information
                    printf("Paste Debug:\n");
                    printf("  Clipboard text length: %zu bytes\n", paste_length);
                    printf("  Current buffer state:\n");
                    printf("    bufferIndex: %d\n", bufferIndex);
                    printf("    buffer_size: %d\n", buffer_size);
                    
                    // Maximum paste size to prevent potential memory issues
                    const size_t MAX_PASTE_SIZE = 1024 * 1024;  // 1MB limit
                    if (paste_length > MAX_PASTE_SIZE) {
                        showNotification(renderer, font, "Paste too large. Truncating.");
                        clipboard_text[MAX_PASTE_SIZE] = '\0';
                        paste_length = MAX_PASTE_SIZE;
                    }
                    
                    // Ensure buffer can accommodate paste
                    size_t required_size = bufferIndex + paste_length + 1;
                    if (required_size > buffer_size) {
                        // Calculate new buffer size with exponential growth
                        size_t new_buffer_size = buffer_size;
                        while (new_buffer_size < required_size) {
                            new_buffer_size *= 2;
                            if (new_buffer_size > MAX_PASTE_SIZE * 4) {
                                showNotification(renderer, font, "Cannot paste. Buffer limit exceeded.");
                                SDL_free(clipboard_text);
                                return 1;
                            }
                        }
                        
                        // Reallocate buffers
                        char* new_textBuffer = (char*)realloc(textBuffer, new_buffer_size);
                        char* new_tempBuffer = (char*)realloc(tempBuffer, new_buffer_size);
                        int* new_lineNumbers = (int*)realloc(lineNumbers, new_buffer_size * sizeof(int));
                        
                        if (!new_textBuffer || !new_tempBuffer || !new_lineNumbers) {
                            showNotification(renderer, font, "Memory allocation failed during paste.");
                            free(new_textBuffer);
                            free(new_tempBuffer);
                            free(new_lineNumbers);
                            SDL_free(clipboard_text);
                            return 1;
                        }
                        
                        textBuffer = new_textBuffer;
                        tempBuffer = new_tempBuffer;
                        lineNumbers = new_lineNumbers;
                        buffer_size = new_buffer_size;
                        
                        printf("  Buffer expanded to: %zu bytes\n", new_buffer_size);
                    }
                    
                    // Insert clipboard text
                    memmove(&textBuffer[cursor + paste_length], &textBuffer[cursor], bufferIndex - cursor);
                    memcpy(&textBuffer[cursor], clipboard_text, paste_length);
                    
                    cursor += paste_length;
                    bufferIndex += paste_length;
                    textBuffer[bufferIndex] = '\0';
                    
                    SDL_free(clipboard_text);
                    
                    // Optional: Show paste size notification
                    char notification[100];
                    snprintf(notification, sizeof(notification), "Pasted %zu characters", paste_length);
                    showNotification(renderer, font, notification);
                }
            }
            // Ensure cursor stays within valid bounds
            if (cursor > bufferIndex) {
                cursor = bufferIndex;
            }
        }
		
        if (SDL_GetTicks() - cursorBlinkTime >= 500) {
            showCursor = !showCursor;
            cursorBlinkTime = SDL_GetTicks();
        }

		if (showCursor) {
			textBuffer[cursor] = '|';
		}else 
			textBuffer[cursor] = ' ';
		
//		printf("\n %d", cursor_line);
		
//		printf("Cursor Line: %d \n",cursor_line);
		
 //       textBuffer[cursor] = '|'; // Render cursor as '|'
//       SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100); // White background

		
		// Set color to red for the rectangle (this will be the color key)
		SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, background_color.a);
		SDL_RenderClear(renderer);
		
		// Update screen
//		SDL_RenderPresent(renderer);
		// Initialize line numbers array
		lineNumbers[0] = 0;
		int line_cnt = 1;
		total_lines = 0;
		
		// Count total lines and build line numbers array
		for(int i = 0; i < bufferIndex; i++) {
			if(textBuffer[i] == '\n') {
				if (line_cnt < buffer_size) {
					lineNumbers[line_cnt] = i + 1;
					line_cnt++;
					total_lines++;
				}
			}
		}

		// Get window dimensions
		SDL_GetWindowSize(window, &netWidth, &netHeight);
		linesToDisplay = (netHeight - 25) / TTF_FontHeight(font);

		// Clear tempBuffer
		tempBuffer[0] = '\0';

		// Update cursor_line and ensure cursor stays within buffer
		cursor_line = 0;
		if (cursor > bufferIndex) {
			cursor = bufferIndex;
		}
		
		for(int i = 0; i < cursor; i++) {
			if(textBuffer[i] == '\n') {
				cursor_line++;
			}
		}

		// Always start rendering from top (25px below menu bar)
		render_y_off = 25;
		int start_pos = 0;
		int end_pos = bufferIndex;

		// Initialize virtual cursor if not set
		if (virtual_cursor_line == 0) {
			virtual_cursor_line = cursor_line;
		}

		if (total_lines > linesToDisplay) {
			// Text exceeds window height
			int visible_lines_above = linesToDisplay / 2;
			int visible_lines_below = linesToDisplay - visible_lines_above;

			// Only move virtual cursor if actual cursor goes too far from it
			if (cursor_line > virtual_cursor_line + visible_lines_below - 2) {
				virtual_cursor_line = cursor_line - visible_lines_below + 2;
			} else if (cursor_line < virtual_cursor_line - visible_lines_above ) {
				virtual_cursor_line = cursor_line + visible_lines_above;
			}

			// Keep virtual cursor in valid range
			if (virtual_cursor_line < visible_lines_above) {
				virtual_cursor_line = visible_lines_above;
			}
			if (virtual_cursor_line > total_lines - visible_lines_below) {
				virtual_cursor_line = total_lines - visible_lines_below;
			}

			// Calculate viewport boundaries
			if (virtual_cursor_line - visible_lines_above >= 0 && 
				virtual_cursor_line - visible_lines_above < total_lines) {
				start_pos = lineNumbers[virtual_cursor_line - visible_lines_above];
			}
			
			if (virtual_cursor_line + visible_lines_below < total_lines) {
				end_pos = lineNumbers[virtual_cursor_line + visible_lines_below];
			}
		} else {
			// Text fits in window, show everything from top
			start_pos = 0;
			end_pos = bufferIndex;
			virtual_cursor_line = cursor_line;
		}


		// Copy visible text to tempBuffer
		int length = end_pos - start_pos;
		if (length > 0 && length < buffer_size) {
			strncpy(tempBuffer, textBuffer + start_pos, length);
			tempBuffer[length+1] = '\0';
		} 
		
		if(cursor >= end_pos){
//			printf("\n HERE");
			cursor = end_pos - 1;
			tempBuffer[end_pos] = '\0';
		}
		
		if(end_pos == 0){
			cursor = end_pos;
			tempBuffer[end_pos + 1] = '\0';
			printf("%c",tempBuffer[0]);
		}
			
/***		
		printf("\n Start: %d", start_pos);
		printf("\n End: %d", end_pos);
		printf("\n Cursor: %d", cursor);
		printf("\n Virtual Cursor: %d", cursor);		
		printf("\n Length: %d", length);
***/		
		// Tokenize and render text based on newline characters
		char* token;
		char* str = tempBuffer; 
		
		int y_off = 0;
		int tokenCnt = 0;
		int highlight_text_index = 0;
		int totalCharsProcessed = 0;		
		
		drawcursor();			

		while ((token = strsep(&str, "\n")) != NULL) {
			tokenCnt++;
			int lineLength = strlen(token);
			// If token is empty, use a space character instead
			char* textToRender = strlen(token) == 0 ? " " : token;
			

			SDL_Surface* textSurface = TTF_RenderText_Blended(font, textToRender, text_color);
			if (textSurface) {
				SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
				if (textTexture) {
					SDL_Rect textRect = {0, y_off + render_y_off, textSurface->w, textSurface->h};
					y_off += textSurface->h;
//					printf("%d, %d \n", textSurface->w, textSurface->h);

					if (highlight_flag == 1) {
						// Calculate the actual position in the full text, accounting for viewport
						int lineStartIdx = totalCharsProcessed;
						int lineEndIdx = lineStartIdx + lineLength;
						
						// Adjust highlight range for viewport scrolling
						int adjusted_highlight_start = highlight_start - lineNumbers[viewport_top_line];
						int adjusted_highlight_end = highlight_end - lineNumbers[viewport_top_line];
						
						// If the line is in the highlighted range
						if (adjusted_highlight_start < lineEndIdx && adjusted_highlight_end > lineStartIdx) {
							// Calculate highlight positions relative to this line
							int highlightBegin = (adjusted_highlight_start > lineStartIdx) ? 
											   (adjusted_highlight_start - lineStartIdx) : 0;
							int highlightEndIdx = (adjusted_highlight_end < lineEndIdx) ? 
												(adjusted_highlight_end - lineStartIdx) : lineLength;
							
							// Render the highlight background
							if (highlightBegin < highlightEndIdx) {
								// Calculate pixel positions based on character positions
								int preHighlightWidth;
								char* preHighlightText = malloc(highlightBegin + 1);
								strncpy(preHighlightText, textToRender, highlightBegin);
								preHighlightText[highlightBegin] = '\0';
								TTF_SizeText(font, preHighlightText, &preHighlightWidth, NULL);
								free(preHighlightText);

								// Get the width of the highlighted portion
								int highlightWidth;
								int highlightLength = highlightEndIdx - highlightBegin;
								char* highlightText = malloc(highlightLength + 1);
								strncpy(highlightText, textToRender + highlightBegin, highlightLength);
								highlightText[highlightLength] = '\0';
								TTF_SizeText(font, highlightText, &highlightWidth, NULL);
								free(highlightText);

								SDL_Rect highlightRect = {
									textRect.x + preHighlightWidth,
									textRect.y,
									highlightWidth,
									textRect.h
								};
								
								SDL_SetRenderDrawColor(renderer, highlightColor.r, 
													 highlightColor.g, 
													 highlightColor.b, 
													 highlightColor.a);
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
		drawMenuBar();
		drawThemesBar();
		drawscroll(scroll_y_pos);
		
		if(notificationFlag){
			showNotification(renderer, font, notificationMessage);
			if(SDL_GetTicks() - notifStartTime > 3000)
				notificationFlag = false;
		}
		
		if(file_item_drop_down_flag == true){
			draw_file_dropdown();				
			
			if(new_drawer_flag_clicked){
				printf("\n New Drawer Selected!");
				new_drawer_flag = false;
				file_item_drop_down_flag = false;
				
				if(!fileSaved){
					if(file){
						int result_d = tinyfd_messageBox(
							"",      // Title of the message box
							"Do you want to save the changes?", // The message
							"yesno",      // Type of dialog: yes/no, ok/cancel, etc.
							"question",   // Icon type (question, info, warning, error)
							0             // Default button (0 means no default button)
						);
						
						if(result_d == 1){
							// Save the current file
							ftruncate(fileno(file), 0);
							rewind(file);
							
							if (file) {
								memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
								cursor = bufferIndex;
								
								int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
								fflush(file);
								if (result < 0) {
									printf("Error while saving file");
								}
							}
						}else{
							fclose(file);
							bufferIndex = 0;
							showNotification(renderer,font, "Previous progress not saved!");
						}
					}else{
							showNotification(renderer, font, "Overriding current progress!");
					}
				}
				
				const char *filters[2] = {"*.txt", "*.c"};
				
				filename = tinyfd_saveFileDialog(
					"Save File",            // Dialog title
					"default.txt",           // Default file name
					2,                      // Number of filters
					filters,                // File type filters
					"Text or C Files"      // Filter description
				);
				
				if(filename != NULL) {
					if(file) {
						fclose(file);
					}
					file = fopen(filename, "w");
					file = fopen(filename, "r+");
				}
				else{
					showNotification(renderer, font, "Incorrect file path!");
				}
				
				bufferIndex = 0;
				cursor = 0;
				textBuffer[0] = '\0';
				fileSaved = true;
			}
			else if(open_drawer_flag_clicked){
				printf("\n Open Drawer Selected!");
				open_drawer_flag = false;
				file_item_drop_down_flag = false;
				
				if(file){
					int result_d = tinyfd_messageBox(
						"",      // Title of the message box
						"Do you want to save chanes made to the previos file?", // The message
						"yesno",      // Type of dialog: yes/no, ok/cancel, etc.
						"question",   // Icon type (question, info, warning, error)
						0             // Default button (0 means no default button)
					);
					
					if(result_d == 1){
						ftruncate(fileno(file), 0);
						rewind(file);
						
						if (file) {
							memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
							cursor = bufferIndex;
							
							int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
							fflush(file);
							if (result < 0) {
								printf("Error while saving file");
							}
							fclose(file);
						}
						
						//free(textBuffer);
						//free(tempBuffer);
					}else{
						fclose(file);
						bufferIndex = 0;
						showNotification(renderer,font, "Previous progress not saved!");
					}
					fclose(file);
				}
				
				bufferIndex = 0;
				
				const char *filters[2] = {"*.txt", "*.c"};	
				const char *filter_description = "Text or C Files";
				
				filename = tinyfd_openFileDialog(
					"Select a file",  // Title of the dialog
					"",               // Initial directory
					2,               // Number of filter patterns
					filters,         // Filter patterns
					filter_description, // Filter description
					0                // Allow multiple files (0 means no default button)
				);
				
				if(filename){
					if(file) {
						fclose(file);
					}
					file = fopen(filename, "r+");
					
					if (file == NULL) {
						perror("Error opening file");
						return 1;
					}

					// Get file size for initial buffer allocation
					fseek(file, 0, SEEK_END);
					long fileSize = ftell(file);
					rewind(file);

					printf("Opening file: %s\n", filename);
					printf("Initial file size: %ld bytes\n", fileSize);
					printf("Initial buffer size: %d\n", buffer_size);

					bufferIndex = 0;
					size_t bytesRead;
					int buffer_growth_count = 0;

					while ((bytesRead = fread(textBuffer + bufferIndex, 1, buffer_size - bufferIndex - 1, file)) > 0) {
						bufferIndex += bytesRead;

						printf("Iteration %d:\n", buffer_growth_count);
						printf("  Bytes read: %zu\n", bytesRead);
						printf("  Current bufferIndex: %zu\n", bufferIndex);
						printf("  Current buffer size: %d\n", buffer_size);

						// If we're close to buffer capacity, grow it
						if (bufferIndex >= buffer_size - 1) {
							buffer_growth_count++;
							
							// Grow buffer
							buffer_size *= GROWTH_FACTOR;
							
							printf("  BUFFER GROWTH ATTEMPT %d:\n", buffer_growth_count);
							printf("  Expanding buffer to: %d\n", buffer_size);

							char* new_textBuffer = (char*)realloc(textBuffer, buffer_size);
							char* new_tempBuffer = (char*)realloc(tempBuffer, buffer_size);
							int* new_lineNumbers = (int*)realloc(lineNumbers, buffer_size);

							if (new_textBuffer == NULL || new_tempBuffer == NULL || new_lineNumbers == NULL) {
								printf("CRITICAL: Failed to expand buffers\n");
								free(textBuffer);
								free(tempBuffer);
								free(lineNumbers);
								fclose(file);
								return 1;
							}

							textBuffer = new_textBuffer;
							tempBuffer = new_tempBuffer;
							lineNumbers = new_lineNumbers;

							printf("  Buffer successfully expanded\n");
						}
					}

					// Null-terminate the buffer
					textBuffer[bufferIndex] = '\0';

					// Final debug information
					printf("File loading complete:\n");
					printf("  Total buffer growth attempts: %d\n", buffer_growth_count);
					printf("  Final buffer size: %d\n", buffer_size);
					printf("  Total bytes read: %zu\n", bufferIndex);

					cursor = bufferIndex;
				}
				else{
					showNotification(renderer, font, "Incorrect file path!");
				}
			}
			else if(save_drawer_flag_clicked){
				printf("\n Save Drawer Selected!");
				save_drawer_flag = false;
				file_item_drop_down_flag = false;
				
				if (file) {
					memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
					cursor = bufferIndex;
						
					//printf("Here");
					ftruncate(fileno(file), 0);
					rewind(file);
					int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
					fflush(file);
					if (result < 0) {
						printf("Error while saving file");
					}
					else{
						 notificationMessage = "File saved successfully!";
						 notificationFlag = true;
						 notifStartTime = SDL_GetTicks();
					}
				}else{
					notificationMessage = "Try Save As or open new file!";
					notificationFlag = true;
					notifStartTime = SDL_GetTicks();
				}	
			}
			
			else if(saveas_drawer_flag_clicked){
				printf("\n Save as Drawer Selected!");
				saveas_drawer_flag = false;
				file_item_drop_down_flag = false;
				
				if(!file){
				const char *filters[2] = {"*.txt", "*.c"};
				const char *filter_description = "Text or C Files";
				
				filename = tinyfd_saveFileDialog(
					"Save File",            // Dialog title
					"default.txt",           // Default file name
					2,                      // Number of filters
					filters,                // File type filters
					filter_description      // Filter description
				);
				
				if(filename != NULL) {
					if(file) {
						fclose(file);
					}
					file = fopen(filename, "w");
					file = fopen(filename, "r+");
				}
				}
				
				
				if(filename){				
					if (file) {
						memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
						cursor = bufferIndex;
							
						//printf("Here");
						ftruncate(fileno(file), 0);
						rewind(file);
						int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
						fflush(file);
						if (result < 0) {
							printf("Error while saving file");
						}
						else{
							 notificationMessage = "File saved successfully!";
							 notificationFlag = true;
							 notifStartTime = SDL_GetTicks();
						}
					}else{
						notificationMessage = "Error while saving the file!";
						notificationFlag = true;
						notifStartTime = SDL_GetTicks();
					}	
				}
				else{
					notificationMessage = "File path or file extension incorrect!";
					notificationFlag = true;
					notifStartTime = SDL_GetTicks();
				}
			}
			else if(exit_drawer_flag_clicked){
				printf("\n Exit Drawer Selected!");
				exit_drawer_flag = false;
				file_item_drop_down_flag = false;
				
				if(fileSaved){
					close_without_saving:
					if(file){
	//					printf("\n %d",bufferIndex);

						ftruncate(fileno(file), 0);
						rewind(file);
						
						if (file) {
							memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
							cursor = bufferIndex;
							
							int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
							fflush(file);
							if (result < 0) {
								printf("Error while saving file");
							}
							fclose(file);
						}
						
						free(textBuffer);
						free(tempBuffer);
					}
					quit = 1;					
				}
				else{
					int result_d = tinyfd_messageBox(
						"",      // Title of the message box
						"Do you want to save progress before closing?", // The message
						"yesno",      // Type of dialog: yes/no, ok/cancel, etc.
						"question",   // Icon type (question, info, warning, error)
						0             // Default button (0 means no default button)
					);
					
					if(result_d == 1){
						ftruncate(fileno(file), 0);
						rewind(file);
						
						if (file) {
							memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
							cursor = bufferIndex;
							
							int result = fprintf(file, "%.*s", bufferIndex, textBuffer);
							fflush(file);
							if (result < 0) {
								printf("Error while saving file");
							}
							fclose(file);
						}
						free(textBuffer);
						free(tempBuffer);
						quit = 1;
					}else{
						fclose(file);
						bufferIndex = 0;
						free(textBuffer);
						free(tempBuffer);
						quit = 1;
					}
				}								
			}
			
			new_drawer_flag_clicked = false;
			open_drawer_flag_clicked = false;
			save_drawer_flag_clicked = false;
			saveas_drawer_flag_clicked = false;
			exit_drawer_flag_clicked = false;
		}

		if(themes_item_drop_down_flag == true){
			draw_themes_dropddown();
			
			if(theme_drawer_forest_flag_clicked){
				printf("\n Forest Drawer Selected!");
				theme_drawer_forest_flag = false;
				themes_item_drop_down_flag = false;
				theme = 1;
				//background_color = ;
				//highlight_color = ;
				//text_color = ;
				//menu_bar_color = ;
				//text_select_color = ;
				//scroll_bar_color = ;
			}
			else if(theme_drawer_mountain_flag_clicked){
				printf("\n Mountain Drawer Selected!");
				theme_drawer_mountain_flag = false;
				themes_item_drop_down_flag = false; 
				theme = 2;
			}
			else if(theme_drawer_bubblegum_flag_clicked){
				printf("\n Bubblegum Drawer Selected!");
				theme_drawer_bubblegum_flag = false;
				themes_item_drop_down_flag = false;
				theme = 3;
			}
			else if(theme_drawer_wood_flag_clicked){
				printf("\n Wood Drawer Selected!");
				theme_drawer_wood_flag = false;
				themes_item_drop_down_flag = false;
				theme = 4;
			}
			else if(theme_drawer_tiles_flag_clicked){
				printf("\n Tiles Drawer Selected!");
				theme_drawer_tiles_flag = false;
				themes_item_drop_down_flag = false;
				theme = 5;
			}
			else if(theme_drawer_obsidian_flag_clicked){
				printf("\n Obsidian Drawer Selected!");
				theme_drawer_obsidian_flag = false;
				themes_item_drop_down_flag = false;
				theme = 6;
			}
			
			theme_drawer_forest_flag_clicked = false;
			theme_drawer_mountain_flag_clicked = false;
			theme_drawer_bubblegum_flag_clicked = false;
			theme_drawer_wood_flag_clicked = false;
			theme_drawer_tiles_flag_clicked = false;
			theme_drawer_obsidian_flag_clicked = false;
		}
		
		// To see if a click event has occured on any of the drawers 
		
		
		
		line_number = tokenCnt;
//		printf("%d\n", line_number);
//		printf("%d",tokenCnt); DEGUBGIN ATTEMPT TO IMPLEMENT A NEW LINE ON AN EMPTY TOKEN 
        SDL_RenderPresent(renderer);
	}

    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

// Helper function to ensure cursor is visible and adjust viewport if needed
void ensure_cursor_visible() {
    update_viewport();
    
    // Get the line height
    int line_height = TTF_FontHeight(font);
    
    // Calculate the y position where text should be rendered
    render_y_off = 25; // Start below menu bar
    
    // If text doesn't fill the window height, center it vertically
    if (total_lines * line_height < netHeight) {
        render_y_off = (netHeight - (total_lines * line_height)) / 2 + 25;
    } else {
        // Adjust render offset based on viewport top line
        render_y_off = 25 - (viewport_top_line * line_height);
    }
}

void update_viewport() {
    // Calculate total visible lines in viewport
    // Account for menu bar (25px) and padding
    int available_height = netHeight - 25;
    int line_height = TTF_FontHeight(font);
    linesToDisplay = available_height / line_height;

    // Initialize virtual cursor line to center of screen if needed
    if (virtual_cursor_line == 0) {
        virtual_cursor_line = linesToDisplay / 2;
    }
    
    // Update cursor_line based on cursor position
    cursor_line = 0;
    for (int i = 0; i < cursor && i < bufferIndex; i++) {
        if (textBuffer[i] == '\n') {
            cursor_line++;
        }
    }

    // Update total_lines count
    total_lines = 1; // Start at 1 since even empty file has one line
    for (int i = 0; i < bufferIndex; i++) {
        if (textBuffer[i] == '\n') {
            total_lines++;
        }
    }
    
    // Adjust viewport when cursor moves outside visible area
    if (cursor_line > viewport_bottom_line) {
        // Cursor moved below viewport
        int shift = cursor_line - viewport_bottom_line;
        virtual_cursor_line += shift;
        viewport_top_line += shift;
        viewport_bottom_line += shift;
    } else if (cursor_line < viewport_top_line) {
        // Cursor moved above viewport
        int shift = viewport_top_line - cursor_line;
        virtual_cursor_line -= shift;
        viewport_top_line -= shift;
        viewport_bottom_line -= shift;
    }
    
    // Keep virtual cursor centered when possible
    if (total_lines > linesToDisplay) {
        int ideal_virtual_line = viewport_top_line + (linesToDisplay / 2);
        if (abs(virtual_cursor_line - ideal_virtual_line) > linesToDisplay / 4) {
            virtual_cursor_line = ideal_virtual_line;
        }
    }
    
    // Ensure viewport boundaries are valid
    if (viewport_top_line < 0) {
        viewport_top_line = 0;
        viewport_bottom_line = linesToDisplay - 1;
        virtual_cursor_line = linesToDisplay / 2;
    }
    if (viewport_bottom_line >= total_lines) {
        viewport_bottom_line = total_lines - 1;
        viewport_top_line = (total_lines > linesToDisplay) ? 
                           viewport_bottom_line - linesToDisplay + 1 : 0;
        virtual_cursor_line = viewport_top_line + (linesToDisplay / 2);
    }
    
    // Update line numbers array
    if (lineNumbers == NULL) {
        lineNumbers = malloc(sizeof(int) * total_lines);
    } else {
        lineNumbers = realloc(lineNumbers, sizeof(int) * total_lines);
    }
    
    // Fill line numbers array
    int line = 0;
    lineNumbers[0] = 0;
    for (int i = 0; i < bufferIndex; i++) {
        if (textBuffer[i] == '\n') {
            line++;
            if (line < total_lines) {
                lineNumbers[line] = i + 1;
            }
        }
    }			
}

void showNotification(SDL_Renderer *renderer, TTF_Font *font, const char *message) {
	
	int windowWidth = 400; // Width of the notification popup
	int windowHeight = 100; // Height of the notification popup
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	
	SDL_Rect notificationRect = {
		(netWidth - windowWidth - 20),  // Center horizontally
		(netHeight - windowHeight - 20), // Center vertically
		windowWidth,                      // Width
		windowHeight                      // Height
	};

	// Render notification background
	SDL_SetRenderDrawColor(renderer, notification_background_color.r, notification_background_color.g, notification_background_color.b, notification_background_color.a); // Gray background 50, 50, 50, 150
	SDL_RenderFillRect(renderer, &notificationRect);

	// Add text to the notification
	SDL_Color textColor = {notification_text_color.r, notification_text_color.g, notification_text_color.b, notification_text_color.a}; // White text
	SDL_Surface* textSurface = TTF_RenderText_Blended(font, message, textColor);
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

	int textWidth, textHeight;
	SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

	SDL_Rect textRect = {
		notificationRect.x + (notificationRect.w - textWidth) / 2,
		notificationRect.y + (notificationRect.h - textHeight) / 2,
		textWidth,
		textHeight
	};    

	SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

	SDL_RenderPresent(renderer);

	// Free resources
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);
}

void drawscroll(int scroll_y) {
    SDL_SetRenderDrawColor(renderer, scroll_bar_color.r, scroll_bar_color.g, scroll_bar_color.b, scroll_bar_color.a);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    
    // Calculate total lines
    int total_lines = 0;
    for(int i = 0; i < bufferIndex; i++) {
        if(textBuffer[i] == '\n') {
            total_lines++;
        }
    }

    if (total_lines * TTF_FontHeight(font) > netHeight) {
        scrollbar.x = netWidth - 25;
        scrollbar.w = 20;
        
        // Calculate scrollbar height based on visible lines ratio
        float visible_ratio = (float)linesToDisplay / total_lines;
        scrollbar.h = (int)((netHeight - 50) * visible_ratio);  // Subtract 50 to account for menu bar and bottom margin
        if (scrollbar.h < 30) scrollbar.h = 30;
        
        // Check if manual scrollbar dragging is active
        if (manual_scrollbar_drag) {
            // Calculate new scrollbar position based on mouse movement
            int scroll_start = 25;  // Top margin of scrollbar area
            int scroll_end = netHeight - scrollbar.h - 25;  // Bottom margin of scrollbar area
            
            // Constrain the manual scroll Y position within the scroll area
            int constrained_scroll_y = fmax(scroll_start, fmin(manual_scroll_mouseY, scroll_end));
            
            // Calculate scroll progress based on constrained mouse Y position
            float scroll_progress = (float)(constrained_scroll_y - scroll_start) / (scroll_end - scroll_start);
            
            // Calculate the first visible line based on scroll progress
            // Allow scrolling beyond viewport limits
            int first_visible_line = (int)(scroll_progress * total_lines);
            
            // Update cursor position to match the first visible line
            int new_cursor = 0;
            for (int line = 0; line < first_visible_line; line++) {
                while (new_cursor < bufferIndex && textBuffer[new_cursor] != '\n') {
                    new_cursor++;
                }
                if (new_cursor < bufferIndex) new_cursor++; // Move past the newline
            }
            
            // Simply update cursor position without modifying buffer
			if(new_cursor > cursor){
				memmove(&textBuffer[cursor], &textBuffer[cursor + 1], new_cursor - cursor);
				cursor = new_cursor;
			}
			else if(new_cursor < cursor){
				memmove(&textBuffer[new_cursor + 1], &textBuffer[new_cursor], cursor - new_cursor);
				cursor = new_cursor;
			}
            
            // Update virtual cursor line and render offset
            virtual_cursor_line = first_visible_line;
            render_y_off = -first_visible_line * TTF_FontHeight(font);
            
            // Update scrollbar position
            scrollbar.y = constrained_scroll_y;
            manual_scrollbar_drag = false;
            //printf("Scroll Progress: %f, First Visible Line: %d, Render Y Offset: %d, Cursor: %d\n", 
            //       scroll_progress, first_visible_line, render_y_off, cursor);
        } else {
            // Original dynamic calculation
            float scroll_progress = (float)(virtual_cursor_line - (linesToDisplay / 2)) / (total_lines - linesToDisplay);
            
            // Ensure scroll_progress is between 0 and 1
            scroll_progress = fmax(0, fmin(1, scroll_progress));
            
            // Calculate max range considering menu bar height
            int max_scrollbar_range = netHeight - scrollbar.h - 25;
            
            // Position scrollbar proportionally, starting just below menu bar
            scrollbar.y = 25 + (int)(scroll_progress * max_scrollbar_range);
        }

        SDL_RenderFillRect(renderer, &scrollbar);
    }
}

void drawcursor() {
    SDL_SetRenderDrawColor(renderer, cursor_color.r, cursor_color.g, cursor_color.b, cursor_color.a);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    
    // Calculate the cursor's Y position based only on render_y_off
    int font_height = TTF_FontHeight(font);
    int cursorY;
	
	// Use original logic when total lines fit in the window
	if (total_lines <= linesToDisplay) {
		cursorY = render_y_off + (cursor_line * font_height);
	} 
	// Use new viewport-adjusted logic when scrolling is needed
	else {
		cursorY = render_y_off + ((cursor_line - (virtual_cursor_line - linesToDisplay/2)) * font_height);
	}

   
    // Only draw if cursor is in visible area
    if (cursorY >= 25 && cursorY < netHeight) {
        cursor_highlight_start = cursorY;
        SDL_Rect cursor_highlight = {0, cursor_highlight_start, netWidth, font_height};
        SDL_RenderFillRect(renderer, &cursor_highlight);
    }
}

void drawMenuBar() {
    SDL_SetRenderDrawColor(renderer, menu_bar_color.r, menu_bar_color.g, menu_bar_color.b, menu_bar_color.a);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    SDL_Rect menuBar = {0, 0, netWidth, FILE_ITEM_HEIGHT};
    SDL_RenderFillRect(renderer, &menuBar);

    // Draw the "File" menu item
    SDL_SetRenderDrawColor(renderer, menu_bar_item_color.r, menu_bar_item_color.g, menu_bar_item_color.b, menu_bar_item_color.a);
    fileItem.x = 0;
    fileItem.y = 0;
    fileItem.w = FILE_ITEM_WIDTH;
    fileItem.h = FILE_ITEM_HEIGHT;

    SDL_RenderFillRect(renderer, &fileItem);

    // Set text color
    SDL_Color textColor = {menubar_item_font_color.r, menubar_item_font_color.g, menubar_item_font_color.b, menubar_item_font_color.a};

    font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);
    
    // Render the text for "File"
    SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, "File", textColor);
    if (!textSurface) {
        SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

    SDL_Rect textRect = {
        fileItem.x + 5,
        fileItem.y + (fileItem.h - textHeight) / 2,
        textWidth,
        textHeight
    };

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
}

void draw_file_dropdown() {
    SDL_SetRenderDrawColor(renderer, dropdown_color.r, dropdown_color.g, dropdown_color.b, dropdown_color.a);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    
    fileItem_DROP_DOWN.x = 0;
    fileItem_DROP_DOWN.y = FILE_ITEM_HEIGHT;
    fileItem_DROP_DOWN.w = 200;
    fileItem_DROP_DOWN.h = 142;
    
    int height_one_above = fileItem_DROP_DOWN.y;
    SDL_RenderFillRect(renderer, &fileItem_DROP_DOWN);

    void each_drawer(char* name) {
        SDL_Color textColor = {drawertext_color.r, drawertext_color.g, drawertext_color.b, drawertext_color.a};
        font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);

        SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, name, textColor);
        if (!textSurface) {
            SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);

        int textWidth, textHeight;
        SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

        textRect.x = fileItem_DROP_DOWN.x + 5;
        textRect.y = height_one_above + 5;
        textRect.w = textWidth;
        textRect.h = textHeight;

        height_one_above = textRect.y + textRect.h + 5;
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        textRect.w = 200;

        if (isMouseOver(textRect, mouseX, mouseY)) {
            if(strcmp(name, "New") == 0) new_drawer_flag = true; else new_drawer_flag = false;
            if(strcmp(name, "Open") == 0) open_drawer_flag = true; else open_drawer_flag = false;
            if(strcmp(name, "Save") == 0) save_drawer_flag = true; else save_drawer_flag = false;
            if(strcmp(name, "Save As") == 0) saveas_drawer_flag = true; else saveas_drawer_flag = false;
            if(strcmp(name, "Exit") == 0) exit_drawer_flag = true; else exit_drawer_flag = false;
            
            SDL_SetRenderDrawColor(renderer, horizontal_line_below_text_in_menu_color_bold.r,
                                 horizontal_line_below_text_in_menu_color_bold.g,
                                 horizontal_line_below_text_in_menu_color_bold.b,
                                 horizontal_line_below_text_in_menu_color_bold.a);
        } else {
            SDL_SetRenderDrawColor(renderer, horizontal_line_below_text_in_menu_color.r,
                                 horizontal_line_below_text_in_menu_color.g,
                                 horizontal_line_below_text_in_menu_color.b,
                                 horizontal_line_below_text_in_menu_color.a);
        }
        
        SDL_RenderDrawLine(renderer,
            fileItem_DROP_DOWN.x,
            textRect.y + textRect.h + 5,
            fileItem_DROP_DOWN.x + fileItem_DROP_DOWN.w,
            textRect.y + textRect.h + 5
        );

        SDL_DestroyTexture(textTexture);
    }
    
    each_drawer("New");    
    each_drawer("Open");
    each_drawer("Save");
    each_drawer("Save As");
    each_drawer("Exit");
}

void drawThemesBar() {
    SDL_SetRenderDrawColor(renderer, menu_bar_item_color.r, menu_bar_color.g, menu_bar_color.b, menu_bar_color.a);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    SDL_Rect themeBar = {FILE_ITEM_WIDTH, 0, netWidth, FILE_ITEM_HEIGHT};
    SDL_RenderFillRect(renderer, &themeBar);

    SDL_SetRenderDrawColor(renderer, menu_bar_item_color.r, menu_bar_item_color.g, menu_bar_item_color.b, menu_bar_item_color.a);
    themeItem.x = FILE_ITEM_WIDTH + 1;
    themeItem.y = 0;
    themeItem.w = THEME_ITEM_WIDTH + 20;
    themeItem.h = THEME_ITEM_HEIGHT;

    SDL_RenderFillRect(renderer, &themeItem);

    SDL_Color textColor = {text_color.r, text_color.g, text_color.b, text_color.a};
    font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);
    
    SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, "Theme", textColor);
    if (!textSurface) {
        SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

    SDL_Rect textRect = {
        themeItem.x + 5,
        themeItem.y + (themeItem.h - textHeight) / 2,
        textWidth,
        textHeight
    };    

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
}

void draw_themes_dropddown() {
    SDL_SetRenderDrawColor(renderer, dropdown_color.r, dropdown_color.g, dropdown_color.b, dropdown_color.a);
    SDL_GetWindowSize(window, &netWidth, &netHeight);
    
    themeItem_DROP_DOWN.x = THEME_ITEM_WIDTH + 1;
    themeItem_DROP_DOWN.y = THEME_ITEM_HEIGHT;
    themeItem_DROP_DOWN.w = 200;
    themeItem_DROP_DOWN.h = 170;
    
    int height_one_above = themeItem_DROP_DOWN.y;
    SDL_RenderFillRect(renderer, &themeItem_DROP_DOWN);

    void each_drawer(char* name) {
        SDL_Color textColor = {text_color.r, text_color.g, text_color.b, text_color.a};
        font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);

        SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, name, textColor);
        if (!textSurface) {
            SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);

        int textWidth, textHeight;
        SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

        textRect.x = themeItem_DROP_DOWN.x + 5;
        textRect.y = height_one_above + 5;
        textRect.w = textWidth;
        textRect.h = textHeight;
        
        height_one_above = textRect.y + textRect.h + 5;
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        textRect.w = 200;

        if (isMouseOver(textRect, mouseX, mouseY)) {
            if(strcmp(name, "Forest") == 0) theme_drawer_forest_flag = true; else theme_drawer_forest_flag = false;
            if(strcmp(name, "Mountain") == 0) theme_drawer_mountain_flag = true; else theme_drawer_mountain_flag = false;
            if(strcmp(name, "Bubblegum") == 0) theme_drawer_bubblegum_flag = true; else theme_drawer_bubblegum_flag = false;
            if(strcmp(name, "Wood") == 0) theme_drawer_wood_flag = true; else theme_drawer_wood_flag = false;
            if(strcmp(name, "Tiles") == 0) theme_drawer_tiles_flag = true; else theme_drawer_tiles_flag = false;
            if(strcmp(name, "Obsidian") == 0) theme_drawer_obsidian_flag = true; else theme_drawer_obsidian_flag = false;
            
            SDL_SetRenderDrawColor(renderer, horizontal_line_below_text_in_menu_color_bold.r,
                                 horizontal_line_below_text_in_menu_color_bold.g,
                                 horizontal_line_below_text_in_menu_color_bold.b,
                                 horizontal_line_below_text_in_menu_color_bold.a);
        } else {
            SDL_SetRenderDrawColor(renderer, horizontal_line_below_text_in_menu_color.r,
                                 horizontal_line_below_text_in_menu_color.g,
                                 horizontal_line_below_text_in_menu_color.b,
                                 horizontal_line_below_text_in_menu_color.a);
        }
        
        SDL_RenderDrawLine(renderer,
            themeItem_DROP_DOWN.x,
            textRect.y + textRect.h + 5,
            themeItem_DROP_DOWN.x + themeItem_DROP_DOWN.w,
            textRect.y + textRect.h + 5
        );

        SDL_DestroyTexture(textTexture);
    }
    
    each_drawer("Forest");
    each_drawer("Mountain");
    each_drawer("Bubblegum");
    each_drawer("Wood");
    each_drawer("Tiles");
    each_drawer("Obsidian");
}

void manage_buffer_size() {
    const int MIN_BUFFER_SIZE = 256;     // Minimum buffer size
    const float SHRINK_THRESHOLD = 0.25; // Shrink when buffer is less than 25% full
    //const float GROWTH_FACTOR = 1.5;     // Factor to grow buffer when needed

    // Calculate current buffer usage
    float buffer_usage = (float)bufferIndex / buffer_size;

    // Determine optimal buffer size based on actual content
    int optimal_buffer_size = bufferIndex + (bufferIndex * 0.2); // Add 20% extra space
    optimal_buffer_size = fmax(optimal_buffer_size, MIN_BUFFER_SIZE);

    // Debug: Print current buffer state
    printf("Buffer Management:\n");
    printf("  Current buffer_size: %d\n", buffer_size);
    printf("  Current bufferIndex: %d\n", bufferIndex);
    printf("  Buffer usage: %.2f%%\n", buffer_usage * 100);
    printf("  Optimal buffer size: %d\n", optimal_buffer_size);

    // Shrink buffer if significantly oversized
    if (buffer_usage < SHRINK_THRESHOLD && buffer_size > optimal_buffer_size) {
        // Reallocate buffer to optimal size
        char* new_textBuffer = (char*)realloc(textBuffer, optimal_buffer_size);
        
        if (new_textBuffer) {
            textBuffer = new_textBuffer;
            
            // Zero out the newly allocated space beyond current content
            if (optimal_buffer_size > bufferIndex) {
                memset(textBuffer + bufferIndex + 1, 0, optimal_buffer_size - bufferIndex - 1);
            }
            
            // Update buffer size
            buffer_size = optimal_buffer_size;
            
            // Debug logging
            printf("  Buffer shrunk to optimal size\n");
            printf("  New buffer_size: %d\n", buffer_size);
        } else {
            // Allocation failed, log error
            printf("  ERROR: Failed to shrink buffer\n");
        }
    }
}