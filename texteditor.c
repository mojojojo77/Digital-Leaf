define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define INITIAL_SIZE 256
#define GROWTH_FACTOR 2


// Helper Functions
// Easy Swap Function 
void swap(char *a, char *b) {
    char temp = *a;
    *a = *b;
    *b = temp;
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


// Highlight flag
bool highlight_flag = 0;
int highlight_start;
int highlight_end;

// Menu Bar 
int netWidth;
int netHeight;

// Menu item dimensions
const int MENU_HEIGHT = 20;
const int MENU_ITEM_WIDTH = 40;

// Colour of the highlight when text is selected
SDL_Color highlightColor = {173, 200, 255, 255};

enum MenuItem {
    FILE_QUIT,
    FILE_COUNT
};

SDL_Window* window = NULL;
		
SDL_Renderer* renderer = NULL;

TTF_Font* font = NULL;
TTF_Font* font_menu = NULL;


void drawMenuBar() {
	// Set the color for the menu bar (dark gray)
	SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
	SDL_GetWindowSize(window, &netWidth, &netHeight);
	SDL_Rect menuBar = {0, 0, netWidth, MENU_HEIGHT};
	SDL_RenderFillRect(renderer, &menuBar);

	// Draw the "File" menu item
	SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);  // White color
	SDL_Rect fileItem = {0, 0, MENU_ITEM_WIDTH, MENU_HEIGHT};
	SDL_RenderFillRect(renderer, &fileItem);
	
	// Draw a border around the "File" menu item (black border)
//	SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);  // Black color for the border
//	SDL_RenderDrawRect(renderer, &menuBar);  // Draw the border around the menu item
	
//	SDL_RenderDrawLine(renderer, menuBar.x, menuBar.y, menuBar.x, menuBar.y + menuBar.h);          // Left side
//	SDL_RenderDrawLine(renderer, menuBar.x + menuBar.w, menuBar.y, menuBar.x + menuBar.w, menuBar.y + menuBar.h);  // Right side
//	SDL_RenderDrawLine(renderer, menuBar.x, menuBar.y + menuBar.h, menuBar.x + menuBar.w, menuBar.y + menuBar.h);  // Bottom side


	// Set text color (black)
	SDL_Color textColor = {0, 0, 0, 255};

	font_menu = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 13);
	
	// Render the text for "File"
	SDL_Surface* textSurface = TTF_RenderText_Blended(font_menu, "File", textColor);
	if (!textSurface) {
		SDL_Log("Unable to render text surface! TTF_Error: ¿m*æç\n", TTF_GetError());
	}

	// Create a texture from the surface
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_FreeSurface(textSurface);  // Free the surface as it's no longer needed

	// Get the dimensions of the text texture
	int textWidth = 0, textHeight = 0;
	SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

	// Calculate the position to center the text inside the menu item
	SDL_Rect textRect = {
		fileItem.x + (fileItem.w - textWidth) / 2,  // Center horizontally
		fileItem.y + (fileItem.h - textHeight) / 2, // Center vertically
		textWidth,
		textHeight
	};

	// Render the text on the screen
	SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

	// Destroy the text texture after rendering
	SDL_DestroyTexture(textTexture);
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
	FILE *file;
	
	textBuffer[0] = '\0';
		
	if (argc == 2