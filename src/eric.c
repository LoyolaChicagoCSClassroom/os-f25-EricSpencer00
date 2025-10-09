#include <stdio.h>

int main(void) {
    char input[100];

    printf("Enter something: ");

    fgets(input, sizeof(input), stdin);
    // input = input.toUpper();
    for (int i = 0; input[i]; i++) {
        if (input[i] >= 'a' && input[i] <= 'z') {
            input[i] = input[i] - ('a' - 'A');
        }
    }

    printf("You typed: %s", input);
    return 0;
}