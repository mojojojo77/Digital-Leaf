# TextEditor
Designing a text editor from scratch in C using the SDL2 graphics library

Downloading the files and running the executable should work.
If you encounter any errors, mention it in the issues section for me to fix! 

# If you are writing something important make sure to save files frequently might it crash, still working on improving it whenever I get time!

GCC compilation commands for texteditor.c file : 
gcc -O3 -o "Leaf.exe" .\texteditor.c .\lib\tinyfiledialogs\tinyfiledialogs.c -LC:/mingw/lib -lcomdlg32 -lole32 -lSDL2 -lSDL2main -lSDL2_ttf -lSDL2_image 
gcc -O3 -o "Leaf.exe" .\texteditor.c .\lib\tinyfiledialogs\tinyfiledialogs.c -LC:/mingw/lib -lcomdlg32 -lole32 -lSDL2 -lSDL2main -lSDL2_ttf -lSDL2_image .\text.res -mwindows 
