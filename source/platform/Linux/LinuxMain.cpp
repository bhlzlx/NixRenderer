#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <NixApplication.h>
#include <nix/io/archieve.h>
#include <string>

#include <unistd.h>

std::string GetOwnerPath()
{
    char path[1024];
    int cnt = readlink("/proc/self/exe", path, 1024);
    if(cnt < 0|| cnt >= 1024)
    {
        return NULL;
    }
//最后一个'/' 后面是可执行程序名，去掉可执行程序的名字，只保留路径
    for(int i = cnt; i >= 0; --i)
    {
        if(path[i]=='/')
        {
            path[i + 1]='\0';
            break;
        }
    }
    std::string s_path(path);   //这里我为了处理方便，把char转成了string类型
    return s_path;
}

int main(void) {
    Display *d;
    Window w;
    XEvent e;
    NixApplication* game = GetApplication();
    std::string path = GetOwnerPath();
    path += "/../..";
    Nix::IArchieve* arch = Nix::CreateStdArchieve(path);
    int s;
    d = XOpenDisplay(NULL);
    
    game->initialize( d, arch );
    
    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d, s), 100, 100, 500, 500, 1, 777215, 111111);
    
    game->resize(500, 500);
    
    XSelectInput(d, w, ExposureMask | KeyPressMask);
    
    XMapWindow(d, w);
    while (1) {
        XNextEvent(d, &e);
        if (e.type == Expose) {
        }
        if (e.type == KeyPress) break;
        game->tick();
    }
    XCloseDisplay(d);
    return 0;
}
