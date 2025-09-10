#include "core/client.h"


int main()
{
    Client c;
    HWND hWnd = GetConsoleWindow();
    CloseWindow(hWnd);

    if (c.OnUserCreate())
    {
        c.run();
    }


    return 0;
}