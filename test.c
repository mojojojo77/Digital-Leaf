#include<stdio.h>
#include<string.h>

int main(){
	
	char a[] = "This\nis\na\nstring";
	char* b = strtok(a,"\n");
	
	while(b != NULL){
		printf(b);
		b = strtok("\n", "\n");
	}
	
	return 0;
}