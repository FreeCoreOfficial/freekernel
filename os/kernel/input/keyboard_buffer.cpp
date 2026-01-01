#include "keyboard_buffer.h"

#define KBD_BUF_SIZE 128

static char buffer[KBD_BUF_SIZE];
static volatile int head = 0;
static volatile int tail = 0;

void kbd_buffer_init()
{
    head = 0;
    tail = 0;
}

static int next_index(int i)
{
    return (i + 1) % KBD_BUF_SIZE;
}

void kbd_push(char c)
{
    int next = next_index(head);

    if (next == tail)
        return; // buffer full → drop

    buffer[head] = c;
    head = next;
}

bool kbd_has_char()
{
    return head != tail;
}

char kbd_get_char()
{
    if (head == tail)
        return 0;

    char c = buffer[tail];
    tail = next_index(tail);
    return c;
}
