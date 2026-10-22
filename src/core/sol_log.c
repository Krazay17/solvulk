#include "sol_core.h"
#include "stdarg.h"
#include "time.h"

static FILE *log_file;

int sollog_init()
{
    log_file = fopen("sol.log", "a");
    if (log_file)
    {
        time_t now   = time(NULL);
        struct tm *t = localtime(&now);

        if (t)
        {
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
            fprintf(log_file, "\n--- Sol Log: %s ---\n", timestamp);
            fflush(log_file);
        }
    }
    return log_file != NULL ? 0 : 1;
}

void sollog_close()
{
    if (log_file)
    {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
}

void sollogf(const char *fmt, ...)
{
    if (!log_file)
        return;

    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    fputc('\n', log_file);
    fflush(log_file);
}