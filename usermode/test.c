char* data = "Hello, world!";

int main(void)
{
    int i = 0;
    while (data[i] != '\0') {
        i++;
    }
    return i;
}
