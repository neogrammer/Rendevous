#include "client.h"


int main()
{
    Client c;
    if (c.OnUserCreate()) c.run();

    HWND hWnd = GetConsoleWindow();
    CloseWindow(hWnd);
    return 0;
}