#include <stdio.h>
#include <string.h>

int main() {
    char name[50];
    printf("请输入你的名字：");
    scanf("%s", name);
    printf("你输入的名字是：%s\n", name);
    printf("Hello, %s! 欢迎进入嵌入式开发世界！\n", name);
    return 0;
}