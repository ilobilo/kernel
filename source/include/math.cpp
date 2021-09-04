int pow(int base, int exp)
{
    int result = 1;
    for (exp; exp > 0; exp--)
    {
        result = result * base;
    }
    return result;
}