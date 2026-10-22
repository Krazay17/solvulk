/*
 * File: s_ai_names.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-21
 *
 */
#include "s_ai.h"

static const char *prefix[] = {
    "Guy", "Man",     "Mister", "Free", "Sergeant", "Ass",       "Crusher", "1337", "Kray",
    "Ya",  "Mobster", "Bro",    "Big",  "Monsieur", "Professor", "Lord",    "Soul", "Jack",
};

static const char *suffix[] = {
    "Guy",    "Dude",   "Man",  "Mister", "Infiltrator", "Beamer",  "Whoop", "99",    "Zay",    "Ya",
    "Killer", "Master", "Nerd", "Hopper", "Baby",        "Lobster", "Death", "Eater", "Ripper",
};

#define PREFIX_COUNT sizeof(prefix) / sizeof(prefix[0])
#define SUFFIX_COUNT sizeof(suffix) / sizeof(suffix[0])

void Sol_Ai_RandomName(char *dest, size_t size)
{
    snprintf(dest, size, "%s %s", prefix[rand() % PREFIX_COUNT], suffix[rand() % SUFFIX_COUNT]);
}