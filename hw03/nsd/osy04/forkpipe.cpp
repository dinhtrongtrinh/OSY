#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>

volatile std::sig_atomic_t g_terminate_requested = 0;

extern "C" void handle_sigterm(int) {
    g_terminate_requested = 1;
}

int main() {
    pid_t pid_gen = -1;
    pid_t pid_nsd = -1;
    int fd[2];

    if (pipe(fd) == -1) {
        std::perror("pipe");
        return 2;
    }

    pid_gen = fork();
    if (pid_gen == -1) {
        std::perror("fork gen");
        close(fd[0]);
        close(fd[1]);
        return 2;
    }

    if (pid_gen == 0) {
        if (std::signal(SIGTERM, handle_sigterm) == SIG_ERR) {
            std::perror("signal");
            _exit(2);
        }

        close(fd[0]);

        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            std::perror("dup2");
            close(fd[1]);
            _exit(2);
        }
        close(fd[1]);

        std::srand(static_cast<unsigned>(getpid() ^ (unsigned)time(nullptr)));
        while (!g_terminate_requested) {
            std::printf("%d %d\n", std::rand() % 4096, std::rand() % 4096);
            std::fflush(stdout);
            sleep(1);
        }

        std::fprintf(stderr, "GEN TERMINATED\n");
        std::fflush(stderr);
        _exit(0);
    }

    pid_nsd = fork();
    if (pid_nsd == -1) {
        std::perror("fork nsd");
        kill(pid_gen, SIGTERM);
        waitpid(pid_gen, nullptr, 0);
        close(fd[0]);
        close(fd[1]);
        return 2;
    }

    if (pid_nsd == 0) {
        close(fd[1]);

        if (dup2(fd[0], STDIN_FILENO) == -1) {
            std::perror("dup2 nsd");
            close(fd[0]);
            _exit(2);
        }
        close(fd[0]);

        execl("./nsd", "nsd", static_cast<char*>(nullptr));
        std::perror("execl ./nsd");
        _exit(2);
    }

    close(fd[0]);
    close(fd[1]);

    sleep(5);
    if (kill(pid_gen, SIGTERM) == -1) {
        std::perror("kill gen");
    }

    int status_gen = 0;
    int status_nsd = 0;

    if (waitpid(pid_gen, &status_gen, 0) == -1) {
        std::perror("waitpid gen");
    }
    if (waitpid(pid_nsd, &status_nsd, 0) == -1) {
        std::perror("waitpid nsd");
    }

    bool error = false;

    if (WIFEXITED(status_gen)) {
        if (WEXITSTATUS(status_gen) != 0) {
            error = true;
        }
    } else if (WIFSIGNALED(status_gen)) {
        error = true;
    } else {
        error = true;
    }

    if (WIFEXITED(status_nsd)) {
        if (WEXITSTATUS(status_nsd) != 0) {
            error = true;
        }
    } else {
        error = true;
    }

    if (error) {
        std::printf("ERROR\n");
        return 1;
    } else {
        std::printf("OK\n");
        return 0;
    }
}
