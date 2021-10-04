int pow(int base, int exp)
{
    int result = 1;
    for (exp; exp > 0; exp--)
    {
        result = result  *base;
    }
    return result;
}

int abs(int num)
{
    return num < 0 ? -num : num;
}

int sign(int num)
{
    return (num > 0) ? 1 : ((num < 0) ? -1 : 0);
}