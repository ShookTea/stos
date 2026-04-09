extern char** environ;

char* getenv(const char* name __attribute__((unused)))
{
    return environ[0];
}
