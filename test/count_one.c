int x, cnt;

int main() {
    int i;
    i = 7;
    x = 217;
    cnt = 0;

    printf("number of 1 in %d(", x);
    while (i >= 0) {
        printf("%d", (x & (1 << i)) >> i);
        --i;
    }

    while (x) {
        // x -= (x & (-x));
        x = x & (x - 1);
        ++cnt;
    }
    printf(") is %d\n", cnt);
    
    return 0;
}