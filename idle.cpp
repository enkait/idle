// A simple script that polls X11 to find if the user is idle for some number of seconds.
// If yes, it sends a SIGCONT signal to a process given by pid.
// When the user becomes active again, it sends SIGSTOP to the process.
// Useful for doing calculations in the background which would be annoying to the user if they
// were working. In particular, IO heavy calculations can make other processes unresponsive,
// even if they are ionice'd, because it can take a long time to flush the IO queue.

#include <X11/extensions/scrnsaver.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <signal.h>

int stop_process(pid_t pid) {
    std::cout << "Stopping process: " << pid << std::endl;
    return kill(pid, SIGSTOP);
}

int cont_process(pid_t pid) {
    std::cout << "Continuing process: " << pid << std::endl;
    return kill(pid, SIGCONT);
}

class XScreenSaver {
    XScreenSaverInfo *info;
    Display *display;

    public:
    XScreenSaver() {
        info = XScreenSaverAllocInfo();
        assert(info > 0);
        display = XOpenDisplay(0);
        assert(display > 0);
    }

    ~XScreenSaver() {
        XCloseDisplay(display);
        XFree(info);
    }

    unsigned long get_idle_time() {
        XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
        return info->idle;
    }
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Stops a process given by [pid], until the user has been inactive for [secs] seconds." << std::endl;
        std::cout << "Usage: ./idle [secs] [pid]" << std::endl;
        std::cout << "Note that killing ./idle will not make the process continue - send SIGCONT to it to do that." << std::endl;
        return 0;
    }
    unsigned long s;
    pid_t pid;
    std::stringstream(argv[1]) >> s;
    std::stringstream(argv[2]) >> pid;
    s *= 1000;
    if (pid <= 0) {
        std::cout << "You don't want to do this with pid: " << pid << std::endl;
        return 0;
    }
    XScreenSaver ss;

    stop_process(pid);
    while (true) {
        unsigned long idle_time = ss.get_idle_time();
        std::cout << "Current idle time: " << idle_time << "ms/" << s << "ms" << std::endl;
        if (idle_time > s) {
            cont_process(pid);
            while(true) {
                int idle_time = ss.get_idle_time();
                std::cout << "Current idle time: " << idle_time << "ms/" << s << "ms" << std::endl;
                if (idle_time < s) {
                    stop_process(pid);
                    break;
                }
                usleep(50000);
            }
        }
        sleep(1);
    }
    return 0;
}
