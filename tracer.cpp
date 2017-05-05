#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <memory>

int signal_transmission(int parent_pid, int signal) {
    std::string ps_cmd("ps --no-headers -o pid --ppid=" + std::to_string(parent_pid));
    std::shared_ptr<FILE> ps(popen(ps_cmd.c_str(), "r"), pclose);
    if (!ps) {
        std::cout << "can't get child pids\n";
        return -1;
    }

    int pid;

    while (!feof(ps.get())) {
        if (fscanf(ps.get(), "%d", &pid) && kill(pid, signal))
            std::cout << "error while passing " << signal << " to " << pid << '\n';
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cout << "usage:\n\t./tracer <command> <options>\n";
        return -1;
    }

    const pid_t pid = fork();

    if (pid < 0) {
        std::cout << "fork failed\n";
        return -1;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);
    } else {
        std::cout << "process started [pid: " << pid << "]\n";

        int status;

        while (1) {
            if (waitpid(pid, &status, WUNTRACED | WCONTINUED) < 0)
                break;

            if (WIFEXITED(status))
                break;

            if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
                std::cout << "process stopped\n";
                signal_transmission(pid, SIGSTOP);
            } else if (WIFCONTINUED(status)) {
                std::cout << "process resumed\n";
                signal_transmission(pid, SIGCONT);
            }
        }
        std::cout << "process finished [pid: " << pid << "]\n";
    }

	return 0;
}
