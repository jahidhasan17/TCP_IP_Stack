#include <stdio.h>


void func1() {
    for (int i = 0; i < 10; i++) {
        static int val = 0;
        val++;

        printf("func1 - val: %d\n", val);
    }
}

void func2() {
    for (int i = 0; i < 10; i++) {
        static int val = 0;
        val++;

        printf("func2 - val: %d\n", val);
    }
}

int main() {
    for (int i = 0; i < 10; i++) {
        static int val = 0;
        val++;

        printf("Parent - val: %d\n", val);
    }

    func1();
    func2();
}