<<<<<<< HEAD
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Helper Functions
void swap(char *a, char *b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL2 Text Input",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 24);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Color textColor = {0, 0, 0, 255}; // Black color
    char textBuffer[256] = ""; // Buffer to store user input
    char tempBuffer[256]; // Temporary buffer for strtok operations
    int quit = 0;
	int cursor = 0;
	int bufferIndex = 0;
	int tmpBufferIndex = 0;
    SDL_StartTextInput();

    while (!quit) {
        SDL_Event e;
        
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
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
        textBuffer[cursor] = '|'; // Render cursor as '|'

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
        SDL_RenderClear(renderer);

        // Copy the buffer because strtok modifies the original string
        strcpy(tempBuffer, textBuffer);
        
        // Tokenize and render text based on newline characters
        char* textBufferToken = strtok(tempBuffer, "\n");
        int y_off = 0;
        int tokenCnt = 0;

        while (textBufferToken != NULL) {
            tokenCnt++;
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, textBufferToken, textColor);
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture) {
                    SDL_Rect textRect = {0, y_off, textSurface->w, textSurface->h};
                    y_off += textSurface->h;
                    
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            textBufferToken = strtok(NULL, "\n");
			printf("%d, %d \n",textSurface->w,textSurface->h);
        }

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

=======
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Test Commit

// Helper Functions
void swap(char *a, char *b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL2 Text Input",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 24);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Color textColor = {0, 0, 0, 255}; // Black color
    char textBuffer[256] = ""; // Buffer to store user input
    char tempBuffer[256]; // Temporary buffer for strtok operations
    int quit = 0;
	int cursor = 0;
	int bufferIndex = 0;
	int tmpBufferIndex = 0;
    SDL_StartTextInput();

    while (!quit) {
        SDL_Event e;
        
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
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
        textBuffer[cursor] = '|'; // Render cursor as '|'

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
        SDL_RenderClear(renderer);

        // Copy the buffer because strtok modifies the original string
        strcpy(tempBuffer, textBuffer);
        
        // Tokenize and render text based on newline characters
        char* textBufferToken = strtok(tempBuffer, "\n");
        int y_off = 0;
        int tokenCnt = 0;

        while (textBufferToken != NULL) {
            tokenCnt++;
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, textBufferToken, textColor);
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture) {
                    SDL_Rect textRect = {0, y_off, textSurface->w, textSurface->h};
                    y_off += textSurface->h;
                    
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            textBufferToken = strtok(NULL, "\n");
			printf("%d, %d \n",textSurface->w,textSurface->h);
        }

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

>>>>>>> 3be01a2 (The repository in git is created with the latest progress in the code uploaded)
