#include <stdio.h>

#define MAX 100
int stack[MAX];
int top = -1; // Initialize top of stack

int is_full() {
    return top == MAX - 1;
}

int is_empty() {
    return top == -1;
}

void push(int value) {
    if (is_full()) {
        printf("Stack Overflow\n");
        return;
    }
    stack[++top] = value;
}

int pop() {
    if (is_empty()) {
        printf("Stack Underflow\n");
        return -1; // Return an error value
    }
    return stack[top--];
}

int peek() {
    if (is_empty()) {
        printf("Stack is empty\n");
        return -1; // Return an error value
    }
    return stack[top];
}

void display() {
    if (is_empty()) {
        printf("Stack is empty\n");
        return;
    }
    printf("Stack elements: ");
    for (int i = top; i >= 0; i--) {
        printf("%d ", stack[i]);
    }
    printf("\n");
}

int main() {
    push(10);
    push(20);
    push(30);
    display(); // Should display 30, 20, 10

    printf("Top element is %d\n", peek()); // Should display 30

    printf("Popped element is %d\n", pop()); // Should display 30
    display(); // Should display 20, 10

    return 0;
}