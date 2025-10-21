#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

int end = 0;

void f(int signum) {
    end = 1;
}

int main () {
    pid_t gen;
    pid_t nsd;

    int fd[2];

    if (pipe(fd) == -1) {
        exit(2);
    }

    gen = fork();

    if (gen == -1) {
        exit(2);
    } else if (gen == 0) {
        signal(SIGTERM, f);

        close(fd[0]);

        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            exit(2);
        }
        close(fd[1]);

        while (end == 0) {
            printf("%d %d\n", rand() % 4096, rand() % 4096);
            fflush(stdout);
            sleep(1);
        }
        fprintf(stderr, "GEN TERMINATED\n");
        fflush(stderr);

        exit(0);
    }

    nsd = fork();

    if (nsd == -1) {
        exit(2);
    } else if (nsd == 0) {
        close(fd[1]);

        if (dup2(fd[0], STDIN_FILENO) == -1) {
            exit(2);
        }
        close(fd[0]);

        execl("./nsd", "nsd", (char *)NULL);
        exit(2);
    }

    close(fd[0]);
    close(fd[1]);

    sleep(5);

    kill(gen, SIGTERM);

    int status_gen, status_nsd;

    waitpid(gen, &status_gen, 0);
    waitpid(nsd, &status_nsd, 0);

    int error = 0;
    if (WIFEXITED(status_gen)) {
        if (WEXITSTATUS(status_gen) != 0) {
            error = 1;
        } 
    } else if (WIFSIGNALED(status_gen)) {
        error = 1; 
    }
    if (WIFEXITED(status_nsd)) {
        if (WEXITSTATUS(status_nsd) != 0)
            error = 1;
    } else {
        error = 1;
    }

    if (error) {
        printf("ERROR\n");
        return 1;
    } else {
        printf("OK\n");
        return 0;
    }
    
}
