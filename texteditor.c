#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>



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


// Menu Bar 
int netWidth;
int netHeight;

// Menu item dimensions
const int MENU_HEIGHT = 20;
const int MENU_ITEM_WIDTH = 40;

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
    char textBuffer[256] = ""; // Buffer to store user input
    char tempBuffer[256]; // Temporary buffer for strtok operations
    int quit = 0;
	int cursor = 0;
	int bufferIndex = 0;
	int tmpBufferIndex = 0;
	FILE *file;
		
	if (argc == 2){
		char *filename = argv[1];  // Get the filename from command-line arguments
		file = fopen(filename, "r+");  // Open the file in read mode

		if (file == NULL) {
			perror("Error opening file");
			return 1;
		}

		size_t bytesRead;

		while ((bytesRead = fread(textBuffer, 1, 256, file)) > 0) {
			// Write the buffer content to stdout (or process it as needed)
			fwrite(textBuffer, 1, bytesRead, stdout);
		}
		bufferIndex = strlen(textBuffer);
		printf("%d",bufferIndex);
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

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 16);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Color textColor = {0, 0, 0, 255}; // Black color
    SDL_StartTextInput();

	Uint32 cursorBlinkTime = SDL_GetTicks(); // Cursor Blink time 
	int showCursor = 1;

    while (!quit) {

        SDL_Event e;
        
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT){
				if(argc == 2){
					ftruncate(fileno(file), 0);
					rewind(file);
					while(cursor < bufferIndex){
						swap(&textBuffer[cursor], &textBuffer[cursor+1]);
						cursor++;
					}
					textBuffer[cursor] = '\0';
					fprintf(file, textBuffer);
					fclose(file);
				}
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && cursor > 0) {
					textBuffer[cursor-1] = textBuffer[cursor];
					cursor--;
					for(int i=1; i<bufferIndex; i++){
						textBuffer[cursor+i] = textBuffer[cursor+i+1];
                    }					
					textBuffer[bufferIndex] = '\0';
                    bufferIndex--;
                } else if (e.type == SDL_WINDOWEVENT) {
					// Handle window resize event
					if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						// Get the new window dimensions						
						SDL_GetWindowSize(window, &netWidth, &netHeight);
						// You can now use the new width and height for rendering or layout adjustments
						SDL_Log("Window resized to %d x %d", netWidth, netHeight);
					}
				} else if (e.key.keysym.sym == SDLK_RETURN) {
					tmpBufferIndex = bufferIndex; // Remember to reuse this code 
					while(cursor < tmpBufferIndex){
						swap(&textBuffer[tmpBufferIndex+1], &textBuffer[tmpBufferIndex]);
						tmpBufferIndex--;
                    }
                    textBuffer[cursor] = '\n'; // Insert a newline character on Return key
					bufferIndex++;
                    cursor++;
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
					if(argc == 2){
						ftruncate(fileno(file), 0);
						rewind(file);
						while(cursor < bufferIndex){
							swap(&textBuffer[cursor], &textBuffer[cursor+1]);
							cursor++;
						}
						textBuffer[cursor] = '\0';
						fprintf(file, textBuffer);
						fclose(file);
					}
                    quit = 1; // Quit on Escape
                } else if (e.key.keysym.sym == SDLK_LEFT && cursor > 0) {
                    swap(&textBuffer[cursor - 1], &textBuffer[cursor]);
                    --cursor;
                } else if (e.key.keysym.sym == SDLK_RIGHT && cursor < bufferIndex) {
                    swap(&textBuffer[cursor + 1], &textBuffer[cursor]);
                    ++cursor;
                } else if (e.key.keysym.sym == SDLK_UP) {

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
								i++;
								swap(&textBuffer[cursor + 1], &textBuffer[cursor]);
								cursor++;
								if(textBuffer[cursor + 1] == '\n')
									break;
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
				}
            } else if (e.type == SDL_TEXTINPUT) {
                if (bufferIndex < sizeof(textBuffer) - 1) {
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
		
 //       textBuffer[cursor] = '|'; // Render cursor as '|'

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
        SDL_RenderClear(renderer);

        // Copy the buffer because strtok modifies the original string
        strcpy(tempBuffer, textBuffer);
        
        // Tokenize and render text based on newline characters
		char* token;
		char* str = tempBuffer; 
		
		int y_off = 0;
		int tokenCnt = 0;
		
		while ((token = strsep(&str, "\n")) != NULL) {
			tokenCnt++;
			
			// If token is empty, use a space character instead
			char* textToRender = strlen(token) == 0 ? " " : token;
			
			SDL_Surface* textSurface = TTF_RenderText_Blended(font, textToRender, textColor);
			if (textSurface) {
				SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
				if (textTexture) {
					SDL_Rect textRect = {0, y_off+25, textSurface->w, textSurface->h};
					y_off += textSurface->h;
			printf("%d, %d \n", textSurface->w, textSurface->h);
				
					SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
					SDL_DestroyTexture(textTexture);
				}
				SDL_FreeSurface(textSurface);
			}
		}
//		printf("%d",tokenCnt); DEGUBGIN ATTEMPT TO IMPLEMENT A NEW LINE ON AN EMPTY TOKEN 
		        
		drawMenuBar();
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

