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


// Helper Functions

// Show notification 
void showNotification(SDL_Renderer* , TTF_Font* , const char *);
char* notificationMessage;
bool notificationFlag;
clock_t notifStartTime;


// SDL Clickable regions 
// File

SDL_Rect fileItem = {0,0,0,0}; 
SDL_Rect themeItem = {0,0,0,0};
SDL_Rect scrollbar = {0,0,0,0};

SDL_Rect fileItem_DROP_DOWN = {0,0,0,0};
SDL_Rect themeItem_DROP_DOWN = {0,0,0,0};

// Scrollbar variables
bool scrollbar_flag = false;
int scroll_y_pos;


// Mouse functionality variables	
bool isDragging = false;
int mouseX; 
int mouseY;
// Is mouse over a certain region (SDL_Rect)
bool isMouseOver(SDL_Rect rect, int mouseX, int mouseY) {
	
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

// Line number
int line_number;
int cursor_line;
int current_cursor_line;
int current_cursor_char;

// Height of the cursors highlight to render from 
int cursor_highlight_start;

// Highlight flag
bool highlight_flag = 0;
int highlight_start;
int highlight_end;
int highlight_anchor;


// File drop down flag 
bool file_item_drop_down_flag = false;
bool themes_item_drop_down_flag = false;


//  Drawer Flags 
bool new_drawer_flag = false;
bool open_drawer_flag = false;
bool save_drawer_flag = false;
bool saveas_drawer_flag = false;
bool exit_drawer_flag = false;

bool theme_drawer_forest_flag = false;
bool theme_drawer_mountain_flag = false;
bool theme_drawer_bubblegum_flag = false;
bool theme_drawer_wood_flag = false;
bool theme_drawer_tiles_flag = false;
bool theme_drawer_obsidian_flag = false;

bool mouse_clicked_flag = false;


// Temporary flag for the shift operation
int temp_flag;
int t_flag = 0;

// Y offset of the rendered textbox on the screen
int render_y_off = 25;
// To store the height of the entire block that is currently being rendered
int currentTextBlockHeight;

// Count the number of scrolls and substract when scroll up. To align the higlight line text with the cursor
int scroll_count;

// Menu Bar 
int netWidth;
int netHeight;

// Menu item dimensions
const int FILE_ITEM_HEIGHT = 20;
const int FILE_ITEM_WIDTH = 40;

// Theme item dimensions
const int THEME_ITEM_HEIGHT = 20;
const int THEME_ITEM_WIDTH = 40;

// Colour of the highlight when text is selected
SDL_Color highlightColor = {33, 150, 243, 0.35*255};

enum MenuItem {
    FILE_QUIT,
    FILE_COUNT
};

SDL_Window* window = NULL;
		
SDL_Renderer* renderer = NULL;

TTF_Font* font = NULL;
TTF_Font* font_menu = NULL;

int current_font_size = 16;  // Starting font size

void drawscroll(int scroll_y){
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	SDL_SetRenderDrawColor(renderer, 0, 150, 150, 255);
	
	scrollbar.x = netWidth - 25; 
	scrollbar.y = render_y_off + scroll_y;
	scrollbar.w = 20; 
	scrollbar.h = 40;
	
	SDL_RenderFillRect(renderer, &scrollbar);
}

void drawcursor(){
	SDL_SetRenderDrawColor(renderer, 0, 150, 150, 100);
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	
	// Calculate the cursor's Y position
    int cursorY = 20 + ((cursor_line * TTF_FontHeight(font)) + render_y_off);
   
   if (cursorY >= 20 && cursorY < netHeight) {
	    cursor_highlight_start = 25 + ((cursor_line*TTF_FontHeight(font)) + scroll_count*TTF_FontHeight(font))%netHeight;
		SDL_Rect cursor_highlight = {0, cursor_highlight_start, netWidth, TTF_FontHeight(font)};

		SDL_RenderFillRect(renderer, &cursor_highlight);
   }
}

void drawMenuBar() {
	// Set the color for the menu bar (dark gray)	
	SDL_SetRenderDrawColor(renderer, 27, 40, 48, 255);
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	SDL_Rect menuBar = {0, 0, netWidth, FILE_ITEM_HEIGHT};
	SDL_RenderFillRect(renderer, &menuBar);
	

	// Draw the "File" menu item
	SDL_SetRenderDrawColor(renderer, 3, 20, 31, 255);  // White color
	fileItem.x = 0;
	fileItem.y = 0;
	fileItem.w = FILE_ITEM_WIDTH;
	fileItem.h = FILE_ITEM_HEIGHT;

	SDL_RenderFillRect(renderer, &fileItem);
	
	// Draw a border around the "File" menu item (black border)
//	SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);  // Black color for the border
//	SDL_RenderDrawRect(renderer, &menuBar);  // Draw the border around the menu item
	
//	SDL_RenderDrawLine(renderer, menuBar.x, menuBar.y, menuBar.x, menuBar.y + menuBar.h);          // Left side
//	SDL_RenderDrawLine(renderer, menuBar.x + menuBar.w, menuBar.y, menuBar.x + menuBar.w, menuBar.y + menuBar.h);  // Right side
//	SDL_RenderDrawLine(renderer, menuBar.x, menuBar.y + menuBar.h, menuBar.x + menuBar.w, menuBar.y + menuBar.h);  // Bottom side


	// Set text color (black)
	SDL_Color textColor = {255, 255, 255, 255};

	font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);
	
	// Render the text for "File"
	SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, "File", textColor);
	if (!textSurface) {
		SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
	}

	// Create a texture from the surface
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_FreeSurface(textSurface);  // Free the surface as it's no longer needed

	// Get the dimensions of the text texture
	int textWidth = 0, textHeight = 0;
	SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

	// Calculate the position to center the text inside the menu item
	SDL_Rect textRect = {
		fileItem.x + 5,  // Center horizontally
		fileItem.y + (fileItem.h - textHeight) / 2, // Center vertically
		textWidth,
		textHeight
	};

	// Render the text on the screen
	SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

	// Destroy the text texture after rendering
	SDL_DestroyTexture(textTexture);
}

void draw_file_dropdown(){
	SDL_SetRenderDrawColor(renderer, 3, 20, 31, 255);  // Set dropdown background color
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	
	fileItem_DROP_DOWN.x = 0;  // Dropdown area
	fileItem_DROP_DOWN.y = FILE_ITEM_HEIGHT;
	fileItem_DROP_DOWN.w = 200; 
	fileItem_DROP_DOWN.h = 142;
	
	
	
	SDL_Rect textRect;
	int height_one_above = fileItem_DROP_DOWN.y;

	// Render the dropdown background
	SDL_RenderFillRect(renderer, &fileItem_DROP_DOWN);

	void each_drawer(char* name){
		// Define text color and load font
		SDL_Color textColor = {255, 255, 255, 255};
		
		font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);

		// Render the text for "New"
		SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, name, textColor);
		if (!textSurface) {
			SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
		}

		// Create a texture from the surface
		SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
		SDL_FreeSurface(textSurface);  // Free the surface as it's no longer needed

		// Get the dimensions of the text texture
		int textWidth = 0, textHeight = 0;
		SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

		// Calculate the position to center the text inside the menu item
		textRect.x = fileItem_DROP_DOWN.x + 5;  // Horizontal padding
		textRect.y = height_one_above + 5;  // Vertically center the text
		textRect.w = textWidth;
		textRect.h = textHeight;

		
		height_one_above = textRect.y + textRect.h + 5;
		// Render the text on the screen
		SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

		textRect.w = 200;

		// Render the horizontal line below the text, mouse over a current drawer then highlight it 
		if (isMouseOver(textRect, mouseX, mouseY)){
			if(name == "New") new_drawer_flag = true; else new_drawer_flag = false;
			if(name == "Open") open_drawer_flag = true; else open_drawer_flag = false;
			if(name == "Save") save_drawer_flag = true;else save_drawer_flag = false;
			if(name == "Save As") saveas_drawer_flag = true; else saveas_drawer_flag = false;
			if(name == "Exit") exit_drawer_flag = true; else exit_drawer_flag = false;
			
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		}
		else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);  // Set line color (black)
		
		
		SDL_RenderDrawLine(renderer,
			fileItem_DROP_DOWN.x,                 // Left edge of the dropdown
			textRect.y + textRect.h + 5,          // Just below the text
			fileItem_DROP_DOWN.x + fileItem_DROP_DOWN.w,  // Right edge of the dropdown
			textRect.y + textRect.h + 5           // Just below the text
		);

		// Destroy the text texture after rendering
		SDL_DestroyTexture(textTexture);
	}
	
	each_drawer("New");	
	each_drawer("Open");
	each_drawer("Save");
	each_drawer("Save As");
	each_drawer("Exit");
}
void drawThemesBar() {
	// Set the color for the menu bar (dark gray)	
	SDL_SetRenderDrawColor(renderer, 27, 40, 48, 255);
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	SDL_Rect themeBar = {FILE_ITEM_WIDTH, 0, netWidth, FILE_ITEM_HEIGHT};
	SDL_RenderFillRect(renderer, &themeBar);
	

	// Draw the "File" menu item
	SDL_SetRenderDrawColor(renderer, 3, 20, 31, 255);  // White color
	themeItem.x = FILE_ITEM_WIDTH + 1;
	themeItem.y = 0;
	themeItem.w = FILE_ITEM_WIDTH + 20;
	themeItem.h = FILE_ITEM_HEIGHT;

	SDL_RenderFillRect(renderer, &themeItem);
	
	// Draw a border around the "File" menu item (black border)
//	SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);  // Black color for the border
//	SDL_RenderDrawRect(renderer, &menuBar);  // Draw the border around the menu item
	
//	SDL_RenderDrawLine(renderer, menuBar.x, menuBar.y, menuBar.x, menuBar.y + menuBar.h);          // Left side
//	SDL_RenderDrawLine(renderer, menuBar.x + menuBar.w, menuBar.y, menuBar.x + menuBar.w, menuBar.y + menuBar.h);  // Right side
//	SDL_RenderDrawLine(renderer, menuBar.x, menuBar.y + menuBar.h, menuBar.x + menuBar.w, menuBar.y + menuBar.h);  // Bottom side


	// Set text color (black)
	SDL_Color textColor = {255, 255, 255, 255};

	font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);
	
	// Render the text for "File"
	SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, "Theme", textColor);
	if (!textSurface) {
		SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
	}

	// Create a texture from the surface
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_FreeSurface(textSurface);  // Free the surface as it's no longer needed

	// Get the dimensions of the text texture
	int textWidth = 0, textHeight = 0;
	SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

	// Calculate the position to center the text inside the menu item
	SDL_Rect textRect = {
		themeItem.x + 5,  // Center horizontally
		themeItem.y + (themeItem.h - textHeight) / 2, // Center vertically
		textWidth,
		textHeight
	};	

	// Render the text on the screen
	SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

	// Destroy the text texture after rendering
	SDL_DestroyTexture(textTexture);
}

void draw_themes_dropddown(){
	SDL_SetRenderDrawColor(renderer, 3, 20, 31, 255);  // Set dropdown background color
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	
	themeItem_DROP_DOWN.x = THEME_ITEM_WIDTH + 1;  // Dropdown area
	themeItem_DROP_DOWN.y = THEME_ITEM_HEIGHT;
	themeItem_DROP_DOWN.w = 200;
	themeItem_DROP_DOWN.h = 170;
	
	
	SDL_Rect textRect;
	int height_one_above = themeItem_DROP_DOWN.y;

	// Render the dropdown background
	SDL_RenderFillRect(renderer, &themeItem_DROP_DOWN);

	void each_drawer(char* name){
		// Define text color and load font
		SDL_Color textColor = {255, 255, 255, 255};
		font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);

		// Render the text for "New"
		SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, name, textColor);
		if (!textSurface) {
			SDL_Log("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
		}

		// Create a texture from the surface
		SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
		SDL_FreeSurface(textSurface);  // Free the surface as it's no longer needed

		// Get the dimensions of the text texture
		int textWidth = 0, textHeight = 0;
		SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

		// Calculate the position to center the text inside the menu item
		textRect.x = themeItem_DROP_DOWN.x + 5;  // Horizontal padding
		textRect.y = height_one_above + 5;  // Vertically center the text
		textRect.w = textWidth;
		textRect.h = textHeight;
		
		// Render the horizontal line below the text
		
		height_one_above = textRect.y + textRect.h + 5;
		// Render the text on the screen
		SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

		textRect.w = 200;

		if (isMouseOver(textRect, mouseX, mouseY)){
			if(name == "Forest") theme_drawer_forest_flag = true; else theme_drawer_forest_flag = false;
			if(name == "Mountain") theme_drawer_mountain_flag = true; else theme_drawer_mountain_flag = false;
			if(name == "Bubblegum") theme_drawer_bubblegum_flag = true; else theme_drawer_bubblegum_flag = false;
			if(name == "Wood") theme_drawer_wood_flag = true; else theme_drawer_wood_flag = false;
			if(name == "Tiles") theme_drawer_tiles_flag = true; else theme_drawer_tiles_flag = false;
			if(name == "Obsidian") theme_drawer_obsidian_flag = true; else theme_drawer_obsidian_flag = false;
			
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		}
		else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);  // Set line color (black)
		
		SDL_RenderDrawLine(renderer,
			themeItem_DROP_DOWN.x,                 // Left edge of the dropdown
			textRect.y + textRect.h + 5,          // Just below the text
			themeItem_DROP_DOWN.x + themeItem_DROP_DOWN.w,  // Right edge of the dropdown
			textRect.y + textRect.h + 5           // Just below the text
		);


		// Destroy the text texture after rendering
		SDL_DestroyTexture(textTexture);
	}
	
	each_drawer("Forest");
	each_drawer("Mountain");
	each_drawer("Bubblegum");
	each_drawer("Wood");
	each_drawer("Tiles");
	each_drawer("Obsidian");
	
}

int main(int argc, char* argv[]) {
	size_t buffer_size = INITIAL_SIZE;
    char* textBuffer = (char*)malloc(buffer_size); // Buffer to store user input
	char* tempBuffer = (char*)malloc(buffer_size);

	if(textBuffer == NULL){
		perror("Initializing buffer failed!");
	}
//    char tempBuffer[256]; // Temporary buffer for strtok operations
    int quit = 0;
	int cursor = 0;
	int bufferIndex = 0;
	int tmpBufferIndex = 0;
	
	textBuffer[0] = '\0';
		
	if (argc == 2){
		filename = argv[1];  // Get the filename from command-line arguments
		file = fopen(filename, "r+");  // Open the file in read mode

		if (file == NULL) {
			perror("Error opening file");
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
        // Write the buffer to stdout (or process it)
 //       fwrite(textBuffer, 1, bufferIndex, stdout);
    }
		
//		bufferIndex = strlen(textBuffer);
//		printf("%d",bufferIndex);
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

    SDL_Color textColor = {255, 255, 255, 255}; // Black color
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
				mouseX = e.button.x;
				mouseY = e.button.y;
				
//				*x = mouseX; 
//				*y = mouseY;
				
				if(highlight_flag)
					highlight_flag = 0;
				
				isDragging = true;
				
				if(e.button.button == SDL_BUTTON_LEFT){
				// Check if the click occurred within the clickable region
					mouse_clicked_flag = true;
					
					if(mouseY > 25){
						current_cursor_line = (mouseY - 25)/TTF_FontHeight(font);
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
						printf("\n %d", last_line);
						
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
									printf("\n %d", cursor_line);
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
								printf("HERE");

								memmove(&textBuffer[cursor], &textBuffer[cursor+1], bufferIndex - cursor + 1);
								cursor = bufferIndex;
						}
					}
					
										
					if ((isMouseOver(fileItem, mouseX, mouseY) && file_item_drop_down_flag == false)) file_item_drop_down_flag = true;
					
					else file_item_drop_down_flag = false;						
					
					if ((isMouseOver(themeItem, mouseX, mouseY) && themes_item_drop_down_flag == false)) themes_item_drop_down_flag = true;
					else themes_item_drop_down_flag = false;	
					
					if (isMouseOver(scrollbar, mouseX, mouseY)) scrollbar_flag = true;
					else scrollbar_flag = false;
				}
			} else if(e.type == SDL_MOUSEMOTION){
				mouseX = e.button.x;
				mouseY = e.button.y;

				// Mouse over file menu condition 
				if(e.button.button == SDL_BUTTON_LEFT){
					if(scrollbar_flag){ 					
						scroll_y_pos = e.motion.y - 40;
						if(scroll_y_pos < 0) scroll_y_pos = 0;
					}
					if(isDragging == true && (e.motion.x > 0 && e.motion.y > 25)){
						
						
						//if(e.button.button == SDL_BUTTON_LEFT){
						// Check if the click occurred within the clickable region
							if(mouseY > 25){

								current_cursor_line = (mouseY - 25)/TTF_FontHeight(font);
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

								printf("\n %d, %d", total_lines+1, current_cursor_line+1);
								
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
											printf("\n %d", cursor_line);
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
									jump_here_two:
										memmove(&textBuffer[cursor], &textBuffer[cursor+1], bufferIndex - cursor + 1);
										cursor = bufferIndex;
								}
							}
							
												
							if (isMouseOver(fileItem, mouseX, mouseY) && file_item_drop_down_flag == false) file_item_drop_down_flag = true;
							else file_item_drop_down_flag = false;						
							
							if (isMouseOver(themeItem, mouseX, mouseY) && themes_item_drop_down_flag == false) themes_item_drop_down_flag = true;
							else themes_item_drop_down_flag = false;	
											
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
				
			} else if(e.type == SDL_MOUSEBUTTONUP){
				isDragging = false; 
				scrollbar_flag = false;
				mouse_clicked_flag = false;
				
			} else if (e.type == SDL_MOUSEWHEEL){

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
					if((e.wheel.y > 0 || currentTextBlockHeight + render_y_off > netHeight) && (render_y_off < 25 || e.wheel.y < 0)){
						SDL_Event event;
						if(e.wheel.y == 1){
							event.type = SDL_KEYDOWN;
							event.key.keysym.sym = SDLK_DOWN;
							SDL_PushEvent(&event);
						}
						else if(e.wheel.y == -1){
							event.type = SDL_KEYDOWN;
							event.key.keysym.sym = SDLK_UP;
							SDL_PushEvent(&event);
						}
						
						scroll_count += e.wheel.y;
						render_y_off += (e.wheel.y*TTF_FontHeight(font));
					}
					if(render_y_off > 25) render_y_off = 25;
				}
//				printf("%d \n", line_number);
				
			} else if (e.type == SDL_KEYDOWN) {
//				printf("%c" ,textBuffer[cursor-1]);
				
				if(bufferIndex + 10 > buffer_size){
					buffer_size = buffer_size * GROWTH_FACTOR;
					textBuffer = (char*)realloc(textBuffer, buffer_size);
					tempBuffer = (char*)realloc(tempBuffer, buffer_size);
					
					if (textBuffer == NULL || tempBuffer == NULL) {
						perror("realloc failed");
						free(textBuffer);
						free(tempBuffer);
						fclose(file);
						exit(1);
					}
					printf("Buffer Updated! Buffer size: %d",buffer_size);					
				}
				
//				highlight_flag = 0;
                if (((e.key.keysym.sym == SDLK_BACKSPACE && cursor >= 0) || (is_alnum_or_special(e.key.keysym.sym) && highlight_flag == 1 && temp_flag == 1) && (mod & ( KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0)) {
//					printf("HEREEEEEEEEEEEEEEEEEEEEEEEE");					
					if(highlight_flag == 0 && cursor > 0){
						// Move text left to overwrite the character at cursor-1
						memmove(&textBuffer[cursor - 1], &textBuffer[cursor], (bufferIndex - cursor + 1) * sizeof(char));
						// Update cursor and buffer index
						cursor--;
						bufferIndex--;
						
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
							textBuffer = (char*) realloc(textBuffer, buffer_size*GROWTH_FACTOR*sizeof(char));
							printf("Buffer size updated: %d", buffer_size);
						}

//						printf("%d, %d, %d", bufferIndex + bufferIndex - highlight_start, buffer_size, buffer_size*GROWTH_FACTOR);
					
						memset(&textBuffer[bufferIndex+1], '\0', (bufferIndex - highlight_start));
						memmove(&textBuffer[highlight_start], &textBuffer[highlight_end], (bufferIndex - highlight_start));
						bufferIndex = bufferIndex - (highlight_end - highlight_start);
						
//						textBuffer = (char*) realloc(textBuffer, bufferIndex*GROWTH_FACTOR*sizeof(char));
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
                } else if (e.key.keysym.sym == SDLK_ESCAPE) { // Note : Move cursor to the end of the buffer and change it to null before saving the file 
					if(argc == 2){
						printf("\n %d",bufferIndex);

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
						printf("%d, %d", highlight_start, highlight_end);
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
							event.key.repeat = 0; // Not a repeated key

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
							while (movedCol < currentCol && 
								   cursor < bufferIndex - 1 && 
								   textBuffer[cursor + 1] != '\n') {
								swap(&textBuffer[cursor], &textBuffer[cursor + 1]);
								cursor++;
								movedCol++;
							}
						}
					}
//					printf("%d\n",cursor);
				} 
								
				if(mod & KMOD_CTRL){
										
					if(e.key.keysym.sym == SDLK_s){	
						
						if(!file){
						const char *filters[2] = {".txt", ".c"};	
						
						filename = tinyfd_saveFileDialog(
							"Save File",            // Dialog title
							"default.txt",           // Default file name
							2,                      // Number of filters
							filters,                // File type filters
							"Text or C Files"       // Filter description
						);
						
						file = fopen(filename, "w");
						file = fopen(filename, "r+");
						}
						
						
						if(filename){
							
						
							if (file) {
								memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
								cursor = bufferIndex;
									
								printf("Here");
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
					
					if(e.key.keysym.sym == SDLK_c){
						if(highlight_flag == 1){
							SDL_SetClipboardText(string_slice(textBuffer,highlight_start,highlight_end));
						}
					}
					
					if(e.key.keysym.sym == SDLK_v){
						char* copied_text = SDL_GetClipboardText();
						char* cleaned_text = malloc(strlen(copied_text) + 1);
						size_t clean_index = 0;

			
						for (size_t i = 0; i < strlen(copied_text); i++) {
							if (copied_text[i] != '\r') {
								cleaned_text[clean_index++] = copied_text[i];
							}
						}
						
						cleaned_text[clean_index] = '\0';  // Null terminate the cleaned string

						if (cleaned_text) {
	//						printf(cleaned_text);
							size_t paste_len = strlen(cleaned_text);
							size_t buffer_len = strlen(textBuffer);

							// Create a cleaned version of the text
							
							// Clean up the text - remove \r characters
							while((bufferIndex + paste_len) > buffer_size){
								buffer_size = buffer_size * GROWTH_FACTOR;
								textBuffer = (char*)realloc(textBuffer, buffer_size);
	//							tempBuffer = (char*)realloc(tempBuffer, buffer_size);

								if (textBuffer == NULL || tempBuffer == NULL) {
									printf("realloc failed");
									free(textBuffer);
									free(tempBuffer);
									fclose(file);	
									exit(1);
								}
								printf("Buffer Updated! Buffer size: %d",buffer_size);					

							}
							
							if ((bufferIndex + paste_len) < buffer_size) {
								// Make space for the new text by moving existing text
								memmove(&textBuffer[cursor + paste_len], &textBuffer[cursor], (buffer_len - cursor + 1) * sizeof(char));

								// Insert the copied text at the cursor position
								memmove(&textBuffer[cursor], cleaned_text, paste_len * sizeof(char));

								// Update buffer index and cursor position
								bufferIndex += paste_len;
								cursor += paste_len;
							}
							// Free the clipboard text
							SDL_free(copied_text);
							free(cleaned_text);
						}
					}
				}

				if (mod & KMOD_SHIFT) {
//					printf("Shift is pressed.\n");
					
					temp_flag = 0;
					
					// Store current cursor position before any movement
					//highlight_anchor;
					
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
					if(e.key.keysym.sym == SDLK_a){
						highlight_flag = 1;
						highlight_start = 0;
						highlight_end = bufferIndex;
						
						temp_flag = 1;
						
						memmove(&textBuffer[cursor],&textBuffer[cursor+1], bufferIndex - cursor);
						cursor = bufferIndex;
					}					
				}
                else if (mod & KMOD_ALT);   //printf("Alt is pressed.\n");
                else if (mod & KMOD_GUI);  //printf("GUI (Windows/Command) is pressed.\n");
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
                if (bufferIndex < buffer_size - 1) {
					tmpBufferIndex = bufferIndex; // Remember to reuse this code
					while(cursor < tmpBufferIndex){
						swap(&textBuffer[tmpBufferIndex+1], &textBuffer[tmpBufferIndex]);
						tmpBufferIndex--;
                    }
					textBuffer[cursor] = *e.text.text;					
					bufferIndex++;
                    cursor++;
                }
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
		SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
		SDL_RenderClear(renderer);
		
		// Update screen
//		SDL_RenderPresent(renderer);
        

		int temp_cursor = cursor;
		int temp_cnt = 0;

		// Clear tempBuffer
		tempBuffer[0] = '\0';

		// Find the starting position for the last 50 lines
		while (temp_cursor >= 0 && temp_cnt < 50) {
			if (textBuffer[temp_cursor] == '\n') {
				temp_cnt++;
			}
			temp_cursor--;
		}

		int start_pos = temp_cursor + 1; // Start position for backward lines

		// Reset cursor and counter to find the ending position for the next 50 lines
		temp_cursor = cursor;
		temp_cnt = 0;

		while (temp_cursor <= bufferIndex && temp_cnt < 50) {
			if (textBuffer[temp_cursor] == '\n') {
				temp_cnt++;
			}
			temp_cursor++;
		}

		int end_pos = temp_cursor; // End position for forward lines

		// Copy the slice of text into tempBuffer
		int length = end_pos - start_pos;
		if (length > 0) {
			strncpy(tempBuffer, textBuffer + start_pos, length);
			tempBuffer[length] = '\0'; // Null-terminate the tempBuffer
		} else {
			printf("Error: Buffer size exceeded or invalid length.\n");
		}
		
//		printf(tempBuffer);
        
//		strcpy(tempBuffer, textBuffer);
        
        // Tokenize and render text based on newline characters
		char* token;
		char* str = tempBuffer; 
		
		int y_off = 0;
		int tokenCnt = 0;
		int highlight_text_index = 0;
		int totalCharsProcessed = 0;
		
		// Maybe figure out an efficient way to keep track of the cursor line wihtout using the for loop everywhere? 
		// If have to use, locate a position for for loop where it won't have to computed again in the mouse click event (to compute the last line)
		cursor_line = 0;
		
		for(int i=cursor; i>0; i--){
			if(textBuffer[i] == '\n'){
				cursor_line++;
			}
		}

		drawcursor();			

		while ((token = strsep(&str, "\n")) != NULL) {
			tokenCnt++;
			int lineLength = strlen(token);
			// If token is empty, use a space character instead
			char* textToRender = strlen(token) == 0 ? " " : token;
			

			SDL_Surface* textSurface = TTF_RenderText_Blended(font, textToRender, textColor);
			if (textSurface) {
				SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
				if (textTexture) {
					SDL_Rect textRect = {0, y_off + render_y_off, textSurface->w, textSurface->h};
					y_off += textSurface->h;
//					printf("%d, %d \n", textSurface->w, textSurface->h);

					if (highlight_flag == 1) {
						// Calculate the actual position in the full text
						int lineStartIdx = totalCharsProcessed;
						int lineEndIdx = lineStartIdx + lineLength;
						
						// If the line is in the highlighted range
						if (highlight_start < lineEndIdx && highlight_end > lineStartIdx) {
							// Calculate highlight positions relative to this line
							int highlightBegin = (highlight_start > lineStartIdx) ? 
											   (highlight_start - lineStartIdx) : 0;
							int highlightEndIdx = (highlight_end < lineEndIdx) ? 
												(highlight_end - lineStartIdx) : lineLength;
							
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
													 highlightColor.g, highlightColor.b, 
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
			
			if(new_drawer_flag && mouse_clicked_flag){
				printf("New Drawer Selected!");
				new_drawer_flag = false;
			}
			else if(open_drawer_flag && mouse_clicked_flag){
				printf("Open Drawer Selected!");
				open_drawer_flag = false;
			}
			else if(save_drawer_flag && mouse_clicked_flag){
				printf("Save Drawer Selected!");
				save_drawer_flag = false;
			}
			else if(saveas_drawer_flag && mouse_clicked_flag){
				printf("Save as Drawer Selected!");
				saveas_drawer_flag = false;
			}
			else if(exit_drawer_flag && mouse_clicked_flag){
				printf("Exit Drawer Selected!");
				exit_drawer_flag = false;
			}
		}else{
			file_item_drop_down_flag == false;
		}

		if(themes_item_drop_down_flag == true){
			draw_themes_dropddown();
			
			if(theme_drawer_forest_flag && mouse_clicked_flag){
				printf("Theme Forest Drawer Selected!");
				theme_drawer_forest_flag = false;
			}
			else if(theme_drawer_mountain_flag && mouse_clicked_flag){
				printf("Theme Mountain Drawer Selected!");
				theme_drawer_mountain_flag = false;
			}
			else if(theme_drawer_bubblegum_flag && mouse_clicked_flag){
				printf("Theme Bubblegum Drawer Selected!");
				theme_drawer_bubblegum_flag = false;
			}
			else if(theme_drawer_wood_flag && mouse_clicked_flag){
				printf("Theme wood Drawer Selected!");
				theme_drawer_wood_flag = false;
			}
			else if(theme_drawer_tiles_flag && mouse_clicked_flag){
				printf("Theme tiles Drawer Selected!");
				theme_drawer_tiles_flag = false;
			}
			else if(theme_drawer_obsidian_flag && mouse_clicked_flag){
				printf("Theme obsidian Drawer Selected!");
				theme_drawer_obsidian_flag = false;
			}
		}else{
			themes_item_drop_down_flag = false;
		}
		
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


// Helper Function to display notificaiton 

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
	SDL_SetRenderDrawColor(renderer, 50, 50, 50, 150); // Gray background
	SDL_RenderFillRect(renderer, &notificationRect);

	// Add text to the notification
	SDL_Color textColor = {255, 255, 255, 255}; // White text
	SDL_Surface *textSurface = TTF_RenderText_Blended(font, message, textColor);
	SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

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

    // Delay to display the notification for the specified duration (in milliseconds)
}
